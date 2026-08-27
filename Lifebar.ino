#include "user_config.h"
#include "lvgl_port.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "i2c_bsp.h"
#include "src/lcd_bl_bsp/lcd_bl_pwm_bsp.h"

#include "lifebar_model.h"
#include "lifebar_log.h"
#include "lifebar_net.h"
#include "ui_lifebar.h"

/* ---- Notschalter zum Eingrenzen -------------------------------------
   Bleibt der Start haengen, eines davon auf 0 setzen und neu flashen.
   Der Marker im Serial Monitor zeigt, wie weit er gekommen ist.      */
#define USE_BOOT_RESET  1     /* Werksreset ueber die BOOT-Taste      */

/* Startmarker. Auf 0 setzen, wenn nicht gebraucht. */
#define BOOT_DEBUG 0

/* ---- Eingrenzung ----------------------------------------------------
   USE_NET 0  -> Funkteil bleibt komplett aus (kein WLAN, kein Portal)
   USE_WEB 0  -> WLAN verbindet, aber ohne Webserver, DNS und mDNS

   Damit laesst sich trennen, ob die Stoerung vom Funkbetrieb selbst
   kommt oder von den Netzwerkdiensten. Reihenfolge zum Testen:
     1) USE_NET 1, USE_WEB 0  - Funk an, keine Dienste
     2) USE_NET 0             - gar kein Funk
   Geht 1) nicht und 2) schon, ist es das Funkteil.
   Geht 1) und 2), ist es der Webserver.                          */
#define USE_NET 1
#define USE_WEB 1
#define USE_MDNS 1    /* mDNS getrennt schaltbar - belegt viel RAM */
#define HEAP_DEBUG 0  /* Speicherverbrauch mitschreiben            */
#define SHOW_SPLASH 0     /* Startbild - zum Eingrenzen abschaltbar */

#if BOOT_DEBUG
  #define STEP(n, txt) Serial.printf("[boot %d] %s\n", n, txt)
#else
  #define STEP(n, txt) do {} while (0)
#endif

static uint32_t week_check  = 0;
static bool     week_synced = false;

void setup()
{
  Serial.begin(115200);
  /* Entscheidend: ohne das blockiert jede Ausgabe unbegrenzt, sobald
     der USB-Puffer voll ist und der Host nicht abholt. Genau das hat
     den Start haengen lassen.                                       */
  Serial.setTxTimeoutMs(0);
  delay(300);
  STEP(1, "start");

  i2c_master_Init();
  STEP(3, "i2c init");

  lvgl_port_init();
  STEP(4, "lvgl");

  lcd_bl_pwm_bsp_init(LCD_PWM_MODE_255);
  STEP(5, "backlight");

#if SHOW_SPLASH
  if (lvgl_lock(3000)) { ui_splash(); lvgl_unlock(); }
  else Serial.println("[boot] WARNUNG: LVGL-Mutex nicht bekommen");
#endif
  uint32_t splash_t0 = millis();
  STEP(6, "splash");

  store_begin();
  STEP(7, "store");

#if USE_BOOT_RESET
  if (store_boot_reset_requested()) {
    Serial.println("[boot] Werksreset angefordert");
    store_wipe();
    log_begin();
    log_clear();
    if (lvgl_lock(2000)) { ui_message("RESET", NULL, 0xFF4D4D); lvgl_unlock(); }
    delay(1500);
    ESP.restart();
  }
#endif
  STEP(8, "boot-taste geprueft");

  log_begin();
  STEP(9, "log");

#if USE_NET
  net_begin();
#endif
  STEP(10, "net");

#if SHOW_SPLASH
  while (millis() - splash_t0 < 3300) delay(20);
#else
  (void)splash_t0;
#endif

  if (lvgl_lock(5000)) { ui_init(); lvgl_unlock(); }
  else Serial.println("[boot] WARNUNG: UI konnte nicht aufgebaut werden");
  STEP(11, "ui bereit");

#if HEAP_DEBUG
  Serial.printf("[heap] nach dem Start: frei %u, groesster Block %u\n",
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
#endif

  Serial.printf("[boot] configured=%d total=%lu min, %lu Ereignisse\n",
                st.configured, (unsigned long)total_min,
                (unsigned long)log_count);
}

void loop()
{
#if USE_NET
  bool changed = net_tick();
#else
  bool changed = false;
#endif

  if (!week_synced) {
    if (time_valid) {
      week_synced = true;
      week_check  = millis();
      if (week_tick(true)) { store_save_now(); changed = true; }
    }
  } else if (millis() - week_check > 60000UL) {
    week_check = millis();
    if (week_tick(true)) { store_save_now(); changed = true; }
  }

  if (changed && lvgl_lock(200)) {
    ui_net_changed();
    lvgl_unlock();
  }

  /* Aenderung kam ueber den Browser - Oberflaeche neu aufbauen */
#if USE_NET
  if (setup_applied && lvgl_lock(500)) {
    setup_applied = false;
    ui_init();
    lvgl_unlock();
  }

  /* Buchung aus dem Browser: nur die Anzeige nachziehen */
  if (ui_refresh_pending && lvgl_lock(200)) {
    ui_refresh_pending = false;
    main_refresh();
    lvgl_unlock();
  }
#endif

  ui_reset_tick();               /* Werksreset ausserhalb des UI-Tasks */
#if USE_NET
  ui_wifi_tick();                /* WLAN-Reset ebenso                  */
#endif

  log_tick();
  ui_prefs_tick();
#if HEAP_DEBUG
  {
    static uint32_t hlast = 0;
    if (millis() - hlast > 5000) {
      hlast = millis();
      Serial.printf("[heap] frei %u, groesster Block %u\n",
                    (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                    (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    }
  }
#endif

  store_tick();
  delay(20);
}

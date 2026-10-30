#include "lifebar_config.h"
#include "user_config.h"
#include "lvgl_port.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "i2c_bsp.h"

#include "lifebar_hw.h"
#include "lifebar_model.h"
#include "lifebar_log.h"
#include "lifebar_net.h"
#include "ui_lifebar.h"
#include "lifebar_controls.h"

/* Compile-Schalter stehen in lifebar_config.h */

#if BOOT_DEBUG
  #define STEP(n, txt) Serial.printf("[boot %d] %s\n", n, txt)
#else
  #define STEP(n, txt) do {} while (0)
#endif

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

  backlight_begin(100);          /* gespeicherte Stufe folgt nach store */
  STEP(5, "backlight");

#if SHOW_SPLASH
  if (lvgl_lock(3000)) { ui_splash(); lvgl_unlock(); }
  else Serial.println("[boot] WARNUNG: LVGL-Mutex nicht bekommen");
#endif
  uint32_t splash_t0 = millis();
  STEP(6, "splash");

  store_begin();
  backlight_set(BRIGHT);
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

  hw_begin();
  STEP(10, "bedienteile");

#if USE_NET
  net_begin();
#endif
  STEP(11, "net");

#if SHOW_SPLASH
  while (millis() - splash_t0 < 3300) delay(20);
#else
  (void)splash_t0;
#endif

  if (lvgl_lock(5000)) { ui_init(); lvgl_unlock(); }
  else Serial.println("[boot] WARNUNG: UI konnte nicht aufgebaut werden");
  STEP(12, "ui bereit");

#if HEAP_DEBUG
  Serial.printf("[heap] nach dem Start: frei %u, groesster Block %u\n",
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
#endif

  Serial.printf("[boot] configured=%d total=%lu min, %lu Ereignisse\n",
                st.configured, (unsigned long)total_min,
                (unsigned long)log_count);
}

/* Wochenwechsel: einmal, sobald die Uhr steht, danach jede Minute */
static bool week_sync_tick(void)
{
  static uint32_t week_check  = 0;
  static bool     week_synced = false;

  if (!time_valid) return false;
  if (week_synced && millis() - week_check < 60000UL) return false;

  week_synced = true;
  week_check  = millis();
  if (!week_tick(true)) return false;
  store_save_now();
  return true;
}

/* Aenderungen aus Netz, Woche und Browser auf das Display bringen */
static void ui_sync_tick(bool changed)
{
  if (changed && lvgl_lock(200)) {
    ui_net_changed();
    lvgl_unlock();
  }

#if USE_NET
  /* Aenderung kam ueber den Browser - Oberflaeche neu aufbauen */
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
}

void loop()
{
  controls_tick();               /* zuerst: Tasten sollen sofort reagieren */

#if USE_NET
  bool changed = net_tick();
#else
  bool changed = false;
#endif
  changed |= week_sync_tick();
  ui_sync_tick(changed);

  ui_reset_tick();               /* Werksreset ausserhalb des UI-Tasks */
#if USE_NET
  ui_wifi_tick();                /* WLAN-Reset ebenso                  */
#endif

  log_tick();
  ui_prefs_tick();
  store_tick();

#if HEAP_DEBUG
  static uint32_t hlast = 0;
  if (millis() - hlast > 5000) {
    hlast = millis();
    Serial.printf("[heap] frei %u, groesster Block %u\n",
                  (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
  }
#endif

  delay(20);
}

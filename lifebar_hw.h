#pragma once
#include <Arduino.h>
#include "driver/ledc.h"
#include "driver/pulse_cnt.h"
#include "lifebar_config.h"
#include "user_config.h"
#include "i2c_bsp.h"

#if USE_USB_MEDIA && !ARDUINO_USB_MODE
  #include "USB.h"
  #include "USBHIDConsumerControl.h"
  #define HAVE_USB_MEDIA 1
#else
  #define HAVE_USB_MEDIA 0
#endif

/* =====================================================================
   LifeBar - Hardware am Board (Waveshare ESP32-S3-Touch-LCD-3.49, V2)

   Backlight, Tasten, Drehknopf, USB-Mediatasten. Kein LVGL hier -
   alles wird aus loop() bedient, nichts davon aus dem UI-Task.
   ===================================================================== */

/* ---- Backlight -------------------------------------------------------
   Auf V2 speist LCD_BL (GPIO 42) ueber 10k + 39k in den FB-Pin des
   AP3032-Treibers, mit 100 nF dazwischen. Das ist analoges Dimmen,
   kein Flackern: LOW = voller LED-Strom, HIGH = der Treiber regelt
   auf null. BL_EN am IO-Expander schaltet den Treiber ganz ab.

   Aus dem Schaltplan gerechnet (FB-Referenz 0,2 V) faellt der Strom
   linear mit dem Tastgrad und ist bei etwa 64 % null. BL_DUTY_ZERO
   ist dieser Punkt in 8 Bit; die Stufen bleiben deutlich darunter.

   Der Absturz beim Helligkeitswechsel kam uebrigens nicht von hier,
   sondern vom Neuaufbau der Einstellungsseite - siehe scr_retire()
   in ui_lifebar.h.                                                   */
#define BL_LEDC_TIMER  LEDC_TIMER_3
#define BL_LEDC_CH     LEDC_CHANNEL_1
#define BL_DUTY_ZERO   164

static uint8_t bl_pct = 100;
static bool    bl_on  = true;

static void bl_apply(void)
{
  uint32_t duty = bl_on ? (uint32_t)BL_DUTY_ZERO * (100 - bl_pct) / 100 : 255;
  ledc_set_duty(LEDC_LOW_SPEED_MODE, BL_LEDC_CH, duty);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, BL_LEDC_CH);
}

static void backlight_begin(uint8_t pct)
{
  ledc_timer_config_t tc = {};
  tc.speed_mode      = LEDC_LOW_SPEED_MODE;
  tc.duty_resolution = LEDC_TIMER_8_BIT;
  tc.timer_num       = BL_LEDC_TIMER;
  tc.freq_hz         = 50 * 1000;
  tc.clk_cfg         = LEDC_USE_RC_FAST_CLK;

  ledc_channel_config_t cc = {};
  cc.gpio_num   = EXAMPLE_PIN_NUM_BK_LIGHT;
  cc.speed_mode = LEDC_LOW_SPEED_MODE;
  cc.channel    = BL_LEDC_CH;
  cc.intr_type  = LEDC_INTR_DISABLE;
  cc.timer_sel  = BL_LEDC_TIMER;
  cc.duty       = 0;

  ESP_ERROR_CHECK_WITHOUT_ABORT(ledc_timer_config(&tc));
  ESP_ERROR_CHECK_WITHOUT_ABORT(ledc_channel_config(&cc));
  bl_pct = pct;
  bl_apply();
}

static void backlight_set(uint8_t pct)
{
  bl_pct = pct > 100 ? 100 : pct;
  bl_apply();
}

/* Ganz an oder aus. Aus heisst: Treiber per BL_EN abgeschaltet und
   GPIO 42 auf HIGH - dunkel, auch wenn der Expander nicht antwortet. */
static void backlight_power(bool on)
{
  bl_on = on;
  if (on) { exio_set(EXIO_PIN_BL_EN, true);  bl_apply(); }
  else    { bl_apply(); exio_set(EXIO_PIN_BL_EN, false); }
}

/* ---- Bildschirm an/aus -------------------------------------------------
   Anfragen kommen aus dem Webserver und aus dem UI-Task (langer Druck
   auf den dunklen Bildschirm). Ausgefuehrt wird in loop().          */
#define SCREEN_REQ_NONE   -1
static volatile int8_t screen_req = SCREEN_REQ_NONE;   /* 0 = aus, 1 = an */
static bool screen_on = true;

/* ---- Tasten ------------------------------------------------------------
   Alle Tasten schalten nach GND, die Eingaenge haben Pull-ups.

   PWR (Rueckseite)    GPIO 16  - Bildschirm an/aus. Liest auch bei
                                  USB-Versorgung sauber LOW (ueber D3).
   Drehknopf A / B     GPIO 1 / 2   (22-Pin-Header J6)
   Knopf-Taster        GPIO 5       (J6)
   Zurueck/Play/Vor    GPIO 38 / 39 / 40  (J3)
   Bildschirm-Taste    GPIO 41      (J3) - wie PWR, aber aussen am Deck

   Bewusst gemieden: GPIO 0, 3, 45, 46 sind Strapping-Pins, GPIO 4
   haengt am Batterie-Spannungsteiler, GPIO 43 ist der UART-Ausgang,
   auf dem der Bootloader schreibt. GPIO 38-41 teilen sich die Leitungen
   mit dem SD-Slot - der bleibt in diesem Projekt leer.
   Dreht der Knopf falsch herum: PIN_KNOB_A und PIN_KNOB_B tauschen.  */
#define PIN_PWR_KEY    16
#define PIN_KNOB_A      1
#define PIN_KNOB_B      2
#define PIN_KNOB_SW     5
#define PIN_KEY_PREV   38
#define PIN_KEY_PLAY   39
#define PIN_KEY_NEXT   40
#define PIN_KEY_SCREEN 41

enum { KEY_PWR = 0, KEY_KNOB, KEY_PREV, KEY_PLAY, KEY_NEXT, KEY_SCREEN, KEY_COUNT };

#define KEY_BIT(k)     (1u << (k))
#define KEY_STABLE_MS  25      /* so lange muss ein Pegel stehen    */
#define KEY_HOLD_MS    600     /* ab hier zaehlt es als Halten      */

/* EC11: vier Flanken je Rastung. Bei Encodern mit halber Rastung
   (zwei Flanken) hier 2 eintragen.                                */
#define KNOB_EDGES_PER_DETENT 4

typedef struct {
  uint8_t pin;
  bool    down;           /* entprellter Zustand            */
  bool    raw;            /* letzter gelesener Pegel        */
  bool    held;           /* Halten wurde schon gemeldet    */
  uint32_t raw_t;         /* seit wann der Pegel ansteht    */
  uint32_t down_t;        /* seit wann gedrueckt            */
} hw_key_t;

static hw_key_t hw_keys[KEY_COUNT] = {
  { PIN_PWR_KEY }, { PIN_KNOB_SW }, { PIN_KEY_PREV }, { PIN_KEY_PLAY }, { PIN_KEY_NEXT },
  { PIN_KEY_SCREEN }
};

typedef struct {
  uint8_t click;          /* KEY_BIT: kurz gedrueckt und losgelassen */
  uint8_t hold;           /* KEY_BIT: haelt seit KEY_HOLD_MS         */
  int     steps;          /* Rastungen, + = im Uhrzeigersinn         */
} hw_events_t;

static pcnt_unit_handle_t knob_unit = NULL;
static int knob_base = 0;

/* Der Drehgeber laeuft ueber den Pulszaehler. Der zaehlt jede Flanke
   in Hardware, auch waehrend loop() gerade im Webserver steckt - beim
   Abfragen alle 20 ms gingen bei schnellem Drehen sonst Schritte
   verloren. Prellen hebt sich bei der Quadraturzaehlung von selbst
   auf (+1 -1), uebrig bleiben ganze Rastungen.                       */
static void knob_begin(void)
{
  pcnt_unit_config_t uc = {};
  uc.low_limit  = -30000;
  uc.high_limit =  30000;
  uc.flags.accum_count = 1;
  if (pcnt_new_unit(&uc, &knob_unit) != ESP_OK) { knob_unit = NULL; return; }

  pcnt_glitch_filter_config_t fc = {};
  fc.max_glitch_ns = 1000;
  pcnt_unit_set_glitch_filter(knob_unit, &fc);

  pcnt_chan_config_t ca = {};
  ca.edge_gpio_num  = PIN_KNOB_A;
  ca.level_gpio_num = PIN_KNOB_B;
  pcnt_channel_handle_t cha = NULL;
  pcnt_new_channel(knob_unit, &ca, &cha);

  pcnt_chan_config_t cb = {};
  cb.edge_gpio_num  = PIN_KNOB_B;
  cb.level_gpio_num = PIN_KNOB_A;
  pcnt_channel_handle_t chb = NULL;
  pcnt_new_channel(knob_unit, &cb, &chb);

  pcnt_channel_set_edge_action(cha, PCNT_CHANNEL_EDGE_ACTION_DECREASE,
                                    PCNT_CHANNEL_EDGE_ACTION_INCREASE);
  pcnt_channel_set_level_action(cha, PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                     PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
  pcnt_channel_set_edge_action(chb, PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                                    PCNT_CHANNEL_EDGE_ACTION_DECREASE);
  pcnt_channel_set_level_action(chb, PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                     PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

  /* Mit accum_count zaehlt der Treiber ueber die Grenzen hinweg weiter,
     sobald dort Beobachtungspunkte liegen.                         */
  pcnt_unit_add_watch_point(knob_unit, uc.low_limit);
  pcnt_unit_add_watch_point(knob_unit, uc.high_limit);

  /* Der Pulszaehler stellt die Pins als Eingang ein, die Pull-ups
     kommen danach.                                                 */
  gpio_pullup_en((gpio_num_t)PIN_KNOB_A);
  gpio_pullup_en((gpio_num_t)PIN_KNOB_B);

  pcnt_unit_enable(knob_unit);
  pcnt_unit_clear_count(knob_unit);
  pcnt_unit_start(knob_unit);
  knob_base = 0;
}

static int knob_steps(void)
{
  if (!knob_unit) return 0;
  int c = 0;
  if (pcnt_unit_get_count(knob_unit, &c) != ESP_OK) return 0;
  int steps = (c - knob_base) / KNOB_EDGES_PER_DETENT;
  knob_base += steps * KNOB_EDGES_PER_DETENT;
  return steps;
}

/* ---- USB-Mediatasten ---------------------------------------------------
   HID "Consumer Control": laeuft ohne Treiber auf macOS, Windows,
   Linux, iPad und Android. Ist kein Rechner dran (nur Netzteil),
   wird nichts gesendet.                                             */
#if HAVE_USB_MEDIA
static USBHIDConsumerControl media_hid;
#endif

static void media_begin(void)
{
#if HAVE_USB_MEDIA
  media_hid.begin();
  USB.begin();
#endif
}

static void media_key(uint16_t usage)
{
#if HAVE_USB_MEDIA
  if (!USB) return;                     /* nicht am Rechner */
  media_hid.press(usage);
  media_hid.release();
#else
  (void)usage;
#endif
}

/* HID-Codes aus der Consumer-Page. Stehen hier, damit der Rest auch
   ohne TinyUSB-Header baut.                                          */
#define MEDIA_NEXT      0x00B5
#define MEDIA_PREV      0x00B6
#define MEDIA_PLAY      0x00CD
#define MEDIA_MUTE      0x00E2
#define MEDIA_VOL_UP    0x00E9
#define MEDIA_VOL_DOWN  0x00EA

/* ---- Einstieg ---------------------------------------------------------- */

static void hw_begin(void)
{
  for (int k = 0; k < KEY_COUNT; k++) {
#if !USE_CONTROLS
    if (k != KEY_PWR) continue;
#endif
    pinMode(hw_keys[k].pin, INPUT_PULLUP);
    hw_keys[k].raw = hw_keys[k].down = false;
  }
#if USE_CONTROLS
  knob_begin();
#endif
  media_begin();

  Serial.printf("[hw] Bedienteile %s, USB-Mediatasten %s\n",
                USE_CONTROLS ? "an" : "aus",
                HAVE_USB_MEDIA ? "an" : (USE_USB_MEDIA ? "aus (USB-Modus: Hardware-CDC)" : "aus"));
}

/* Aus loop(). Entprellt die Tasten und liefert, was seit dem letzten
   Aufruf passiert ist.                                              */
static hw_events_t hw_poll(void)
{
  hw_events_t ev = { 0, 0, 0 };
  uint32_t now = millis();

  for (int k = 0; k < KEY_COUNT; k++) {
#if !USE_CONTROLS
    if (k != KEY_PWR) continue;
#endif
    hw_key_t &key = hw_keys[k];
    bool r = digitalRead(key.pin) == LOW;
    if (r != key.raw) { key.raw = r; key.raw_t = now; continue; }
    if (r == key.down || now - key.raw_t < KEY_STABLE_MS) {
      if (key.down && !key.held && now - key.down_t >= KEY_HOLD_MS) {
        key.held = true;
        ev.hold |= KEY_BIT(k);
      }
      continue;
    }

    key.down = r;
    if (r) { key.down_t = now; key.held = false; }
    else if (!key.held) ev.click |= KEY_BIT(k);
  }

#if USE_CONTROLS
  ev.steps = knob_steps();
#endif
  return ev;
}

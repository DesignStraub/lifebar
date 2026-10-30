#pragma once
#include "lvgl_port.h"
#include "lifebar_config.h"
#include "lifebar_hw.h"
#include "ui_lifebar.h"

/* =====================================================================
   LifeBar - Bedienteile

   Verbindet Tasten und Drehknopf mit Bildschirm, Buchung und den
   USB-Mediatasten. Laeuft in loop().

   PWR (Rueckseite)   kurz: Bildschirm an/aus
   Bildschirm-Taste   dasselbe, aussen am Deck
   Drehknopf         Lautstaerke - oder, solange eine Spalte
                      ausgewaehlt ist, buchen (eine Rastung = ein Schritt)
   Knopf druecken     Auswahl aufheben, ohne Auswahl stumm
   Zurueck/Play/Vor   Medien

   Medientasten und Lautstaerke wecken den Bildschirm nicht - sie
   haben mit dem Display nichts zu tun und gehen auch, wenn es aus ist.
   ===================================================================== */

static void screen_set(bool on)
{
  if (on == screen_on) return;
  screen_on = on;
  backlight_power(on);
  Serial.printf("[screen] %s\n", on ? "an" : "aus");
}

/* Den Abfangschirm dem Zustand nachfuehren. Steht in jedem Durchlauf,
   statt nur beim Umschalten: bekommt loop() den Mutex einmal nicht,
   klappt es im naechsten.                                          */
static void screen_sync(void)
{
  bool blocked = (dark_blocker != NULL);
  if (blocked != screen_on) return;            /* passt schon */
  if (!lvgl_lock(50)) return;
  if (!screen_on)               ui_screen_dark(true);
  else if (ui_touch_released()) ui_screen_dark(false);
  lvgl_unlock();
}

static void controls_tick(void)
{
  hw_events_t ev = hw_poll();

  int8_t req = screen_req;
  if (req != SCREEN_REQ_NONE) {
    screen_req = SCREEN_REQ_NONE;
    screen_set(req != 0);
  }
  if (ev.click & KEY_BIT(KEY_PWR)) screen_set(!screen_on);
  screen_sync();

#if USE_CONTROLS
  if (ev.click & KEY_BIT(KEY_PREV)) media_key(MEDIA_PREV);
  if (ev.click & KEY_BIT(KEY_PLAY)) media_key(MEDIA_PLAY);
  if (ev.click & KEY_BIT(KEY_NEXT)) media_key(MEDIA_NEXT);

  if (ev.steps) {
    bool booked = false;
    if (screen_on && lvgl_lock(100)) {
      booked = ui_knob_turn(ev.steps);
      lvgl_unlock();
    }
    if (!booked) {
      uint16_t k = ev.steps > 0 ? MEDIA_VOL_UP : MEDIA_VOL_DOWN;
      for (int n = abs(ev.steps); n > 0; n--) media_key(k);
    }
  }

  if (ev.click & KEY_BIT(KEY_KNOB)) {
    bool used = false;
    if (screen_on && lvgl_lock(100)) {
      used = ui_knob_press();
      lvgl_unlock();
    }
    if (!used) media_key(MEDIA_MUTE);
  }
#endif
}

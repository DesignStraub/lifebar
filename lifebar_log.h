#pragma once
#include <Arduino.h>
#include <FFat.h>
#include "lifebar_model.h"

/* =====================================================================
   LifeBar - Ereignisprotokoll

   Die Summen im NVS sagen WIE VIEL, aber nicht WANN. Dieses Log haelt
   jede einzelne Eingabe fest - Grundlage fuer Tagesgrenzen, Muster
   und den Abgleich mit einer Auswertung auf dem Rechner.

   Ein Eintrag = 8 Byte. Zehn Eingaben taeglich ueber zehn Jahre sind
   rund 290 KB. Angehaengt wird nur, nie umgeschrieben: das ist
   absturzsicher und schont den Flash.
   ===================================================================== */

#define LOG_PATH     "/events.bin"
#define LOG_MAX_BYTE (8UL * 1024UL * 1024UL)  /* 8 MB, ~1 Mio Eintraege */

#define EV_SKILL 0
#define EV_BAD   1

/* Solange die Uhr unbekannt ist, steht in ts die Betriebssekunde mit
   diesem Bit. Sobald NTP durch ist, werden die Eintraege nachdatiert. */
#define TS_REL   0x80000000UL
#define OFF_NONE 0xFFFFFFFFUL

typedef struct __attribute__((packed)) {
  uint32_t ts;      /* Unixzeit, 0 = Uhr war unbekannt   */
  uint8_t  type;    /* EV_SKILL oder EV_BAD              */
  uint8_t  idx;     /* Preset-Index                      */
  int16_t  delta;   /* Minuten bzw. Anzahl, auch negativ */
} lb_event_t;

static bool     log_ready = false;
static uint32_t log_count = 0;
static char     log_error[48] = "";
static uint32_t unresolved_off = OFF_NONE;   /* erster undatierter Eintrag */

/* Ereignisse werden im UI-Task nur eingereiht und aus loop()
   geschrieben. Ein Flash-Write im Touch-Callback blockiert sonst das
   Zeichnen und laeuft auf dem knappen LVGL-Stack.                  */
#define LOG_QUEUE 32
static lb_event_t log_q[LOG_QUEUE];
static volatile uint8_t log_q_head = 0;   /* schreibt der UI-Task  */
static volatile uint8_t log_q_tail = 0;   /* liest der loop()-Task */

static void log_begin(void)
{
  if (!FFat.begin(true)) {          /* true = bei Bedarf formatieren */
    snprintf(log_error, sizeof(log_error), "FFat Mount fehlgeschlagen");
    Serial.println("[log] FEHLER: FFat nicht verfuegbar.");
    Serial.println("[log] Partitionsschema ohne FATFS-Anteil?");
    return;
  }
  log_ready = true;

  File f = FFat.open(LOG_PATH, "r");
  if (f) { log_count = f.size() / sizeof(lb_event_t); f.close(); }

  Serial.printf("[log] bereit, %lu Eintraege, FS %u/%u Byte belegt\n",
                (unsigned long)log_count,
                (unsigned)FFat.usedBytes(),
                (unsigned)FFat.totalBytes());
}

/* Aus dem UI-Task: nur einreihen, nichts schreiben. */
static void log_add(uint8_t type, uint8_t idx, int16_t delta)
{
  uint8_t next = (log_q_head + 1) % LOG_QUEUE;
  if (next == log_q_tail) return;        /* Puffer voll, aeltestes gewinnt */

  log_q[log_q_head].ts    = time_valid ? (uint32_t)time(NULL)
                                       : (TS_REL | (millis() / 1000));
  log_q[log_q_head].type  = type;
  log_q[log_q_head].idx   = idx;
  log_q[log_q_head].delta = delta;
  log_q_head = next;
}

/* Wie log_add, aber mit vorgegebenem Zeitpunkt - fuers Nachtragen. */
static void log_add_at(uint32_t ts, uint8_t type, uint8_t idx, int16_t delta)
{
  uint8_t next = (log_q_head + 1) % LOG_QUEUE;
  if (next == log_q_tail) return;

  log_q[log_q_head].ts    = ts;
  log_q[log_q_head].type  = type;
  log_q[log_q_head].idx   = idx;
  log_q[log_q_head].delta = delta;
  log_q_head = next;
}

/* Aus loop(): alles Aufgelaufene in einem Rutsch anhaengen. */
static void log_tick(void)
{
  if (log_q_head == log_q_tail) return;

  if (!log_ready) {                      /* sonst laeuft der Puffer voll */
    log_q_tail = log_q_head;
    return;
  }

  File f = FFat.open(LOG_PATH, "a");
  if (!f) {
    snprintf(log_error, sizeof(log_error), "Datei nicht zu oeffnen");
    Serial.println("[log] FEHLER: events.bin nicht zu oeffnen");
    log_q_tail = log_q_head;
    return;
  }

  int n = 0;
  while (log_q_tail != log_q_head) {
    if (f.size() < LOG_MAX_BYTE) {
      if ((log_q[log_q_tail].ts & TS_REL) && unresolved_off == OFF_NONE)
        unresolved_off = f.size();       /* Startpunkt fuers Nachdatieren */
      f.write((uint8_t *)&log_q[log_q_tail], sizeof(lb_event_t));
      log_count++;
      n++;
    }
    log_q_tail = (log_q_tail + 1) % LOG_QUEUE;
  }
  f.close();
}

/* Wird aufgerufen, sobald die Uhr steht: alle relativ protokollierten
   Eintraege dieser Sitzung auf echte Zeitstempel umschreiben.       */
static void log_fix_times(void)
{
  if (!log_ready || unresolved_off == OFF_NONE) return;

  uint32_t now = (uint32_t)time(NULL);
  uint32_t up  = millis() / 1000;

  File f = FFat.open(LOG_PATH, "r+");
  if (!f) { unresolved_off = OFF_NONE; return; }

  uint32_t   off = unresolved_off;
  lb_event_t e;
  int        n = 0;

  f.seek(off);
  while (f.read((uint8_t *)&e, sizeof(e)) == (int)sizeof(e)) {
    if (e.ts & TS_REL) {
      uint32_t rel = e.ts & ~TS_REL;
      e.ts = now - (up - rel);           /* wie lange ist das her */
      f.seek(off);
      f.write((uint8_t *)&e, sizeof(e));
      n++;
    }
    off += sizeof(e);
    f.seek(off);
  }
  f.close();

  unresolved_off = OFF_NONE;
  if (n) Serial.printf("[log] %d Eintraege nachdatiert\n", n);
}

static const char *log_name(uint8_t type, uint8_t idx)
{
  /* Immer Englisch: das CSV soll sich nicht aendern, wenn jemand die
     Sprache umstellt.                                              */
  if (type == EV_SKILL)
    return (idx < PRESET_COUNT) ? preset[idx].name[LANG_EN] : "?";
  return (idx < BAD_PRESET_COUNT) ? bad_preset[idx].name[LANG_EN] : "?";
}

static void log_clear(void)
{
  log_q_tail = log_q_head;       /* Wartende Eintraege verwerfen */
  if (!log_ready) return;
  FFat.remove(LOG_PATH);
  log_count = 0;
  unresolved_off = OFF_NONE;
  Serial.println("[log] geleert");
}

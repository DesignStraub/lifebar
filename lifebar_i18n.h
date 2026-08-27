#pragma once
#include <Arduino.h>

/* =====================================================================
   LifeBar - Sprachen

   Eine weitere Sprache ist eine weitere Spalte in jeder Zeile und ein
   Eintrag in LANG_NAME. Sonst aendert sich nichts.
   ===================================================================== */

enum { LANG_EN = 0, LANG_DE = 1, LANG_COUNT };

static const char *LANG_NAME[LANG_COUNT] = { "ENGLISH", "DEUTSCH" };
static const char *LANG_TAG[LANG_COUNT]  = { "en",      "de"      };

/* Deutsche Titel werden zusammengeschrieben, englische getrennt:
   WALDKRIEGER gegen WILD WARRIOR.                                  */
static const char *TITLE_SEP[LANG_COUNT] = { " ",       ""        };

static uint8_t lang = LANG_EN;      /* Standard ist Englisch */

enum {
  S_SETTINGS = 0, S_WEEKGOAL, S_NIGHTMODE, S_ON, S_OFF, S_WIFI,
  S_SKILLS, S_VICES, S_RESET, S_LANGUAGE, S_TIMEFMT, S_BRIGHT,
  S_PICK_SKILLS, S_PICK_VICES, S_PICK_LANG, S_NEXT, S_DONE,
  S_WIFI_SETUP, S_STEP1, S_STEP2, S_STEP3, S_LATER,
  S_NO_VICES, S_SET_NOW,
  S_THIS_WEEK, S_LAST_WEEK, S_BONUS, S_LAST_BONUS, S_NOCHANGE,
  S_NO_CLOCK, S_NO_TIME_HINT,
  S_STARTING, S_RESTART, S_NIGHT_ON, S_NIGHT_OFF, S_NEW_TITLE, S_GAP, S_ASK_WIFI, S_ASK_RESET, S_YES, S_CANCEL,
  S_COUNT
};

static const char *STR[S_COUNT][LANG_COUNT] = {
/* S_SETTINGS    */ { "SETTINGS",          "EINSTELLUNGEN" },
/* S_WEEKGOAL    */ { "WEEKLY GOAL",       "WOCHENZIEL" },
/* S_NIGHTMODE   */ { "NIGHT MODE",        "NACHTMODUS" },
/* S_ON          */ { "ON",                "AN" },
/* S_OFF         */ { "OFF",               "AUS" },
/* S_WIFI        */ { "WIFI",              "WLAN" },
/* S_SKILLS      */ { "SKILLS",            "SKILLS" },
/* S_VICES       */ { "VICES",             "LASTER" },
/* S_RESET       */ { "RESET (hold)",      "RESET (lang)" },
/* S_LANGUAGE    */ { "LANGUAGE",          "SPRACHE" },
/* S_TIMEFMT     */ { "TIME FORMAT",       "ZEITFORMAT" },
/* S_BRIGHT      */ { "BRIGHTNESS",        "HELLIGKEIT" },
/* S_PICK_SKILLS */ { "WHAT DO YOU WANT TO LEVEL",
                      "WAS WILLST DU SKILLEN" },
/* S_PICK_VICES  */ { "WHAT DO YOU WANT TO CUT DOWN",
                      "WAS WILLST DU REDUZIEREN" },
/* S_PICK_LANG   */ { "CHOOSE YOUR LANGUAGE",
                      "WAEHLE DEINE SPRACHE" },
/* S_NEXT        */ { "NEXT",              "WEITER" },
/* S_DONE        */ { "DONE",              "FERTIG" },
/* S_WIFI_SETUP  */ { "WIFI SETUP",        "WLAN EINRICHTEN" },
/* S_STEP1       */ { "1.  On your phone, join this network:",
                      "1.  Am Handy mit diesem Netz verbinden:" },
/* S_STEP2       */ { "2.  The page opens by itself. If not: 192.168.4.1",
                      "2.  Die Seite oeffnet sich von selbst. Falls nicht: 192.168.4.1" },
/* S_STEP3       */ { "3.  Pick your network, type the password on the phone.",
                      "3.  Netz waehlen, Passwort am Handy eingeben." },
/* S_LATER       */ { "LATER",             "SPAETER" },
/* S_NO_VICES    */ { "No vices set up yet",
                      "Noch keine Laster festgelegt" },
/* S_SET_NOW     */ { "SET THEM UP",       "JETZT FESTLEGEN" },
/* S_THIS_WEEK   */ { "THIS WEEK  %s      LAST  %s",
                      "DIESE WOCHE  %s      LETZTE  %s" },
/* S_LAST_WEEK   */ { "LAST",              "LETZTE" },
/* S_BONUS       */ { "BONUS +%lu EXP",    "BONUS +%lu EXP" },
/* S_LAST_BONUS  */ { "last week +%lu EXP","letzte Woche +%lu EXP" },
/* S_NOCHANGE    */ { "+/- 0",             "+/- 0" },
/* S_NO_CLOCK    */ { "No clock. Swipe up and set up WiFi.",
                      "Keine Zeit. Wischen nach oben, WLAN einrichten." },
/* S_NO_TIME_HINT*/ { "!  NO CLOCK",       "!  OHNE UHR" },
/* S_STARTING    */ { "starting ...",      "startet ..." },
/* S_RESTART     */ { "RESTART",           "NEUSTART" },
/* S_NIGHT_ON    */ { "enabling night mode",
                      "Nachtmodus wird aktiviert" },
/* S_NIGHT_OFF   */ { "disabling night mode",
                      "Nachtmodus wird deaktiviert" },
/* S_NEW_TITLE   */ { "NEW TITLE",         "NEUER TITEL" },
/* S_GAP         */ { "%lu weeks without entries",
                      "%lu Wochen ohne Eintrag" },
/* S_ASK_WIFI    */ { "Discard WiFi credentials and open the hotspot?",
                      "WLAN-Zugang verwerfen und Hotspot oeffnen?" },
/* S_ASK_RESET   */ { "Erase all data? Hours, log and settings are lost.",
                      "Alle Daten loeschen? Stunden, Protokoll und "
                      "Einstellungen sind weg." },
/* S_YES         */ { "YES",               "JA" },
/* S_CANCEL      */ { "CANCEL",            "ABBRECHEN" }
};

#define T(id) (STR[id][lang])

/* Leitwort des Titels. Wird wochenweise neu bestimmt. */
enum { LEAD_NONE = 0, LEAD_REFLECT, LEAD_FOCUS, LEAD_BROAD,
       LEAD_NIGHT, LEAD_MORNING, LEAD_MIDDAY, LEAD_EVENING, LEAD_COUNT };

static const char *LEAD[LEAD_COUNT][LANG_COUNT] = {
  { "",          ""                },
  { "MINDFUL",   "REFLEKTIERTER"   },
  { "DEVOTED",   "ERGEBENER"       },
  { "VERSATILE", "VIELSEITIGER"    },
  { "NIGHTLY",   "NAECHTLICHER"    },
  { "MORNING",   "MORGENDLICHER"   },
  { "MIDDAY",    "MITTAEGLICHER"   },
  { "EVENING",   "ABENDLICHER"     }
};

static const char *WEEKDAY[7][LANG_COUNT] = {
  { "MONDAY",    "MONTAG"     },
  { "TUESDAY",   "DIENSTAG"   },
  { "WEDNESDAY", "MITTWOCH"   },
  { "THURSDAY",  "DONNERSTAG" },
  { "FRIDAY",    "FREITAG"    },
  { "SATURDAY",  "SAMSTAG"    },
  { "SUNDAY",    "SONNTAG"    }
};

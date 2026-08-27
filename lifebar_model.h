#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <time.h>
#include <string.h>
#include "lifebar_i18n.h"

/* =====================================================================
   LifeBar - Datenmodell  (V2)
   Alles in Minuten, immer Integer. Ein Schritt = 30 Minuten.
   Keine Umlaute im ganzen Projekt: die LVGL-Standardschrift
   (Montserrat 14) enthaelt nur ASCII.
   ===================================================================== */

#define MOD_COUNT    6
#define STEP_MIN     30
#define SAVE_DELAY   3000UL     /* ms Ruhe vor dem NVS-Write            */
#define CLASS_LEVEL   3         /* ab hier wird eine Klasse vergeben    */
/* Der Titel waechst mit dem Level, seine Woerter werden aber
   wochenweise neu bestimmt - Langzeitfortschritt und kurzfristige
   Rueckmeldung getrennt.                                            */
#define TITLE_LVL1    3         /* Nomen - vorher ist man Novize        */
#define TITLE_LVL2   10         /* + Bestimmungswort                    */
#define TITLE_LVL3   20         /* + Leitwort                           */
#define COMBO_SHARE  25         /* Zweitplatzierter braucht 25 % davon  */
#define REFLECT_MIN  10         /* Laster-Eintraege fuer "reflektiert"  */
#define SLOT_EMPTY   0xFF
#define WEEK_HIST    26        /* halbes Jahr Wochenhistorie */
#define WEEK_GOAL_DEF 600      /* 10 h pro Woche als Startziel */

/* ---- Presets ---------------------------------------------------------
   Jedes Modul bringt seinen Klassentitel gleich mit. Die Klasse wird
   nicht gewaehlt, sondern ergibt sich aus der Verteilung der Minuten. */

typedef struct {
  const char *name[LANG_COUNT];
  const char *title[3][LANG_COUNT];   /* Stufe 1-3, waechst mit dem Level */
  const char *pre[LANG_COUNT];   /* Bestimmungswort fuer Kombititel */
  const char *icon;   /* UTF-8 Emoji, nur genutzt wenn LB_EMOJI 1 */
} preset_t;

static const preset_t preset[] = {
 {{"GYM","GYM"},{{"WARRIOR","KRIEGER"},{"BERSERKER","BERSERKER"},{"WARLORD","KRIEGSHERR"}},{"IRON","EISEN"},"\xF0\x9F\x92\xAA"},
 {{"RUNNING","LAUFEN"},{{"RUNNER","LAEUFER"},{"SPRINTER","SPURTER"},{"STORMRUNNER","STURMLAEUFER"}},{"SWIFT","WIND"},"\xF0\x9F\x8F\x83"},
 {{"YOGA","YOGA"},{{"MONK","MOENCH"},{"ADEPT","ADEPT"},{"MASTER","MEISTER"}},{"CALM","STILLE"},"\xF0\x9F\xA7\x98"},
 {{"CYCLING","RAD"},{{"COURIER","KURIER"},{"RIDER","EILBOTE"},{"ROADMASTER","WEGMEISTER"}},{"ROAD","WEG"},"\xF0\x9F\x9A\xB4"},
 {{"STUDY","LERNEN"},{{"SCHOLAR","GELEHRTER"},{"MAGISTER","MAGISTER"},{"ARCHSCHOLAR","ERZGELEHRTER"}},{"RUNE","RUNEN"},"\xF0\x9F\x8E\x93"},
 {{"READING","LESEN"},{{"READER","LESER"},{"BOOKWORM","BUECHERWURM"},{"LOREMASTER","LOREMEISTER"}},{"BOOK","BUCH"},"\xF0\x9F\x93\x96"},
 {{"LANGUAGE","SPRACHE"},{{"LINGUIST","LINGUIST"},{"POLYGLOT","POLYGLOTT"},{"BABELMASTER","BABELMEISTER"}},{"TONGUE","ZUNGEN"},"\xF0\x9F\x97\xA3"},
 {{"CODE","CODE"},{{"MAGE","MAGIER"},{"ARCHMAGE","ERZMAGIER"},{"ARCHITECT","ARCHITEKT"}},{"CIPHER","CODE"},"\xF0\x9F\x92\xBB"},
 {{"MUSIC","MUSIK"},{{"BARD","BARDE"},{"MINSTREL","SPIELMANN"},{"VIRTUOSO","VIRTUOSE"}},{"SONG","KLANG"},"\xF0\x9F\x8E\xB8"},
 {{"PAINTING","MALEN"},{{"ARTIST","KUENSTLER"},{"MAESTRO","MAESTRO"},{"VISIONARY","VISIONAER"}},{"COLOR","FARB"},"\xF0\x9F\x8E\xA8"},
 {{"WRITING","SCHREIBEN"},{{"CHRONICLER","CHRONIST"},{"POET","POET"},{"MYTHWEAVER","MYTHENWEBER"}},{"QUILL","FEDER"},"\xE2\x9C\x8D"},
 {{"CRAFTING","BASTELN"},{{"SMITH","SCHMIED"},{"MASTERSMITH","MEISTERSCHMIED"},{"FORGEMASTER","SCHMIEDEHERR"}},{"HAMMER","HAMMER"},"\xF0\x9F\x94\xA8"},
 {{"MEDITATION","MEDITATION"},{{"ASCETIC","ASKET"},{"HERMIT","EREMIT"},{"ENLIGHTENED","ERLEUCHTETER"}},{"BREATH","ATEM"},"\xF0\x9F\x95\xAF"},
 {{"COOKING","KOCHEN"},{{"COOK","KOCH"},{"CHEF","KUECHENCHEF"},{"KITCHENMASTER","KUECHENMEISTER"}},{"HEARTH","HERD"},"\xF0\x9F\x8D\xB3"},
 {{"NATURE","NATUR"},{{"RANGER","WALDLAEUFER"},{"TRACKER","FAEHRTENLESER"},{"WILDWARDEN","WILDHUETER"}},{"WILD","WALD"},"\xF0\x9F\x8C\xB2"},
 {{"FRIENDS","FREUNDE"},{{"COMPANION","GEFAEHRTE"},{"DIPLOMAT","DIPLOMAT"},{"CHIEFTAIN","HAEUPTLING"}},{"TAVERN","TAFEL"},"\xF0\x9F\x8D\xBB"},
 {{"GARDEN","GARTEN"},{{"GARDENER","GAERTNER"},{"CULTIVATOR","ZUECHTER"},{"GREENSAGE","GRUENMEISTER"}},{"BLOOM","BLUETEN"},"\xF0\x9F\x8C\xB1"},
 {{"CLEANING","PUTZEN"},{{"STEWARD","VERWALTER"},{"CARETAKER","HAUSMEISTER"},{"ORDERKEEPER","ORDNUNGSMEISTER"}},{"ORDER","ORDNUNGS"},"\xF0\x9F\xA7\xB9"}
};
#define PRESET_COUNT ((int)(sizeof(preset) / sizeof(preset[0])))

/* Bad Habits: gezaehlt wird, WIE OFT - nicht wie lange.
   Sie beeinflussen die EXP bewusst nicht.                          */
#define BAD_COUNT 6

static const preset_t bad_preset[] = {
  {{"SOCIAL",    "SOCIAL"    },{{"",""},{"",""},{"",""}},{"",""},"\xF0\x9F\x93\xB1"},
  {{"STREAMING", "STREAMING" },{{"",""},{"",""},{"",""}},{"",""},"\xF0\x9F\x93\xBA"},
  {{"GAMING",    "ZOCKEN"    },{{"",""},{"",""},{"",""}},{"",""},"\xF0\x9F\x8E\xAE"},
  {{"SMOKING",   "RAUCHEN"   },{{"",""},{"",""},{"",""}},{"",""},"\xF0\x9F\x9A\xAC"},
  {{"ALCOHOL",   "ALKOHOL"   },{{"",""},{"",""},{"",""}},{"",""},"\xF0\x9F\x8D\xBA"},
  {{"CAFFEINE",  "KOFFEIN"   },{{"",""},{"",""},{"",""}},"\xE2\x98\x95"    },
  {{"SUGAR",     "ZUCKER"    },{{"",""},{"",""},{"",""}},{"",""},"\xF0\x9F\x8D\xAC"},
  {{"FASTFOOD",  "FASTFOOD"  },{{"",""},{"",""},{"",""}},{"",""},"\xF0\x9F\x8D\x94"},
  {{"SHOPPING",  "SHOPPING"  },{{"",""},{"",""},{"",""}},{"",""},"\xF0\x9F\x9B\x92"},
  {{"PORN",      "PORNO"     },{{"",""},{"",""},{"",""}},{"",""},"\xF0\x9F\x94\x9E"},
  {{"NAILS",     "NAEGEL"    },{{"",""},{"",""},{"",""}},{"",""},"\xF0\x9F\x92\x85"},
  {{"RUMINATING","GRUEBELN"  },{{"",""},{"",""},{"",""}},{"",""},"\xF0\x9F\x8C\x80"}
};
#define BAD_PRESET_COUNT ((int)(sizeof(bad_preset) / sizeof(bad_preset[0])))

/* Fallback, falls im Wizard nichts gewaehlt wird */
static const uint8_t default_slot[MOD_COUNT] =
  { 0, 4, 8, 5, SLOT_EMPTY, SLOT_EMPTY };

/* ---- Zustand --------------------------------------------------------- */

/* Vorgaengerformat ohne Titelbausteine. */
typedef struct {
  uint8_t  version;
  uint8_t  configured;
  uint8_t  brightness;
  uint8_t  net_skipped;
  uint8_t  slot[MOD_COUNT];
  uint8_t  bad_slot[BAD_COUNT];
  uint32_t minutes[PRESET_COUNT];
  uint16_t week_min[PRESET_COUNT];
  uint16_t prev_week[PRESET_COUNT];
  uint32_t bad_total[BAD_PRESET_COUNT];
  uint16_t bad_week[BAD_PRESET_COUNT];
  uint16_t bad_prev[BAD_PRESET_COUNT];
  uint32_t last_seen;
  uint32_t week_start;
  uint16_t week_goal;
  uint16_t week_hist[WEEK_HIST];
  uint8_t  week_hist_pos;
  uint8_t  seals;
  uint8_t  review_pending;
  uint8_t  pad;
} lb_state_v5_t;

typedef struct {
  uint8_t  version;              /* aktuell 6                           */
  uint8_t  configured;
  uint8_t  brightness;           /* ungenutzt, haelt das Format stabil  */
  uint8_t  net_skipped;

  uint8_t  slot[MOD_COUNT];
  uint8_t  bad_slot[BAD_COUNT];

  uint32_t minutes[PRESET_COUNT];
  uint16_t week_min[PRESET_COUNT];
  uint16_t prev_week[PRESET_COUNT];

  uint32_t bad_total[BAD_PRESET_COUNT];
  uint16_t bad_week[BAD_PRESET_COUNT];
  uint16_t bad_prev[BAD_PRESET_COUNT];

  uint32_t last_seen;
  uint32_t week_start;
  uint16_t week_goal;
  uint16_t week_hist[WEEK_HIST];
  uint8_t  week_hist_pos;
  uint8_t  seals;
  uint8_t  review_pending;
  uint8_t  t_level;              /* letztes bekanntes Level */

  /* --- Titelbausteine ---
     hour_w und rec_min werden beim Wochenwechsel halbiert. Damit
     zaehlt die jüngste Vergangenheit mehr, ohne dass wir mehrere
     Wochen einzeln speichern muessen.                              */
  uint16_t hour_w[24];               /* Buchungen je Tagesstunde      */
  uint16_t rec_min[PRESET_COUNT];    /* gewichtete Minuten je Skill   */
  uint8_t  t_noun;                   /* Preset des Nomens             */
  uint8_t  t_pre;                    /* Preset des Bestimmungsworts   */
  uint8_t  t_lead;                   /* LEAD_*                        */
  uint8_t  t_valid;                  /* 0 = noch nie ausgewertet      */
  uint8_t  t_hist[WEEK_HIST][3];     /* Chronik: noun, pre, lead      */
} lb_state_t;

static lb_state_t st;
static uint32_t   total_min = 0;

/* ---- Zugriff auf die belegten Slots ---------------------------------- */

/* NVS-Handle. Steht bewusst weit oben: week_tick greift darauf zu
   und ist im File frueher definiert als der Persistenz-Abschnitt. */
static Preferences prefs;

/* Steht hier statt im Netzwerkteil: das Log braucht es ebenfalls,
   und beide inkludieren dieses Modell.                            */
static bool time_valid = false;

/* Uebersprungene Wochen seit dem letzten Betrieb. Liegt in einem
   eigenen NVS-Schluessel, damit das Struct unveraendert bleibt.    */
static uint8_t  week_gap   = 0;
static volatile bool gap_dirty = false;

/* Anzeigeeinstellung, steht hier weil auch der Webserver sie braucht
   und lifebar_net.h vor ui_lifebar.h eingebunden wird.             */
static bool clock12 = false;      /* 12-Stunden-Anzeige mit AM/PM */

/* Bonus-EXP aus uebertroffenen Wochenzielen.
   Liegt bewusst in eigenen NVS-Schluesseln, damit das Struct und
   damit das Speicherformat unveraendert bleibt.                    */
static uint32_t bonus_min  = 0;   /* insgesamt gutgeschrieben */
static uint32_t last_bonus = 0;   /* aus der letzten Woche    */

static bool mod_active(int i)
{
  return st.slot[i] != SLOT_EMPTY;
}

static int mod_active_count(void)
{
  int n = 0;
  for (int i = 0; i < MOD_COUNT; i++) if (mod_active(i)) n++;
  return n;
}

static const char *mod_name(int i)
{
  return mod_active(i) ? preset[st.slot[i]].name[lang] : "-";
}

static const char *mod_icon(int i)
{
  return mod_active(i) ? preset[st.slot[i]].icon : "";
}

static bool bad_active(int i)
{
  return st.bad_slot[i] != SLOT_EMPTY;
}

static int bad_active_count(void)
{
  int n = 0;
  for (int i = 0; i < BAD_COUNT; i++) if (bad_active(i)) n++;
  return n;
}

static const char *bad_name(int i)
{
  return bad_active(i) ? bad_preset[st.bad_slot[i]].name[lang] : "-";
}

static const char *bad_icon(int i)
{
  return bad_active(i) ? bad_preset[st.bad_slot[i]].icon : "";
}

/* Zugriff ueber den Slot, gespeichert wird beim Preset. */
static uint32_t *mod_min_p(int i)  { return &st.minutes[st.slot[i]];   }
static uint16_t *mod_week_p(int i) { return &st.week_min[st.slot[i]];  }
static uint16_t *mod_prev_p(int i) { return &st.prev_week[st.slot[i]]; }

static uint32_t *bad_tot_p(int i)  { return &st.bad_total[st.bad_slot[i]]; }
static uint16_t *bad_week_p(int i) { return &st.bad_week[st.bad_slot[i]];  }
static uint16_t *bad_prev_p(int i) { return &st.bad_prev[st.bad_slot[i]];  }

/* Preset anhand des Namens finden - fuer Import und Nachtragen.
   Akzeptiert beide Sprachen, damit ein Export in Deutsch auch
   eingelesen wird, wenn das Geraet auf Englisch steht.            */
static int preset_by_name(const char *n)
{
  for (int i = 0; i < PRESET_COUNT; i++)
    for (int l = 0; l < LANG_COUNT; l++)
      if (!strcasecmp(n, preset[i].name[l])) return i;
  return -1;
}

static int bad_by_name(const char *n)
{
  for (int i = 0; i < BAD_PRESET_COUNT; i++)
    for (int l = 0; l < LANG_COUNT; l++)
      if (!strcasecmp(n, bad_preset[i].name[l])) return i;
  return -1;
}

static void recalc_total(void)
{
  /* Bewusst ueber alle Presets: wer ein Habit abwaehlt, soll kein
     Level verlieren. Angezeigt wird trotzdem nur das Aktive.      */
  total_min = 0;
  for (int i = 0; i < PRESET_COUNT; i++) total_min += st.minutes[i];
  total_min += bonus_min;
}

/* ---- Levelkurve ------------------------------------------------------
   Kosten fuer Level n -> n+1 = 20 + 2,6 * n^2, ganzzahlig gerechnet.

   Als Formel statt Tabelle: damit gibt es kein Maximum. Level 100
   liegt bei rund 14.000 Stunden, das erreicht niemand - aber die
   Leiste bleibt bis dahin in Bewegung.

   Quadratisch heisst vorne billig, hinten steil: Level 2 nach 23
   Minuten, Level 10 nach 15 Stunden, Level 20 nach 113 Stunden.   */

#define LEVEL_SANITY 999      /* nur als Schleifenbremse */

static uint32_t level_cost_of(uint32_t n)
{
  return 20UL + (26UL * n * n + 5UL) / 10UL;
}

static void level_from_total(uint32_t total, uint16_t *lvl,
                             uint32_t *have, uint32_t *need)
{
  uint32_t n    = 1;
  uint32_t rest = total;
  uint32_t c    = level_cost_of(1);

  while (rest >= c && n < LEVEL_SANITY) {
    rest -= c;
    n++;
    c = level_cost_of(n);
  }
  *lvl  = (uint16_t)n;
  *have = rest;
  *need = c;
}

static uint16_t current_level(void)
{
  uint16_t lvl; uint32_t a, b;
  level_from_total(total_min, &lvl, &a, &b);
  return lvl;
}

/* ---- Klasse aus der Verteilung -------------------------------------- */

static char class_buf[64];

/* Evolutionsstufe des Nomens - dieselben Schwellen, die auch die
   Bausteine freischalten. Das Bestimmungswort des Zweitplatzierten
   bleibt bewusst unveraendert.                                      */
static int title_tier(uint16_t lvl)
{
  if (lvl >= TITLE_LVL3) return 2;
  if (lvl >= TITLE_LVL2) return 1;
  return 0;
}

/* Dominante Tageszeit aus dem gewichteten Stundenhistogramm. */
static uint8_t title_daypart(void)
{
  uint32_t b[4] = {0, 0, 0, 0};      /* Nacht, Morgen, Mittag, Abend */
  for (int h = 0; h < 24; h++) {
    int k = (h >= 5 && h <= 10) ? 1 : (h >= 11 && h <= 16) ? 2
          : (h >= 17 && h <= 21) ? 3 : 0;
    b[k] += st.hour_w[h];
  }
  int best = 0;
  for (int i = 1; i < 4; i++) if (b[i] > b[best]) best = i;
  if (b[best] == 0) return LEAD_NONE;
  return (uint8_t)(LEAD_NIGHT + best);   /* Reihenfolge passt zum enum */
}

/* Fokus oder Vielseitigkeit, gemessen an der Zahl der gepflegten
   Module: bei zwei Modulen ist 50:50 maximal breit, bei sechs 17:17. */
static uint8_t title_spread(void)
{
  int n = mod_active_count();
  if (n < 2) return LEAD_NONE;

  uint32_t tot = 0, top = 0;
  for (int i = 0; i < PRESET_COUNT; i++) {
    tot += st.rec_min[i];
    if (st.rec_min[i] > top) top = st.rec_min[i];
  }
  if (tot == 0) return LEAD_NONE;

  uint32_t share = top * 100 / tot;
  uint32_t even  = 100 / n;
  if (share >= even + (100 - even) / 2) return LEAD_FOCUS;
  if (share <= even * 5 / 4)            return LEAD_BROAD;
  return LEAD_NONE;
}

/* Wird beim Wochenwechsel gerufen - und einmal, wenn noch kein Titel
   vergeben wurde. vices = Eintraege der abgelaufenen Woche.          */
static void title_eval(uint32_t vices)
{
  int      b1 = -1, b2 = -1;
  uint32_t m1 = 0,  m2 = 0;
  for (int i = 0; i < PRESET_COUNT; i++) {
    uint32_t v = st.minutes[i];
    if (v > m1)      { m2 = m1; b2 = b1; m1 = v; b1 = i; }
    else if (v > m2) { m2 = v;  b2 = i; }
  }

  st.t_noun = (b1 >= 0) ? (uint8_t)b1 : 0;
  st.t_pre  = (b2 >= 0 && m2 * 100 >= m1 * COMBO_SHARE) ? (uint8_t)b2
                                                        : st.t_noun;

  /* Leitwort nach Auffaelligkeit: verdientes Beiwort schlaegt
     Charakterzug, der schlaegt die Tageszeit.                        */
  uint8_t lead = LEAD_NONE;
  if (vices >= REFLECT_MIN)  lead = LEAD_REFLECT;
  if (lead == LEAD_NONE)     lead = title_spread();
  if (lead == LEAD_NONE)     lead = title_daypart();

  st.t_lead  = lead;
  st.t_valid = 1;
}

/* Beim Ueberschreiten einer Titelstufe wird der Titel sofort neu
   bestimmt statt erst beim Wochenwechsel. Gibt true zurueck, wenn
   eine Stufe erreicht wurde.                                        */
static bool title_level_check(void)
{
  uint16_t lvl = current_level();
  uint16_t cap = (lvl > 255) ? 255 : lvl;   /* Feld ist ein Byte */

  if (cap <= st.t_level) { st.t_level = (uint8_t)cap; return false; }

  uint8_t old = st.t_level;
  st.t_level  = (uint8_t)cap;

  const uint8_t th[3] = { TITLE_LVL1, TITLE_LVL2, TITLE_LVL3 };
  for (int k = 0; k < 3; k++) {
    if (old < th[k] && lvl >= th[k]) {
      uint32_t v = 0;
      for (int b = 0; b < BAD_PRESET_COUNT; b++) v += st.bad_week[b];
      title_eval(v);
      return true;
    }
  }
  return false;
}

static const char *current_class(void)
{
  const char *novice = (lang == LANG_DE) ? "NOVIZE" : "NOVICE";
  uint16_t lvl = current_level();
  if (lvl < TITLE_LVL1 || total_min == 0) return novice;

  if (!st.t_valid) {                 /* erste Vergabe ohne Wochenwechsel */
    uint32_t v = 0;
    for (int i = 0; i < BAD_PRESET_COUNT; i++) v += st.bad_week[i];
    title_eval(v);
  }

  const char *noun = preset[st.t_noun].title[title_tier(lvl)][lang];
  if (lvl < TITLE_LVL2) return noun;

  const char *pre = (st.t_pre != st.t_noun) ? preset[st.t_pre].pre[lang] : "";
  const char *sep = (*pre) ? TITLE_SEP[lang] : "";

  if (lvl < TITLE_LVL3 || st.t_lead == LEAD_NONE) {
    snprintf(class_buf, sizeof(class_buf), "%s%s%s", pre, sep, noun);
    return class_buf;
  }

  snprintf(class_buf, sizeof(class_buf), "%s %s%s%s",
           LEAD[st.t_lead][lang], pre, sep, noun);
  return class_buf;
}

/* ---- Wochenrechnung --------------------------------------------------
   Woche laeuft Montag 04:00 bis Montag 04:00. Die Grenze um vier
   morgens, damit ein langer Freitagabend nicht auf Samstag rutscht. */

static uint32_t week_start_for(time_t now)
{
  struct tm t;
  localtime_r(&now, &t);

  int      wd    = (t.tm_wday + 6) % 7;         /* Montag = 0 */
  uint32_t since = (uint32_t)wd * 86400UL
                 + (uint32_t)t.tm_hour * 3600UL
                 + (uint32_t)t.tm_min * 60UL
                 + (uint32_t)t.tm_sec;
  uint32_t start = (uint32_t)now - since + 4UL * 3600UL;
  if ((uint32_t)now < start) start -= 7UL * 86400UL;
  return start;
}

static uint32_t week_total(void)
{
  uint32_t n = 0;
  for (int i = 0; i < MOD_COUNT; i++) if (mod_active(i)) n += *mod_week_p(i);
  return n;
}

static uint32_t prev_week_total(void)
{
  uint32_t n = 0;
  for (int i = 0; i < MOD_COUNT; i++) if (mod_active(i)) n += *mod_prev_p(i);
  return n;
}

/* Was die laufende Woche beim jetzigen Stand einbringen wuerde */
static uint32_t week_bonus_preview(void)
{
  uint32_t tot  = week_total();
  uint32_t over = (tot > st.week_goal) ? (tot - st.week_goal) : 0;
  uint32_t b    = over / 4;
  return (b > st.week_goal) ? st.week_goal : b;
}

/* Aus loop() aufrufen, sobald die Uhr steht.
   true = Woche wurde gerade abgeschlossen.                          */
static bool week_tick(bool have_time)
{
  if (!have_time) return false;

  uint32_t ws = week_start_for(time(NULL));

  if (st.week_start == 0) { st.week_start = ws; return false; }
  if (ws <= st.week_start) return false;

  /* Woche abschliessen */
  uint32_t tot = week_total();

  uint32_t vices_week = 0;
  for (int i = 0; i < BAD_PRESET_COUNT; i++) vices_week += st.bad_week[i];

  title_eval(vices_week);
  st.t_hist[st.week_hist_pos][0] = st.t_noun;
  st.t_hist[st.week_hist_pos][1] = st.t_pre;
  /* Stufe in den oberen Bits - LEAD_* braucht nur drei, das Byte
     bleibt damit ein Byte und das Speicherformat unveraendert.     */
  st.t_hist[st.week_hist_pos][2] =
      (uint8_t)(st.t_lead | (title_tier(current_level()) << 4));

  /* Gewichte halbieren: die jüngste Woche zaehlt am meisten */
  for (int i = 0; i < 24; i++)           st.hour_w[i]  /= 2;
  for (int i = 0; i < PRESET_COUNT; i++) st.rec_min[i] /= 2;
  for (int i = 0; i < PRESET_COUNT; i++) {
    st.prev_week[i] = st.week_min[i];
    st.week_min[i]  = 0;
  }
  for (int i = 0; i < BAD_PRESET_COUNT; i++) {
    st.bad_prev[i] = st.bad_week[i];
    st.bad_week[i] = 0;
  }
  st.week_hist[st.week_hist_pos] = (tot > 65535) ? 65535 : (uint16_t)tot;
  st.week_hist_pos = (st.week_hist_pos + 1) % WEEK_HIST;

  if (tot >= st.week_goal) st.seals++;
  else                     st.seals = 0;

  /* 25 Prozent der Minuten ueber dem Ziel, gedeckelt auf die
     Zielhoehe. Nicht farmbar - der Ueberschuss muss erst
     eingetragen worden sein.                                     */
  uint32_t over = (tot > st.week_goal) ? (tot - st.week_goal) : 0;
  last_bonus = over / 4;
  if (last_bonus > st.week_goal) last_bonus = st.week_goal;
  if (last_bonus) {
    bonus_min += last_bonus;
    prefs.putUInt("bonus", bonus_min);
    Serial.printf("[week] Bonus +%lu EXP\n", (unsigned long)last_bonus);
  }
  prefs.putUInt("lastbonus", last_bonus);
  recalc_total();

  /* Mehr als eine Grenze uebersprungen: das Geraet war laenger aus.
     Die Luecke wird als Nullwochen eingetragen und die Siegelkette
     reisst - sonst behauptet sie etwas, das nicht stattgefunden hat. */
  uint32_t skipped = (ws - st.week_start) / 604800UL;
  if (skipped > 1) {
    for (uint32_t g = 1; g < skipped && g < WEEK_HIST; g++) {
      st.week_hist[st.week_hist_pos]  = 0;
      st.t_hist[st.week_hist_pos][0]  = 0xFF;
      st.week_hist_pos = (st.week_hist_pos + 1) % WEEK_HIST;
    }
    st.seals   = 0;
    week_gap   = (skipped - 1 > 255) ? 255 : (uint8_t)(skipped - 1);
    gap_dirty  = true;
    Serial.printf("[week] %lu Wochen uebersprungen, Siegel zurueckgesetzt\n",
                  (unsigned long)(skipped - 1));
  }

  st.week_start     = ws;
  st.review_pending = 1;
  Serial.printf("[week] abgeschlossen: %lu min, Siegel %d\n",
                (unsigned long)tot, st.seals);
  return true;
}

/* ---- Persistenz ------------------------------------------------------
   NVS statt Datei: verschleissarm, atomar, kein Dateisystem-Mount.
   Geschrieben wird erst, wenn SAVE_DELAY lang nichts mehr passiert.   */

static bool     store_dirty  = false;
static uint32_t store_marked = 0;

static void store_defaults(void)
{
  memset(&st, 0, sizeof(st));
  st.version    = 6;
  st.configured = 0;
  st.brightness = 1;
  st.week_goal  = WEEK_GOAL_DEF;
  memset(st.t_hist, 0xFF, sizeof(st.t_hist));
  for (int i = 0; i < MOD_COUNT; i++) st.slot[i]     = SLOT_EMPTY;
  for (int i = 0; i < BAD_COUNT; i++) st.bad_slot[i] = SLOT_EMPTY;
}

static void store_save_now(void)
{
  /* Erst die Sicherung, dann das Original. Faellt der Strom mitten
     im zweiten Schreibvorgang aus, bleibt die Sicherung heil.      */
  prefs.putBytes("state_b", &st, sizeof(st));
  prefs.putBytes("state", &st, sizeof(st));
  store_dirty = false;
  Serial.println("[store] gespeichert");
}

static void store_begin(void)
{
  prefs.begin("lifebar", false);
  store_defaults();

  size_t len = prefs.getBytesLength("state");
  Serial.printf("[store] NVS-Blob %u Byte (erwartet %u)\n",
                (unsigned)len, (unsigned)sizeof(lb_state_t));

  if (len == sizeof(lb_state_t)) {
    lb_state_t tmp;
    prefs.getBytes("state", &tmp, sizeof(tmp));

    if (tmp.version == 6) {
      st = tmp;
    } else {
      /* Groesse stimmt, Version nicht - vermutlich ein einzelnes
         beschaedigtes Byte nach einem Stromausfall. Wenn der Rest
         plausibel aussieht, uebernehmen wir ihn trotzdem, statt
         Monate an Daten wegzuwerfen.                              */
      bool sane = true;
      for (int i = 0; i < MOD_COUNT; i++)
        if (tmp.slot[i] != SLOT_EMPTY && tmp.slot[i] >= PRESET_COUNT) sane = false;
      for (int i = 0; i < BAD_COUNT; i++)
        if (tmp.bad_slot[i] != SLOT_EMPTY && tmp.bad_slot[i] >= BAD_PRESET_COUNT) sane = false;
      if (tmp.week_goal == 0 || tmp.week_goal > 4200) sane = false;
      for (int i = 0; i < PRESET_COUNT; i++)
        if (tmp.minutes[i] > 60UL * 24 * 365 * 20) sane = false;

      if (sane) {
        st = tmp;
        st.version = 6;
        Serial.printf("[store] Version war %u, Datensatz sah gesund aus "
                      "und wurde uebernommen\n", tmp.version);
      } else {
        Serial.printf("[store] Version %u und unplausibler Inhalt - "
                      "verworfen\n", tmp.version);
      }
    }
  } else if (len == sizeof(lb_state_v5_t)) {
    lb_state_v5_t o;
    prefs.getBytes("state", &o, sizeof(o));
    if (o.version == 5) {
      memcpy(&st, &o, sizeof(o));   /* gemeinsamer Kopf ist deckungsgleich */
      st.version = 6;
      memset(st.hour_w,  0, sizeof(st.hour_w));
      memset(st.rec_min, 0, sizeof(st.rec_min));
      st.t_valid = 0;
      memset(st.t_hist, 0xFF, sizeof(st.t_hist));
      Serial.println("[store] Format 5 -> 6 migriert");
    }
  }

  if (st.t_level == 0) {
    uint16_t l = current_level();
    st.t_level = (uint8_t)((l > 255) ? 255 : l);
  }
  week_gap = prefs.getUChar("gapw", 0);
  /* Wenn das Original nichts hergab, die Sicherung versuchen */
  if (!st.configured && prefs.getBytesLength("state_b") == sizeof(lb_state_t)) {
    lb_state_t bak;
    prefs.getBytes("state_b", &bak, sizeof(bak));
    if (bak.version == 6 && bak.configured) {
      st = bak;
      Serial.println("[store] Original defekt, Sicherung geladen");
    }
  }

  bonus_min  = prefs.getUInt("bonus", 0);
  last_bonus = prefs.getUInt("lastbonus", 0);
  recalc_total();
  Serial.printf("[store] configured=%d, %lu min gesamt\n",
                st.configured, (unsigned long)total_min);
}

static void store_mark_dirty(void)
{
  store_dirty  = true;
  store_marked = millis();
}

/* Aus loop() aufrufen. */
static void store_tick(void)
{
  if (!store_dirty) return;
  if (millis() - store_marked < SAVE_DELAY) return;
  store_save_now();
}

/* Werksreset ueber die BOOT-Taste: beim Start gedrueckt halten.
   Rettungsweg, wenn der Datensatz beschaedigt ist.                 */
static bool store_boot_reset_requested(void)
{
  pinMode(0, INPUT_PULLUP);
  delay(30);
  if (digitalRead(0) != LOW) return false;

  Serial.println("[boot] GPIO0 liegt auf LOW - BOOT gedrueckt?");

  uint32_t t0 = millis();
  while (digitalRead(0) == LOW) {
    if (millis() - t0 > 2000) return true;
    delay(20);
  }
  return false;
}

static void store_wipe(void)
{
  prefs.clear();
  Serial.println("[store] NVS geleert");
}

static void store_reset(void)
{
  store_defaults();
  bonus_min  = 0;
  last_bonus = 0;
  prefs.putUInt("bonus", 0);
  prefs.putUInt("lastbonus", 0);
  recalc_total();
  store_save_now();
}

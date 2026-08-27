#pragma once
#include "lvgl.h"
#include "lifebar_i18n.h"
#include "lifebar_model.h"
#include "lifebar_net.h"
#include "lifebar_log.h"
#include "src/lcd_bl_bsp/lcd_bl_pwm_bsp.h"

/* =====================================================================
   LifeBar - UI  (V2)
   Logische Flaeche nach der 90-Grad-Softwarerotation: 640 x 172

   Hauptseite : Tap = +0,5 h, langer Druck = -0,5 h
                Wischen nach oben  -> Statistik
   Statistik  : Wischen nach unten -> zurueck
   Wizard     : nur beim allerersten Start
   ===================================================================== */

#define SCR_W     640
#define SCR_H     172

#define COL_SCR   0x0B0B0F
#define COL_CARD  0x1C1C24
#define COL_HIT   0x2E6BFF
#define COL_TEXT  0xF2F2F7
#define COL_DIM   0x7A7A88
#define COL_WARN  0xFF4D4D
#define COL_GOOD  0x2FA84F

/* Zeitschwelle nur noch dort, wo tatsaechlich gebucht wird. Auf die
   Spaltenauswahl wirkt sie nicht mehr - ein Fehltipp waehlt bloss die
   falsche Spalte und kostet nichts. Kapazitiver Touch kennt keinen
   Druck, diese Schwelle war das, was sich wie Druck angefuehlt hat. */
#define MIN_PRESS_MS  40

/* Emoji-Icons in den Spalten.
   Auf 1 setzen, sobald die generierte Emoji-Schrift im Sketch liegt.
   Die LVGL-Standardschrift (Montserrat) enthaelt KEINE Emoji -
   ohne eigene Schrift erscheinen nur leere Kaestchen.               */
#define LB_EMOJI  1

/* Groessere Schrift fuer Level und Titel.
   In lv_conf.h muss dafuer LV_FONT_MONTSERRAT_20 auf 1 stehen.
   Echtes Fett gibt es bei den mitgelieferten Montserrat-Fonts nicht -
   die groessere Stufe uebernimmt hier die Betonung.                 */
#if defined(LV_FONT_MONTSERRAT_20) && LV_FONT_MONTSERRAT_20
  #define LB_FONT_BIG (&lv_font_montserrat_20)
#else
  #define LB_FONT_BIG (LV_FONT_DEFAULT)
#endif

/* Grosse Uhr. Faellt der Reihe nach zurueck, je nachdem was in
   lv_conf.h aktiviert ist - am besten LV_FONT_MONTSERRAT_48 auf 1. */
#if defined(LV_FONT_MONTSERRAT_48) && LV_FONT_MONTSERRAT_48
  #define LB_FONT_HUGE (&lv_font_montserrat_48)
#elif defined(LV_FONT_MONTSERRAT_40) && LV_FONT_MONTSERRAT_40
  #define LB_FONT_HUGE (&lv_font_montserrat_40)
#elif defined(LV_FONT_MONTSERRAT_36) && LV_FONT_MONTSERRAT_36
  #define LB_FONT_HUGE (&lv_font_montserrat_36)
#elif defined(LV_FONT_MONTSERRAT_28) && LV_FONT_MONTSERRAT_28
  #define LB_FONT_HUGE (&lv_font_montserrat_28)
#else
  #define LB_FONT_HUGE LB_FONT_BIG
#endif

#if LB_EMOJI
LV_FONT_DECLARE(lb_emoji_28);
#define LB_EMOJI_FONT (&lb_emoji_28)
#endif

static lv_obj_t *scr_main   = NULL;
static lv_obj_t *scr_stats  = NULL;
static lv_obj_t *scr_wizard = NULL;

static lv_obj_t *val_label[MOD_COUNT];
static lv_obj_t *name_label[MOD_COUNT];
static lv_obj_t *xp_bar;
static lv_obj_t *lvl_label;
static lv_obj_t *title_label;
static lv_obj_t *xp_label;
static lv_obj_t *scr_net    = NULL;
static lv_obj_t *net_status = NULL;
static lv_obj_t *scr_clock  = NULL;
static lv_obj_t *scr_week   = NULL;
static lv_obj_t *scr_hero   = NULL;
static lv_obj_t *clock_time = NULL;
static lv_obj_t *clock_day  = NULL;

/* Nach einer Wischgeste kurz keine Taps annehmen, sonst zaehlt das
   Wischen ueber eine Spalte zusaetzlich als Klick.                    */
static uint32_t gesture_block = 0;

/* Wischen zaehlt nur, wenn der Finger am Rand aufgesetzt hat.
   Sonst loest jede Handbewegung ueber dem Display einen Wechsel aus. */
/* Seitwaerts darf ueberall gewischt werden: der Wechsel zwischen
   Skills und Lastern richtet keinen Schaden an, und Buchungen sind
   ohnehin durch Auswaehlen-und-Bestaetigen geschuetzt. Oben und
   unten bleibt eine Zone, damit die Spalten nicht mitziehen.     */
#define EDGE_TOP  52
#define EDGE_BOT  62

/* Wischgriffe wie die Home-Leiste am iPhone: sehr dezent, nur als
   Hinweis, dass es an diesen Kanten weitergeht.                    */
#define GRIP_COL  0x33333F

/* ---- Nachtmodus ------------------------------------------------------
   Zwischen 18 und 5 Uhr werden alle Farben waermer: Blau deutlich
   runter, Gruen leicht, Rot bleibt. Gleiche Wirkung wie Night Shift -
   und der einzige Weg hier, weil die Backlight-PWM auf V2 ins Leere
   greift.                                                            */
#define NIGHT_FROM  18
#define NIGHT_TO     5

static bool night_on  = false;   /* Funktion aktiviert                */
/* Einstellungen werden im UI-Task nur markiert und aus loop()
   geschrieben. Ein NVS-Write aus einem LVGL-Callback heraus sprengt
   den Stack des UI-Tasks und reisst das Geraet in den Neustart.    */
static volatile bool prefs_dirty = false;

static void ui_prefs_tick(void)
{
  if (gap_dirty) {
    gap_dirty = false;
    prefs.putUChar("gapw", week_gap);
  }
  if (!prefs_dirty) return;
  prefs_dirty = false;
  prefs.putUChar("lang",  lang);
  prefs.putBool ("night", night_on);
  prefs.putBool ("clock12", clock12);
}
static bool night_now = false;   /* gerade wirksam                    */

/* Farben haengen an den Objekten, nicht am Zustand. Nach einer
   Aenderung von Nachtmodus oder Helligkeit muss die Seite neu
   gebaut werden - main_refresh() setzt nur Texte.                  */
static bool theme_dirty = false;

/* Werden von loop() abgearbeitet. Beides fasst Flash bzw. Funk an -
   und beides gehoert damit nicht in einen UI-Callback.            */
static volatile bool reset_pending      = false;
static volatile bool wifi_reset_pending = false;

/* Rueckfrage vor Aktionen, die sich nicht zurueckholen lassen. */
#define ASK_WIFI  1
#define ASK_RESET 2
static lv_obj_t *scr_ask  = NULL;
static int       ask_kind = 0;

static void ui_ask(int kind);
static void ui_ask_wifi(void);
static void ui_ask_reset(void);

/* "23:07" oder "11:07 PM" - je nach Einstellung. */
static void fmt_clock(char *buf, size_t n, int hour, int min)
{
  if (!clock12) { snprintf(buf, n, "%02d:%02d", hour, min); return; }
  int h = hour % 12; if (h == 0) h = 12;
  snprintf(buf, n, "%d:%02d %s", h, min, (hour < 12) ? "AM" : "PM");
}

/* Nur die Stunde, fuer das Nachtmodus-Label. */
static void fmt_hour(char *buf, size_t n, int hour)
{
  if (!clock12) { snprintf(buf, n, "%d", hour); return; }
  int h = hour % 12; if (h == 0) h = 12;
  snprintf(buf, n, "%d%s", h, (hour < 12) ? "AM" : "PM");
}

static bool night_check(void)
{
  if (!night_on || !time_valid) return false;
  time_t now = time(NULL);
  struct tm t;
  localtime_r(&now, &t);
  return (t.tm_hour >= NIGHT_FROM) || (t.tm_hour < NIGHT_TO);
}

/* Bernsteinton, in dem der Nachtmodus alles einfaerbt.
   AMBER_G / AMBER_B bestimmen die Waerme, NIGHT_KEEP wieviel
   Originalfarbe erhalten bleibt (damit Rot und Gruen ihre Bedeutung
   behalten), NIGHT_DIM die Gesamthelligkeit.                        */
#define AMBER_G     62
#define AMBER_B     16
#define NIGHT_KEEP  30
#define NIGHT_DIM   85

/* Helligkeit. Die Backlight-PWM liegt auf V2 auf einem anderen IO als
   im Demo-Code, deshalb ueber die Farben. Stufe steht in
   st.brightness - das Feld war ohnehin ungenutzt.                   */
static const uint8_t BRIGHT_PCT[4] = { 100, 78, 58, 40 };
#define BRIGHT (BRIGHT_PCT[st.brightness & 3])

static lv_color_t C(uint32_t c)
{
  uint32_t r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;

  if (!night_now) {
    uint32_t p = BRIGHT;
    return lv_color_make(r * p / 100, g * p / 100, b * p / 100);
  }

  /* Bezug ist der hellste Kanal, nicht die Leuchtdichte: sonst wird
     ein gesaettigtes Blau zu dunklem Matsch statt zu warmem Licht. */
  uint32_t v = r; if (g > v) v = g; if (b > v) v = b;

  uint32_t ar = v, ag = v * AMBER_G / 100, ab = v * AMBER_B / 100;
  uint32_t k = NIGHT_KEEP, m = 100 - NIGHT_KEEP;

  r = (ar * m + r * k) / 100;
  g = (ag * m + g * k) / 100;
  b = (ab * m + b * k) / 100;

  uint32_t d = (uint32_t)NIGHT_DIM * BRIGHT / 100;
  return lv_color_make(r * d / 100, g * d / 100, b * d / 100);
}

static lv_coord_t press_x = -1;
static lv_coord_t press_y = -1;
static uint32_t   press_t = 0;

/* Animation, mit der die aktuelle Unterseite hereingekommen ist -
   der Rueckweg nutzt die Gegenrichtung.                            */
static lv_screen_load_anim_t back_anim = LV_SCR_LOAD_ANIM_NONE;

/* Richtung, in die auf der aktuellen Unterseite gewischt werden muss,
   um zurueckzukommen. Nur diese eine Geste zaehlt dort.            */
static lv_dir_t back_dir = LV_DIR_NONE;

/* Wohin der Rueckweg fuehrt. NULL = Hauptseite. So laesst sich ein
   Stapel bauen: Hauptseite -> Woche -> Einstellungen.              */
static lv_obj_t *back_screen = NULL;

/* Zeichnet einen Wischgriff an die angegebene Kante. */
static void add_grip(lv_obj_t *parent, lv_dir_t edge)
{
  int x, y, w, h;
  switch (edge) {
    case LV_DIR_TOP:    x = (SCR_W - 90) / 2; y = 6;              w = 90; h = 3; break;
    case LV_DIR_BOTTOM: x = (SCR_W - 90) / 2; y = SCR_H - 9;      w = 90; h = 3; break;
    case LV_DIR_LEFT:   x = 6;                y = (SCR_H - 56)/2; w = 3;  h = 56; break;
    case LV_DIR_RIGHT:  x = SCR_W - 9;        y = (SCR_H - 56)/2; w = 3;  h = 56; break;
    default: return;
  }
  lv_obj_t *b = lv_obj_create(parent);
  lv_obj_remove_style_all(b);
  lv_obj_set_size(b, w, h);
  lv_obj_set_pos(b, x, y);
  lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(b, C(GRIP_COL), 0);
  lv_obj_set_style_radius(b, 2, 0);
  lv_obj_remove_flag(b, LV_OBJ_FLAG_CLICKABLE);
}

/* Sitzt der Aufsetzpunkt an der Kante, die zu dieser Wischrichtung
   gehoert?                                                          */
static bool edge_ok(lv_dir_t dir, lv_coord_t px, lv_coord_t py)
{
  LV_UNUSED(px);
  switch (dir) {
    case LV_DIR_RIGHT:
    case LV_DIR_LEFT:   return true;                    /* ueberall */
    case LV_DIR_BOTTOM: return py <= EDGE_TOP;
    case LV_DIR_TOP:    return py >= SCR_H - EDGE_BOT;
    default:            return false;
  }
}

static lv_screen_load_anim_t anim_invert(lv_screen_load_anim_t a)
{
  switch (a) {
    case LV_SCR_LOAD_ANIM_MOVE_LEFT:   return LV_SCR_LOAD_ANIM_MOVE_RIGHT;
    case LV_SCR_LOAD_ANIM_MOVE_RIGHT:  return LV_SCR_LOAD_ANIM_MOVE_LEFT;
    case LV_SCR_LOAD_ANIM_MOVE_TOP:    return LV_SCR_LOAD_ANIM_MOVE_BOTTOM;
    case LV_SCR_LOAD_ANIM_MOVE_BOTTOM: return LV_SCR_LOAD_ANIM_MOVE_TOP;
    default:                           return LV_SCR_LOAD_ANIM_NONE;
  }
}

/* Kurz halten: bei 640x172 mit Softwarerotation wird jedes Bild
   komplett neu gezeichnet, laengere Animationen ruckeln dadurch.  */
#define ANIM_MS 140

/* Farbstufen wie die Item-Qualitaet in WoW */
static uint32_t rarity_color(uint16_t lvl)
{
  /* Farbstufen liegen zwischen den Titelstufen 3, 10 und 20, damit
     sich die beiden Belohnungsarten abwechseln.                    */
  if (lvl >= 50) return 0xE6CC80;   /* artefakt      */
  if (lvl >= 36) return 0xFF8000;   /* legendaer     */
  if (lvl >= 24) return 0xA335EE;   /* episch        */
  if (lvl >= 14) return 0x0070DD;   /* selten        */
  if (lvl >=  6) return 0x1EFF00;   /* ungewoehnlich */
  return 0xFFFFFF;                  /* Anfang: weiss */
}

/* Balkenfuellung: dieselbe Farbe, auf 45 Prozent abgedunkelt.
   So bleibt weisser Text sowohl auf der Fuellung als auch auf dem
   leeren Rest lesbar - der Text sitzt ja mittig ueber beidem.      */
static uint32_t dim_color(uint32_t c)
{
  uint32_t r = ((c >> 16) & 0xFF) * 45 / 100;
  uint32_t g = ((c >>  8) & 0xFF) * 45 / 100;
  uint32_t b = ( c        & 0xFF) * 45 / 100;
  return (r << 16) | (g << 8) | b;
}

static void main_refresh(void);
static void settings_show(void);
static void ui_build_main(void);
static void ui_build_net(void);
static void ui_init(void);
static void ui_build_lock(void);
static bool     locked     = false;
static uint32_t last_touch = 0;
static void ui_build_week(void);
static lv_obj_t *sub_screen(lv_obj_t **slot, lv_dir_t back);
static void back_main_timer_cb(lv_timer_t *t);
static void tab_cb(lv_event_t *e);
static void edit_cb(lv_event_t *e);
static int  wiz_step = 0;   /* 0 = gute, 1 = schlechte Gewohnheiten */
static int  wiz_mode = 0;   /* 0 = Ersteinrichtung, 1 = nur Skills, 2 = nur Laster */
static void wizard_open(int mode);
static void back_gesture_cb(lv_event_t *e);



/* Minuten als "12.5 h" */
static void fmt_h(char *buf, size_t n, uint32_t min)
{
  snprintf(buf, n, "%lu.%c h", (unsigned long)(min / 60),
           (min % 60) ? '5' : '0');
}

/* ---- Helligkeit ------------------------------------------------------ */

/* Helligkeitssteuerung ist raus: auf V2 liegt LCD_BL an einem anderen
   IO als im Demo-Code, die PWM greift dort ins Leere. Das Feld
   st.brightness bleibt im Struct, damit das NVS-Format stabil ist. */

/* =====================================================================
   Vollbildmeldungen (Start, Neustart)
   ===================================================================== */

static lv_obj_t *scr_msg = NULL;

/* Objekte nie aus ihrem eigenen Event-Callback loeschen - LVGL
   arbeitet danach noch damit. lv_async_call verschiebt das ans Ende
   des Verarbeitungszyklus.                                          */
static void del_async(void *obj)
{
  lv_obj_delete((lv_obj_t *)obj);
}

static void rebuild_async(void *fn)
{
  ((void (*)(void))fn)();
}

static void ui_message(const char *big, const char *small, uint32_t col)
{
  lv_obj_t *old = scr_msg;
  scr_msg = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_msg, C(COL_SCR), 0);
  lv_obj_set_style_pad_all(scr_msg, 0, 0);
  lv_obj_set_style_border_width(scr_msg, 0, 0);
  lv_obj_remove_flag(scr_msg, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *t = lv_label_create(scr_msg);
  /* Lange Titel passen in 48 px nicht auf 640 px Breite */
  lv_obj_set_style_text_font(t,
      (strlen(big) > 12) ? LB_FONT_BIG : LB_FONT_HUGE, 0);
  lv_obj_set_style_text_color(t, C(col), 0);
  lv_obj_align(t, LV_ALIGN_CENTER, 0, small ? -18 : 0);
  lv_label_set_text(t, big);

  if (small) {
    lv_obj_t *u = lv_label_create(scr_msg);
    lv_obj_set_style_text_color(u, C(COL_DIM), 0);
    lv_obj_align(u, LV_ALIGN_CENTER, 0, 26);
    lv_label_set_text(u, small);
  }

  lv_screen_load(scr_msg);
  if (old) lv_async_call(del_async, old);
}

/* Startbild. Wird vor der Initialisierung gezeigt, damit die ersten
   Sekunden nicht schwarz bleiben.                                   */
static void ui_splash(void)
{
  ui_message("LIFEBAR", T(S_STARTING), COL_TEXT);
}

/* =====================================================================
   Hauptseite - Skills und Laster in einer Ansicht

   Erster Tap waehlt eine Spalte, danach buchen die grossen Flaechen
   unten. Ein Fehltreffer kostet damit einen Tap statt einer falschen
   Buchung - bei 12 mm Spaltenbreite und schraegem Blick auf das flach
   liegende Display ist genaues Zielen sonst nicht zuverlaessig.
   ===================================================================== */

static bool view_bad = false;   /* false = Skills, true = Laster */
static int  sel_idx  = -1;      /* ausgewaehlte Spalte, -1 = keine */

#define VIEW_MAX (MOD_COUNT > BAD_COUNT ? MOD_COUNT : BAD_COUNT)
static lv_obj_t *col_obj[VIEW_MAX];
static lv_obj_t *act_row = NULL;
static lv_obj_t *act_lbl = NULL;

static int  view_count(void)        { return view_bad ? BAD_COUNT : MOD_COUNT; }
static bool view_active(int i)      { return view_bad ? bad_active(i) : mod_active(i); }
static const char *view_name(int i) { return view_bad ? bad_name(i) : mod_name(i); }
static const char *view_icon(int i) { return view_bad ? bad_icon(i) : mod_icon(i); }

static void view_value(int i, char *buf, size_t n)
{
  if (view_bad) snprintf(buf, n, "%lu x", (unsigned long)(*bad_tot_p(i)));
  else          snprintf(buf, n, "%lu.%c h",
                         (unsigned long)((*mod_min_p(i)) / 60),
                         ((*mod_min_p(i)) % 60) ? '5' : '0');
}

static void refresh_module(int i)
{
  if (!view_active(i) || val_label[i] == NULL) return;
  char b[16];
  view_value(i, b, sizeof(b));
  lv_label_set_text(val_label[i], b);
}

/* Auswahl zeichnen und die Buchungsflaechen ein- oder ausblenden */
static void refresh_selection(void)
{
  for (int i = 0; i < view_count(); i++) {
    if (!col_obj[i]) continue;
    bool on = (i == sel_idx);
    lv_obj_set_style_bg_color(col_obj[i],
        C(on ? (view_bad ? 0x4A2028 : 0x1E3A5F) : COL_CARD), 0);
    lv_obj_set_style_border_width(col_obj[i], on ? 2 : 0, 0);
    lv_obj_set_style_border_color(col_obj[i],
        C(view_bad ? COL_WARN : COL_GOOD), 0);
    if (name_label[i])
      lv_obj_set_style_text_color(name_label[i], C(on ? COL_TEXT : COL_DIM), 0);
  }

  if (sel_idx < 0) {
    if (act_row) lv_obj_add_flag(act_row, LV_OBJ_FLAG_HIDDEN);
    if (xp_bar)  lv_obj_remove_flag(xp_bar, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  if (xp_bar)  lv_obj_add_flag(xp_bar, LV_OBJ_FLAG_HIDDEN);
  if (act_row) lv_obj_remove_flag(act_row, LV_OBJ_FLAG_HIDDEN);
  if (act_lbl) lv_label_set_text_fmt(act_lbl, "%s  %s",
                   view_bad ? "+1" : "+0.5 h", view_name(sel_idx));
}

static void main_refresh(void)
{
  uint16_t lvl;
  uint32_t have, need;
  level_from_total(total_min, &lvl, &have, &need);

  uint32_t rc = rarity_color(lvl);
  lv_label_set_text_fmt(lvl_label, "LVL %u", lvl);
  lv_obj_set_style_text_color(lvl_label, C(rc), 0);
  lv_label_set_text(title_label, current_class());
  lv_obj_set_style_text_color(title_label, C(rc), 0);
  lv_label_set_text_fmt(xp_label, "%lu / %lu EXP",
                        (unsigned long)have, (unsigned long)need);
  lv_bar_set_range(xp_bar, 0, (int32_t)need);
  lv_bar_set_value(xp_bar, (int32_t)have, LV_ANIM_ON);
  lv_obj_set_style_bg_color(xp_bar, C(dim_color(rc)), LV_PART_INDICATOR);

  for (int i = 0; i < view_count(); i++) {
    if (name_label[i]) lv_label_set_text(name_label[i], view_name(i));
    refresh_module(i);
  }
  refresh_selection();
}

/* Tap auf eine Spalte waehlt aus, nochmal darauf hebt es auf */
static void col_event_cb(lv_event_t *e)
{
  int i = (int)(intptr_t)lv_event_get_user_data(e);
  if (!view_active(i)) return;
  if (lv_tick_get() - gesture_block < 400) return;   /* nach einem Wisch */

  sel_idx = (sel_idx == i) ? -1 : i;
  refresh_selection();
}

/* Buchen. dir = +1 oder -1 */
static void act_cb(lv_event_t *e)
{
  int dir = (int)(intptr_t)lv_event_get_user_data(e);
  int i   = sel_idx;
  if (i < 0 || !view_active(i)) return;
  if (lv_tick_get() - press_t < MIN_PRESS_MS) return;

  if (view_bad) {
    if (dir > 0) {
      (*bad_tot_p(i))++; (*bad_week_p(i))++;
      log_add(EV_BAD, st.bad_slot[i], 1);
    } else {
      if ((*bad_tot_p(i)) == 0) return;
      (*bad_tot_p(i))--;
      if (*bad_week_p(i)) (*bad_week_p(i))--;
      log_add(EV_BAD, st.bad_slot[i], -1);
    }
    refresh_module(i);
    store_mark_dirty();
    return;
  }

  if (dir > 0) {
    (*mod_min_p(i))  += STEP_MIN;
    (*mod_week_p(i)) += STEP_MIN;
    st.rec_min[st.slot[i]] += STEP_MIN;
    if (time_valid) {
      time_t nw = time(NULL); struct tm tt;
      localtime_r(&nw, &tt);
      if (st.hour_w[tt.tm_hour] < 60000) st.hour_w[tt.tm_hour]++;
    }
    log_add(EV_SKILL, st.slot[i], STEP_MIN);
  } else {
    if ((*mod_min_p(i)) < STEP_MIN) return;
    (*mod_min_p(i)) -= STEP_MIN;
    if ((*mod_week_p(i)) >= STEP_MIN) (*mod_week_p(i)) -= STEP_MIN;
    if (st.rec_min[st.slot[i]] >= STEP_MIN)
      st.rec_min[st.slot[i]] -= STEP_MIN;
    log_add(EV_SKILL, st.slot[i], -STEP_MIN);
  }

  recalc_total();
  main_refresh();
  store_mark_dirty();

  if (title_level_check()) {
    store_mark_dirty();
    main_refresh();
    ui_message(current_class(), T(S_NEW_TITLE),
               rarity_color(current_level()));
    lv_timer_create(back_main_timer_cb, 2600, NULL);
  }
}

/* Rueckweg von einer Unterseite: nur die Gegenrichtung, und nur von
   der Kante mit dem Griff. Alles andere wird ignoriert, damit eine
   beilaeufige Bewegung nicht herausfuehrt.                          */
static void back_gesture_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());

  if (press_x < 0)                     return;
  if (dir != back_dir)                 return;
  if (!edge_ok(dir, press_x, press_y)) return;

  gesture_block = lv_tick_get();
  press_x = -1;
  lv_indev_wait_release(lv_indev_active());

  lv_obj_t *target = back_screen ? back_screen : scr_main;
  if (theme_dirty || !target) { ui_build_main(); return; }
  lv_screen_load_anim(target, anim_invert(back_anim), ANIM_MS, 0, false);
}


/* Aufsetzpunkt und Zeitpunkt merken - beides brauchen die
   Kantenpruefung der Wischgesten und die Ruheerkennung.            */
static void press_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  lv_point_t p;
  lv_indev_get_point(lv_indev_active(), &p);
  press_x    = p.x;
  press_y    = p.y;
  press_t    = lv_tick_get();
  last_touch = press_t;
}

static void tab_cb(lv_event_t *e)
{
  bool want = (bool)(intptr_t)lv_event_get_user_data(e);
  if (lv_tick_get() - press_t < MIN_PRESS_MS) return;
  if (want == view_bad) return;
  view_bad = want;
  sel_idx  = -1;
  ui_build_main();
}

static void main_gesture_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());

  if (press_x < 0) return;
  if (!edge_ok(dir, press_x, press_y)) return;

  gesture_block = lv_tick_get();
  press_x = -1;
  lv_indev_wait_release(lv_indev_active());

  switch (dir) {
    case LV_DIR_LEFT:
    case LV_DIR_RIGHT:                   /* Skills <-> Laster */
      view_bad = !view_bad;
      sel_idx  = -1;
      ui_build_main();
      break;
    case LV_DIR_BOTTOM: ui_build_week(); break;
    case LV_DIR_TOP:    ui_build_lock(); break;
    default: break;
  }
}

static void ui_build_main(void)
{
  lv_obj_t *old = scr_main;
  scr_main = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_main, C(COL_SCR), 0);
  lv_obj_set_style_pad_all(scr_main, 0, 0);
  lv_obj_set_style_border_width(scr_main, 0, 0);
  lv_obj_remove_flag(scr_main, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(scr_main, main_gesture_cb, LV_EVENT_GESTURE, NULL);
  lv_obj_add_event_cb(scr_main, press_cb, LV_EVENT_PRESSED, NULL);

  /* Reiter: macht sichtbar, dass es eine zweite Ansicht gibt.
     Antippbar, damit man den Wechsel nicht erraten muss.            */
  {
    lv_obj_t *tabs = lv_obj_create(scr_main);
    lv_obj_remove_style_all(tabs);
    lv_obj_set_size(tabs, SCR_W, 16);
    lv_obj_set_pos(tabs, 0, 2);
    lv_obj_set_flex_flow(tabs, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tabs, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(tabs, 18, 0);
    lv_obj_add_flag(tabs, LV_OBJ_FLAG_GESTURE_BUBBLE);

    for (int k = 0; k < 2; k++) {
      bool active = (k == 1) == view_bad;
      lv_obj_t *t = lv_label_create(tabs);
      lv_obj_set_style_text_color(t,
          C(active ? (view_bad ? COL_WARN : COL_GOOD) : 0x3C3C48), 0);
      if (active)      lv_label_set_text(t, k ? T(S_VICES) : T(S_SKILLS));
      else if (k == 1) lv_label_set_text_fmt(t, "%s  >", T(S_VICES));
      else             lv_label_set_text_fmt(t, "<  %s", T(S_SKILLS));
      lv_obj_add_flag(t, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_flag(t, LV_OBJ_FLAG_EVENT_BUBBLE);
      lv_obj_add_event_cb(t, tab_cb, LV_EVENT_CLICKED, (void *)(intptr_t)k);
    }
  }

  const int cnt = view_count();
  int n = 0;
  for (int i = 0; i < cnt; i++) if (view_active(i)) n++;
  for (int i = 0; i < VIEW_MAX; i++) {
    col_obj[i] = NULL; name_label[i] = NULL; val_label[i] = NULL;
  }

  const int gap = 5, marg = 12, col_h = 92;
  const int col_w = n ? (SCR_W - 2 * marg - (n - 1) * gap) / n : 0;
  const int x0    = (SCR_W - (n * col_w + (n - 1) * gap)) / 2;

  int c = 0;
  for (int i = 0; i < cnt; i++) {
    if (!view_active(i)) continue;

    lv_obj_t *col = lv_button_create(scr_main);
    lv_obj_set_size(col, col_w, col_h);
    lv_obj_set_pos(col, x0 + c * (col_w + gap), 20);
    lv_obj_set_style_radius(col, 8, 0);
    lv_obj_set_style_shadow_width(col, 0, 0);
    lv_obj_set_style_bg_color(col, C(COL_CARD), 0);
    lv_obj_add_flag(col, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_add_flag(col, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(col, col_event_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)i);
    col_obj[i] = col;

    name_label[i] = lv_label_create(col);
    lv_obj_set_style_text_color(name_label[i], C(COL_DIM), 0);
    lv_label_set_long_mode(name_label[i], LV_LABEL_LONG_DOT);
    lv_obj_set_width(name_label[i], col_w - 10);
    lv_obj_set_style_text_align(name_label[i], LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(name_label[i], LV_ALIGN_TOP_MID, 0, -8);

#if LB_EMOJI
    lv_obj_t *ic = lv_label_create(col);
    lv_obj_set_style_text_font(ic, LB_EMOJI_FONT, 0);
    lv_obj_set_style_text_color(ic, C(COL_TEXT), 0);
    lv_label_set_text(ic, view_icon(i));
    lv_obj_align(ic, LV_ALIGN_CENTER, 0, 1);
#endif

    val_label[i] = lv_label_create(col);
    lv_obj_set_style_text_color(val_label[i], C(COL_TEXT), 0);
    lv_obj_align(val_label[i], LV_ALIGN_BOTTOM_MID, 0, 2);
    c++;
  }

  /* Zeile mit Level, Titel und EXP */
  lv_obj_t *row = lv_obj_create(scr_main);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, SCR_W, 20);
  lv_obj_set_pos(row, 0, 112);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(row, 12, 0);
  lv_obj_add_flag(row, LV_OBJ_FLAG_GESTURE_BUBBLE);

  lvl_label = lv_label_create(row);
  lv_obj_set_style_text_font(lvl_label, LB_FONT_BIG, 0);
  title_label = lv_label_create(row);
  lv_obj_set_style_text_font(title_label, LB_FONT_BIG, 0);

  /* EXP-Leiste, sichtbar solange nichts ausgewaehlt ist */
  xp_bar = lv_bar_create(scr_main);
  lv_obj_set_size(xp_bar, SCR_W - 2 * marg, 18);
  lv_obj_set_pos(xp_bar, marg, 136);
  lv_obj_set_style_radius(xp_bar, 5, 0);
  lv_obj_set_style_bg_color(xp_bar, C(COL_CARD), 0);
  lv_obj_add_flag(xp_bar, LV_OBJ_FLAG_GESTURE_BUBBLE);

  xp_label = lv_label_create(xp_bar);      /* Zahl sitzt im Balken */
  lv_obj_set_style_text_color(xp_label, C(0xFFFFFF), 0);
  lv_obj_center(xp_label);

  /* Buchungsflaechen, erscheinen bei Auswahl */
  act_row = lv_obj_create(scr_main);
  lv_obj_remove_style_all(act_row);
  lv_obj_set_size(act_row, SCR_W, 44);
  lv_obj_set_pos(act_row, 0, 126);
  lv_obj_remove_flag(act_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(act_row, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(act_row, LV_OBJ_FLAG_GESTURE_BUBBLE);
  lv_obj_add_flag(act_row, LV_OBJ_FLAG_EVENT_BUBBLE);

  lv_obj_t *minus = lv_button_create(act_row);
  lv_obj_set_size(minus, 130, 40);
  lv_obj_set_pos(minus, marg, 2);
  lv_obj_set_style_radius(minus, 8, 0);
  lv_obj_set_style_shadow_width(minus, 0, 0);
  lv_obj_set_style_bg_color(minus, C(COL_CARD), 0);
  lv_obj_set_style_bg_color(minus, C(COL_WARN), LV_STATE_PRESSED);
  lv_obj_add_flag(minus, LV_OBJ_FLAG_GESTURE_BUBBLE);
  lv_obj_add_flag(minus, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_add_event_cb(minus, act_cb, LV_EVENT_CLICKED, (void *)(intptr_t)-1);
  lv_obj_t *ml = lv_label_create(minus);
  lv_obj_set_style_text_font(ml, LB_FONT_BIG, 0);
  lv_obj_set_style_text_color(ml, C(COL_TEXT), 0);
  lv_label_set_text(ml, view_bad ? "-1" : "-0.5 h");
  lv_obj_center(ml);

  lv_obj_t *plus = lv_button_create(act_row);
  lv_obj_set_size(plus, SCR_W - 2 * marg - 130 - gap, 40);
  lv_obj_set_pos(plus, marg + 130 + gap, 2);
  lv_obj_set_style_radius(plus, 8, 0);
  lv_obj_set_style_shadow_width(plus, 0, 0);
  lv_obj_set_style_bg_color(plus, C(view_bad ? 0x5A2630 : 0x1E4D33), 0);
  lv_obj_set_style_bg_color(plus,
      C(view_bad ? COL_WARN : COL_GOOD), LV_STATE_PRESSED);
  lv_obj_add_flag(plus, LV_OBJ_FLAG_GESTURE_BUBBLE);
  lv_obj_add_flag(plus, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_add_event_cb(plus, act_cb, LV_EVENT_CLICKED, (void *)(intptr_t)1);
  act_lbl = lv_label_create(plus);
  lv_obj_set_style_text_font(act_lbl, LB_FONT_BIG, 0);
  lv_obj_set_style_text_color(act_lbl, C(COL_TEXT), 0);
  lv_obj_center(act_lbl);

  main_refresh();
  theme_dirty = false;
  last_touch  = lv_tick_get();
  lv_screen_load(scr_main);
  if (old) lv_async_call(del_async, old);
}

static void goal_cb(lv_event_t *e)
{
  int d = (int)(intptr_t)lv_event_get_user_data(e);
  int g = (int)st.week_goal + d * 60;
  if (g < 60)   g = 60;
  if (g > 4200) g = 4200;          /* 70 h, mehr ist nicht plausibel */
  st.week_goal = (uint16_t)g;
  store_mark_dirty();
  lv_async_call(rebuild_async, (void *)settings_show);
}

/* Kleiner Helfer, damit alle Buttons gleich aussehen */
static lv_obj_t *sbtn(lv_obj_t *par, int x, int y, int w, const char *txt,
                      uint32_t bg, uint32_t bg_press, uint32_t fg,
                      lv_event_cb_t cb, void *ud)
{
  lv_obj_t *b = lv_button_create(par);
  lv_obj_set_size(b, w, 30);
  lv_obj_set_pos(b, x, y);
  lv_obj_set_style_radius(b, 6, 0);
  lv_obj_set_style_shadow_width(b, 0, 0);
  lv_obj_set_style_bg_color(b, C(bg), 0);
  lv_obj_set_style_bg_color(b, C(bg_press), LV_STATE_PRESSED);
  lv_obj_add_flag(b, LV_OBJ_FLAG_GESTURE_BUBBLE);
  lv_obj_add_flag(b, LV_OBJ_FLAG_EVENT_BUBBLE);
  if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
  lv_obj_t *l = lv_label_create(b);
  lv_obj_set_style_text_color(l, C(fg), 0);
  lv_label_set_text(l, txt);
  lv_obj_center(l);
  return b;
}

static void wifi_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  lv_async_call(rebuild_async, (void *)ui_ask_wifi);
}

static void edit_cb(lv_event_t *e)
{
  wizard_open((int)(intptr_t)lv_event_get_user_data(e));
}

static void back_main_timer_cb(lv_timer_t *t)
{
  lv_timer_delete(t);
  if (scr_main) lv_screen_load_anim(scr_main, LV_SCR_LOAD_ANIM_FADE_IN,
                                    250, 0, false);
}

static void restart_timer_cb(lv_timer_t *t)
{
  lv_timer_delete(t);
  ESP.restart();
}

/* Vollbildmeldung, Daten sichern, danach neu starten.
   reason darf NULL sein.                                            */
static void ui_restart(const char *reason)
{
  /* Schreiben uebernimmt loop() in der Sekunde bis zum Neustart */
  store_mark_dirty();
  ui_message(T(S_RESTART), reason, COL_HIT);
  lv_timer_create(restart_timer_cb, 1200, NULL);
}

/* Der Nachtmodus faerbt alles beim Zeichnen ein. Ein Neustart ist der
   ehrlichste Weg, ihn ueberall wirken zu lassen - sonst behalten die
   nicht sichtbaren Seiten ihre alten Farben.                        */
static void lang_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  lang        = (lang + 1) % LANG_COUNT;
  prefs_dirty = true;
  /* Hauptseite NICHT hier neu bauen - ein Screenwechsel aus dem
     Callback eines Buttons der aktiven Seite heraus ist fragil.
     theme_dirty sorgt dafuer, dass sie beim Zurueckgehen neu
     entsteht, also mit den neuen Texten.                          */
  theme_dirty = true;
  lv_async_call(rebuild_async, (void *)settings_show);
}

static void fmt_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  clock12 = !clock12;
  prefs_dirty = true;
  lv_async_call(rebuild_async, (void *)settings_show);
}

static void night_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  night_on = !night_on;
  prefs_dirty = true;

  night_now = night_check();
  ui_restart(night_on ? T(S_NIGHT_ON) : T(S_NIGHT_OFF));
}

static void bright_cb(lv_event_t *e)
{
  int d = (int)(intptr_t)lv_event_get_user_data(e);
  int v = (int)(st.brightness & 3) - d;      /* mehr Helligkeit = kleinerer Index */
  if (v < 0) v = 0;
  if (v > 3) v = 3;
  st.brightness = (uint8_t)v;
  theme_dirty   = true;
  store_mark_dirty();
  lv_async_call(rebuild_async, (void *)settings_show);
}

/* Eine Zeile Einstellung: Beschriftung, Wert, zwei Tasten. */
static void set_row(lv_obj_t *par, int x, int y, const char *label,
                    const char *value, lv_event_cb_t cb)
{
  lv_obj_t *l = lv_label_create(par);
  lv_obj_set_style_text_color(l, C(COL_DIM), 0);
  lv_obj_set_pos(l, x, y + 8);
  lv_label_set_text(l, label);

  lv_obj_t *v = lv_label_create(par);
  lv_obj_set_style_text_font(v, LB_FONT_BIG, 0);
  lv_obj_set_style_text_color(v, C(COL_TEXT), 0);
  lv_obj_set_pos(v, x + 118, y + 4);
  lv_label_set_text(v, value);

  sbtn(par, x + 186, y, 38, "-", COL_CARD, COL_HIT, COL_TEXT,
       cb, (void *)(intptr_t)-1);
  sbtn(par, x + 228, y, 38, "+", COL_CARD, COL_HIT, COL_TEXT,
       cb, (void *)(intptr_t)1);
}

static void ui_ask_wifi(void)  { ui_ask(ASK_WIFI); }
static void ui_ask_reset(void) { ui_ask(ASK_RESET); }

static void ask_no_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  ask_kind = 0;
  lv_async_call(rebuild_async, (void *)settings_show);
}

static void ask_yes_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  if (ask_kind == ASK_WIFI)  wifi_reset_pending = true;
  if (ask_kind == ASK_RESET) reset_pending      = true;
  ask_kind = 0;
}

/* Rueckfrage vor Aktionen, die sich nicht zurueckholen lassen. */
static void ui_ask(int kind)
{
  ask_kind = kind;

  lv_obj_t *old = scr_ask;
  scr_ask = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_ask, C(COL_SCR), 0);
  lv_obj_set_style_pad_all(scr_ask, 0, 0);
  lv_obj_set_style_border_width(scr_ask, 0, 0);
  lv_obj_remove_flag(scr_ask, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *t = lv_label_create(scr_ask);
  lv_obj_set_style_text_font(t, LB_FONT_BIG, 0);
  lv_obj_set_style_text_color(t, C(COL_WARN), 0);
  lv_label_set_long_mode(t, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(t, SCR_W - 48);
  lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 22);
  lv_label_set_text(t, kind == ASK_WIFI ? T(S_ASK_WIFI) : T(S_ASK_RESET));

  sbtn(scr_ask, 24, 118, 260, T(S_CANCEL),
       COL_CARD, COL_HIT, COL_TEXT, ask_no_cb, NULL);
  sbtn(scr_ask, SCR_W - 24 - 260, 118, 260, T(S_YES),
       0x7A1F1F, COL_WARN, COL_TEXT, ask_yes_cb, NULL);

  lv_screen_load(scr_ask);
  if (old) lv_async_call(del_async, old);
}

static void settings_show(void)
{
  lv_obj_t *sc = sub_screen(&scr_stats, LV_DIR_TOP);
  back_screen = scr_week;        /* eine Ebene zurueck, nicht ganz raus */
  char buf[16];

  lv_obj_t *head = lv_label_create(sc);
  lv_obj_set_style_text_font(head, LB_FONT_BIG, 0);
  lv_obj_set_style_text_color(head, C(COL_TEXT), 0);
  lv_obj_set_pos(head, 24, 4);
  lv_label_set_text(head, T(S_SETTINGS));

  /* --- linke Spalte: zwei Regler --- */
  snprintf(buf, sizeof(buf), "%d h", st.week_goal / 60);
  set_row(sc, 24, 36, T(S_WEEKGOAL), buf, goal_cb);

  snprintf(buf, sizeof(buf), "%d %%", BRIGHT);
  set_row(sc, 24, 70, T(S_BRIGHT), buf, bright_cb);

  /* --- rechte Spalte: drei Schalter --- */
  char h1[8], h2[8];
  fmt_hour(h1, sizeof(h1), NIGHT_FROM);
  fmt_hour(h2, sizeof(h2), NIGHT_TO);

  lv_obj_t *nl = lv_label_create(sc);
  lv_obj_set_style_text_color(nl, C(COL_DIM), 0);
  lv_obj_set_pos(nl, 330, 44);
  lv_label_set_text_fmt(nl, "%s %s-%s", T(S_NIGHTMODE), h1, h2);

  sbtn(sc, 548, 36, 68, night_on ? T(S_ON) : T(S_OFF),
       night_on ? COL_HIT : COL_CARD, COL_HIT, COL_TEXT, night_cb, NULL);

  lv_obj_t *ll = lv_label_create(sc);
  lv_obj_set_style_text_color(ll, C(COL_DIM), 0);
  lv_obj_set_pos(ll, 330, 78);
  lv_label_set_text(ll, T(S_LANGUAGE));

  sbtn(sc, 420, 70, 118, LANG_NAME[lang], COL_CARD, COL_HIT, COL_TEXT,
       lang_cb, NULL);
  sbtn(sc, 548, 70, 68, clock12 ? "12 H" : "24 H",
       COL_CARD, COL_HIT, COL_TEXT, fmt_cb, NULL);

  /* --- Buttonleiste unten --- */
  const int by = 130;
  sbtn(sc,  24, by, 130, T(S_WIFI),   COL_CARD, COL_HIT, COL_TEXT,
       wifi_cb, NULL);
  sbtn(sc, 162, by, 120, T(S_SKILLS), COL_CARD, COL_HIT, COL_TEXT,
       edit_cb, (void *)(intptr_t)1);
  sbtn(sc, 290, by, 120, T(S_VICES),  COL_CARD, COL_HIT, COL_TEXT,
       edit_cb, (void *)(intptr_t)2);

  /* Normaler Tap - die Rueckfrage ist die Sicherung, nicht das Halten */
  sbtn(sc, 466, by, 150, T(S_RESET), 0x7A1F1F, COL_WARN, COL_TEXT,
       [](lv_event_t *e) { LV_UNUSED(e);
         lv_async_call(rebuild_async, (void *)ui_ask_reset); }, NULL);

  lv_screen_load_anim(sc, back_anim, ANIM_MS, 0, false);
}

/* =====================================================================
   Wizard
   ===================================================================== */

static bool      wiz_good[PRESET_COUNT];
static bool      wiz_bad[BAD_PRESET_COUNT];
static lv_obj_t *wiz_cell[PRESET_COUNT > BAD_PRESET_COUNT
                          ? PRESET_COUNT : BAD_PRESET_COUNT];
static lv_obj_t *wiz_head;
static void ui_build_wizard(void);

static bool *wiz_flags(void)   { return wiz_step == 2 ? wiz_bad : wiz_good; }
static int   wiz_n(void)       { return wiz_step == 2 ? BAD_PRESET_COUNT : PRESET_COUNT; }
static int   wiz_max(void)     { return wiz_step == 2 ? BAD_COUNT : MOD_COUNT; }
static const preset_t *wiz_list(void) { return wiz_step == 2 ? bad_preset : preset; }

static int wiz_count(void)
{
  bool *f = wiz_flags();
  int n = 0;
  for (int i = 0; i < wiz_n(); i++) if (f[i]) n++;
  return n;
}

static void wiz_title(void)
{
  if (wiz_step == 0) { lv_label_set_text(wiz_head, T(S_PICK_LANG)); return; }
  lv_label_set_text_fmt(wiz_head, "%s   %d / %d",
                        wiz_step == 2 ? T(S_PICK_VICES) : T(S_PICK_SKILLS),
                        wiz_count(), wiz_max());
}

static void wiz_lang_cb(lv_event_t *e)
{
  lang = (uint8_t)(intptr_t)lv_event_get_user_data(e);
  prefs_dirty = true;
  wiz_step = 1;
  ui_build_wizard();
}

static void wiz_cell_cb(lv_event_t *e)
{
  int   i = (int)(intptr_t)lv_event_get_user_data(e);
  bool *f = wiz_flags();

  if (!f[i] && wiz_count() >= wiz_max()) return;
  f[i] = !f[i];
  lv_obj_set_style_bg_color(wiz_cell[i], C(f[i] ? COL_HIT : COL_CARD), 0);
  wiz_title();
}

static void wiz_next_cb(lv_event_t *e)
{
  LV_UNUSED(e);

  /* Ersteinrichtung: Sprache -> Skills -> Laster */
  if (wiz_mode == 0 && wiz_step < 2) {
    wiz_step++;
    ui_build_wizard();
    return;
  }

  if (wiz_mode != 2) {                       /* Skills uebernehmen */
    int slot = 0;
    for (int i = 0; i < PRESET_COUNT && slot < MOD_COUNT; i++) {
      if (wiz_good[i]) st.slot[slot++] = (uint8_t)i;
    }
    if (slot == 0 && wiz_mode == 0) {
      for (int i = 0; i < MOD_COUNT; i++) st.slot[i] = default_slot[i];
    } else {
      while (slot < MOD_COUNT) st.slot[slot++] = SLOT_EMPTY;
    }
  }

  if (wiz_mode != 1) {                       /* Laster uebernehmen */
    int slot = 0;
    for (int i = 0; i < BAD_PRESET_COUNT && slot < BAD_COUNT; i++) {
      if (wiz_bad[i]) st.bad_slot[slot++] = (uint8_t)i;
    }
    while (slot < BAD_COUNT) st.bad_slot[slot++] = SLOT_EMPTY;
  }

  st.configured = 1;
  recalc_total();
  store_save_now();

  lv_obj_t *old = scr_wizard;
  scr_wizard = NULL;
  if (wiz_mode == 0 && !net_has_creds() && !st.net_skipped) ui_build_net();
  else                                                      ui_build_main();
  if (old) lv_async_call(del_async, old);
}

/* mode: 0 = Ersteinrichtung, 1 = nur Skills, 2 = nur Laster.
   Beim Bearbeiten wird die bestehende Auswahl vorbelegt.          */
static void wizard_open(int mode)
{
  wiz_mode = mode;
  wiz_step = (mode == 0) ? 0 : (mode == 2 ? 2 : 1);

  for (int i = 0; i < PRESET_COUNT; i++)     wiz_good[i] = false;
  for (int i = 0; i < BAD_PRESET_COUNT; i++) wiz_bad[i]  = false;

  if (mode != 0) {                        /* aktuellen Stand vorbelegen */
    for (int i = 0; i < MOD_COUNT; i++)
      if (mod_active(i)) wiz_good[st.slot[i]] = true;
    for (int i = 0; i < BAD_COUNT; i++)
      if (bad_active(i)) wiz_bad[st.bad_slot[i]] = true;
  }
  ui_build_wizard();
}

static void ui_build_wizard(void)
{
  lv_obj_t *old = scr_wizard;
  scr_wizard = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_wizard, C(COL_SCR), 0);
  lv_obj_set_style_pad_all(scr_wizard, 0, 0);
  lv_obj_set_style_border_width(scr_wizard, 0, 0);
  lv_obj_remove_flag(scr_wizard, LV_OBJ_FLAG_SCROLLABLE);

  wiz_head = lv_label_create(scr_wizard);
  lv_obj_set_style_text_color(wiz_head, C(COL_TEXT), 0);
  lv_obj_set_pos(wiz_head, 20, 4);

  lv_obj_t *nb = lv_button_create(scr_wizard);
  lv_obj_set_size(nb, 100, 22);
  lv_obj_set_pos(nb, 520, 0);
  lv_obj_set_style_radius(nb, 6, 0);
  lv_obj_set_style_shadow_width(nb, 0, 0);
  lv_obj_set_style_bg_color(nb, C(COL_HIT), 0);
  lv_obj_add_event_cb(nb, wiz_next_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *nl = lv_label_create(nb);
  lv_obj_set_style_text_color(nl, C(COL_TEXT), 0);
  lv_label_set_text(nl, (wiz_mode == 0 && wiz_step < 2) ? T(S_NEXT)
                                                        : T(S_DONE));
  lv_obj_center(nl);

  if (wiz_step == 0) {                    /* Sprachauswahl */
    lv_obj_delete(nb);                    /* kein Weiter-Button noetig */
    for (int i = 0; i < LANG_COUNT; i++) {
      lv_obj_t *b = lv_button_create(scr_wizard);
      lv_obj_set_size(b, 220, 60);
      lv_obj_set_pos(b, (SCR_W - (LANG_COUNT * 220 + (LANG_COUNT - 1) * 20)) / 2
                        + i * 240, 70);
      lv_obj_set_style_radius(b, 8, 0);
      lv_obj_set_style_shadow_width(b, 0, 0);
      lv_obj_set_style_bg_color(b, C(i == lang ? COL_HIT : COL_CARD), 0);
      lv_obj_add_event_cb(b, wiz_lang_cb, LV_EVENT_CLICKED,
                          (void *)(intptr_t)i);
      lv_obj_t *l = lv_label_create(b);
      lv_obj_set_style_text_font(l, LB_FONT_BIG, 0);
      lv_obj_set_style_text_color(l, C(COL_TEXT), 0);
      lv_label_set_text(l, LANG_NAME[i]);
      lv_obj_center(l);
    }
    wiz_title();
    lv_screen_load(scr_wizard);
    if (old) lv_async_call(del_async, old);
    return;
  }

  const preset_t *list = wiz_list();
  bool           *f    = wiz_flags();
  const int       cnt  = wiz_n();
  const int       rows = (cnt + 5) / 6;

  const int cw = 96, gx = 5, gy = 4;
  const int ch = (rows >= 3) ? 44 : 60;
  const int x0 = (SCR_W - (6 * cw + 5 * gx)) / 2;
  const int y0 = 26;

  for (int i = 0; i < cnt; i++) {
    int r = i / 6, c = i % 6;
    lv_obj_t *cell = lv_button_create(scr_wizard);
    lv_obj_set_size(cell, cw, ch);
    lv_obj_set_pos(cell, x0 + c * (cw + gx), y0 + r * (ch + gy));
    lv_obj_set_style_radius(cell, 6, 0);
    lv_obj_set_style_shadow_width(cell, 0, 0);
    lv_obj_set_style_bg_color(cell, C(f[i] ? COL_HIT : COL_CARD), 0);
    lv_obj_add_event_cb(cell, wiz_cell_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)i);
    wiz_cell[i] = cell;

    /* Bewusst ohne Icon: auf den Auswahlkacheln ueberlagert es den
       Text. Icons gibt es nur auf den Zaehlseiten.                 */
    lv_obj_t *l = lv_label_create(cell);
    lv_obj_set_style_text_color(l, C(COL_TEXT), 0);
    lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
    lv_obj_set_width(l, cw - 8);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(l, list[i].name[lang]);
    lv_obj_center(l);
  }

  wiz_title();
  lv_screen_load(scr_wizard);
  if (old) lv_async_call(del_async, old);
}

/* =====================================================================
   WLAN-Einrichtung
   Auf dem Geraet wird nichts getippt - nur die Anleitung steht hier.
   ===================================================================== */

static void net_skip_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  net_stop_portal();
  st.net_skipped = 1;              /* Einrichtung uebersprungen */
  store_save_now();
  ui_build_main();
}

static void ui_build_net(void)
{
  net_start_portal();

  lv_obj_t *old = scr_net;
  scr_net = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_net, C(COL_SCR), 0);
  lv_obj_set_style_pad_all(scr_net, 0, 0);
  lv_obj_set_style_border_width(scr_net, 0, 0);
  lv_obj_remove_flag(scr_net, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *t = lv_label_create(scr_net);
  lv_obj_set_style_text_font(t, LB_FONT_BIG, 0);
  lv_obj_set_style_text_color(t, C(COL_TEXT), 0);
  lv_obj_set_pos(t, 20, 8);
  lv_label_set_text(t, T(S_WIFI_SETUP));

  lv_obj_t *s1 = lv_label_create(scr_net);
  lv_obj_set_style_text_color(s1, C(COL_DIM), 0);
  lv_obj_set_pos(s1, 20, 42);
  lv_label_set_text(s1, T(S_STEP1));

  lv_obj_t *ap = lv_label_create(scr_net);
  lv_obj_set_style_text_font(ap, LB_FONT_BIG, 0);
  lv_obj_set_style_text_color(ap, C(COL_HIT), 0);
  lv_obj_set_pos(ap, 20, 64);
  lv_label_set_text(ap, ap_ssid);

  lv_obj_t *s2 = lv_label_create(scr_net);
  lv_obj_set_style_text_color(s2, C(COL_DIM), 0);
  lv_obj_set_pos(s2, 20, 94);
  lv_label_set_text(s2,
    T(S_STEP2));

  lv_obj_t *s3 = lv_label_create(scr_net);
  lv_obj_set_style_text_color(s3, C(COL_DIM), 0);
  lv_obj_set_pos(s3, 20, 114);
  lv_label_set_text(s3, T(S_STEP3));

  net_status = lv_label_create(scr_net);
  lv_obj_set_style_text_color(net_status, C(COL_DIM), 0);
  lv_obj_align(net_status, LV_ALIGN_BOTTOM_LEFT, 20, -8);
  lv_label_set_text(net_status, net_state_text());

  lv_obj_t *skip = lv_button_create(scr_net);
  lv_obj_set_size(skip, 150, 30);
  lv_obj_align(skip, LV_ALIGN_BOTTOM_RIGHT, -20, -8);
  lv_obj_set_style_radius(skip, 6, 0);
  lv_obj_set_style_shadow_width(skip, 0, 0);
  lv_obj_set_style_bg_color(skip, C(COL_CARD), 0);
  lv_obj_add_event_cb(skip, net_skip_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *sl = lv_label_create(skip);
  lv_obj_set_style_text_color(sl, C(COL_TEXT), 0);
  lv_label_set_text(sl, T(S_LATER));
  lv_obj_center(sl);

  lv_screen_load(scr_net);
  if (old) lv_async_call(del_async, old);
}

/* Wird aus loop() gerufen, wenn sich der Netzzustand geaendert hat.  */
static void ui_net_changed(void)
{
  if (lv_screen_active() == scr_net) {
    /* verbunden oder Portal ausgelaufen -> zurueck auf die Hauptseite */
    if (net_state == NET_ONLINE ||
        (!portal_up && net_state != NET_CONNECTING)) {
      ui_build_main();
      return;
    }
    if (net_status) lv_label_set_text(net_status, net_state_text());
    return;
  }
  main_refresh();
}

/* =====================================================================
   Uhr  (Wischen nach rechts)
   ===================================================================== */

static void clock_refresh(void)
{
  if (!clock_time) return;

  if (!time_valid) {
    lv_label_set_text(clock_time, "--:--");
    lv_label_set_text(clock_day, T(S_NO_CLOCK));
    return;
  }

  time_t now = time(NULL);
  struct tm t;
  localtime_r(&now, &t);

  char cb[16];
  fmt_clock(cb, sizeof(cb), t.tm_hour, t.tm_min);
  lv_label_set_text(clock_time, cb);
  lv_label_set_text_fmt(clock_day, "%s   %02d.%02d.%04d",
                        WEEKDAY[(t.tm_wday + 6) % 7][lang],
                        t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
}

/* back = Richtung, in die gewischt werden muss, um zurueckzukommen.
   Der Griff wird an die zugehoerige Kante gezeichnet, die Anim
   ergibt sich als Gegenrichtung.                                    */
static lv_obj_t *sub_screen(lv_obj_t **slot, lv_dir_t back)
{
  lv_obj_t *old = *slot;
  lv_obj_t *sc  = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(sc, C(COL_SCR), 0);
  lv_obj_set_style_pad_all(sc, 0, 0);
  lv_obj_set_style_border_width(sc, 0, 0);
  lv_obj_remove_flag(sc, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(sc, back_gesture_cb, LV_EVENT_GESTURE, NULL);
  lv_obj_add_event_cb(sc, press_cb, LV_EVENT_PRESSED, NULL);

  back_dir    = back;
  back_screen = NULL;                 /* Standard: zurueck zur Hauptseite */
  switch (back) {
    case LV_DIR_LEFT:   back_anim = LV_SCR_LOAD_ANIM_MOVE_RIGHT;  break;
    case LV_DIR_RIGHT:  back_anim = LV_SCR_LOAD_ANIM_MOVE_LEFT;   break;
    case LV_DIR_TOP:    back_anim = LV_SCR_LOAD_ANIM_MOVE_BOTTOM; break;
    default:            back_anim = LV_SCR_LOAD_ANIM_MOVE_TOP;    break;
  }

  /* Griff an die Kante, an der man fuer den Rueckweg ansetzt */
  switch (back) {
    case LV_DIR_LEFT:   add_grip(sc, LV_DIR_RIGHT);  break;
    case LV_DIR_RIGHT:  add_grip(sc, LV_DIR_LEFT);   break;
    case LV_DIR_TOP:    add_grip(sc, LV_DIR_BOTTOM); break;
    default:            add_grip(sc, LV_DIR_TOP);    break;
  }

  *slot = sc;
  if (old) lv_async_call(del_async, old);
  return sc;
}

static void lock_press_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  locked     = false;
  sel_idx    = -1;
  press_x    = -1;
  last_touch = lv_tick_get();   /* sonst sperrt der Ruhetimer sofort neu */
  if (scr_main && !theme_dirty) {
    main_refresh();
    lv_screen_load_anim(scr_main, LV_SCR_LOAD_ANIM_MOVE_BOTTOM,
                        ANIM_MS, 0, false);
  } else {
    ui_build_main();          /* Farben haben sich geaendert */
  }
}

/* Lockscreen: nach Ruhe von selbst oder durch Wischen nach oben.
   Zeigt Zeit, Level und Titel - man weiss beim Vorbeigehen, wo man
   steht, ohne etwas anzufassen. Beruehrung fuehrt zurueck.         */
static void ui_build_lock(void)
{
  locked  = true;
  sel_idx = -1;

  lv_obj_t *old = scr_clock;
  scr_clock = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(scr_clock, C(COL_SCR), 0);
  lv_obj_set_style_pad_all(scr_clock, 0, 0);
  lv_obj_set_style_border_width(scr_clock, 0, 0);
  lv_obj_remove_flag(scr_clock, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(scr_clock, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(scr_clock, lock_press_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(scr_clock, press_cb, LV_EVENT_PRESSED, NULL);

  clock_time = lv_label_create(scr_clock);
  lv_obj_set_style_text_font(clock_time, LB_FONT_HUGE, 0);
  lv_obj_set_style_text_color(clock_time, C(COL_TEXT), 0);
  lv_obj_align(clock_time, LV_ALIGN_LEFT_MID, 28, -10);

  clock_day = lv_label_create(scr_clock);
  lv_obj_set_style_text_color(clock_day, C(COL_DIM), 0);
  lv_obj_align(clock_day, LV_ALIGN_LEFT_MID, 30, 28);

  uint16_t lvl;
  uint32_t have, need;
  level_from_total(total_min, &lvl, &have, &need);
  uint32_t rc = rarity_color(lvl);

  lv_obj_t *ll = lv_label_create(scr_clock);
  lv_obj_set_style_text_font(ll, LB_FONT_BIG, 0);
  lv_obj_set_style_text_color(ll, C(rc), 0);
  lv_obj_align(ll, LV_ALIGN_RIGHT_MID, -28, -24);
  lv_label_set_text_fmt(ll, "LVL %u", lvl);

  lv_obj_t *tl = lv_label_create(scr_clock);
  lv_obj_set_style_text_font(tl, LB_FONT_BIG, 0);
  lv_obj_set_style_text_color(tl, C(rc), 0);
  lv_obj_align(tl, LV_ALIGN_RIGHT_MID, -28, 2);
  lv_label_set_text(tl, current_class());

  lv_obj_t *xl = lv_label_create(scr_clock);
  lv_obj_set_style_text_color(xl, C(COL_DIM), 0);
  lv_obj_align(xl, LV_ALIGN_RIGHT_MID, -28, 28);
  lv_label_set_text_fmt(xl, "%lu / %lu EXP",
                        (unsigned long)have, (unsigned long)need);

  clock_refresh();
  lv_screen_load_anim(scr_clock, LV_SCR_LOAD_ANIM_MOVE_TOP, ANIM_MS, 0, false);
  if (old) lv_async_call(del_async, old);
}

/* =====================================================================
   Wochenrueckblick  (Wischen nach unten)
   ===================================================================== */

/* In der Wochenansicht fuehrt ein weiteres Wischen nach unten eine
   Ebene tiefer in die Einstellungen. Nach oben geht es zurueck.    */
static void week_gesture_cb(lv_event_t *e)
{
  LV_UNUSED(e);
  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());

  if (press_x < 0) return;

  if (dir == LV_DIR_BOTTOM && edge_ok(dir, press_x, press_y)) {
    gesture_block = lv_tick_get();
    press_x = -1;
    lv_indev_wait_release(lv_indev_active());
    lv_async_call(rebuild_async, (void *)settings_show);
    return;
  }
  back_gesture_cb(e);
}

static void ui_build_week(void)
{
  lv_obj_t *sc = sub_screen(&scr_week,  LV_DIR_TOP);
  lv_obj_add_event_cb(sc, week_gesture_cb, LV_EVENT_GESTURE, NULL);
  char b1[16], b2[16];

  uint32_t tw = week_total();
  uint32_t pw = prev_week_total();

  lv_obj_t *head = lv_label_create(sc);
  lv_obj_set_style_text_font(head, LB_FONT_BIG, 0);
  lv_obj_set_style_text_color(head, C(COL_TEXT), 0);
  lv_obj_set_pos(head, 12, 4);
  fmt_h(b1, sizeof(b1), tw);
  fmt_h(b2, sizeof(b2), pw);
  lv_label_set_text_fmt(head, T(S_THIS_WEEK), b1, b2);

  /* Siegel: eine Marke je Woche in Folge mit erreichtem Ziel */
  lv_obj_t *seal = lv_label_create(sc);
  lv_obj_set_style_text_color(seal, C(0xFF8000), 0);
  lv_obj_align(seal, LV_ALIGN_TOP_RIGHT, -12, 6);
  if (st.seals == 0) {
    lv_label_set_text(seal, "");
  } else {
    char s[64] = "";
    for (int i = 0; i < st.seals && i < 8; i++) strcat(s, LV_SYMBOL_OK " ");
    lv_label_set_text_fmt(seal, "%s(%d)", s, st.seals);
  }

  /* Zielbalken */
  lv_obj_t *goal = lv_bar_create(sc);
  lv_obj_set_size(goal, 616, 14);
  lv_obj_set_pos(goal, 12, 30);
  lv_obj_set_style_bg_color(goal, C(COL_CARD), 0);
  lv_obj_set_style_bg_color(goal, C(
      tw >= st.week_goal ? 0x1EFF00 : COL_HIT), LV_PART_INDICATOR);
  lv_bar_set_range(goal, 0, st.week_goal ? st.week_goal : 1);
  lv_bar_set_value(goal, (int32_t)(tw > st.week_goal ? st.week_goal : tw),
                   LV_ANIM_OFF);

  /* Bonus: 25 Prozent der Minuten ueber dem Ziel */
  lv_obj_t *bo = lv_label_create(sc);
  lv_obj_align(bo, LV_ALIGN_TOP_RIGHT, -12, 28);
  uint32_t bp = week_bonus_preview();
  if (bp) {
    lv_obj_set_style_text_color(bo, C(0x1EFF00), 0);
    lv_label_set_text_fmt(bo, T(S_BONUS), (unsigned long)bp);
  } else if (last_bonus) {
    lv_obj_set_style_text_color(bo, C(COL_DIM), 0);
    lv_label_set_text_fmt(bo, T(S_LAST_BONUS),
                          (unsigned long)last_bonus);
  } else {
    lv_label_set_text(bo, "");
  }

  /* Links die guten Module, rechts die Bad Habits */
  int y = 56;
  for (int i = 0; i < MOD_COUNT; i++) {
    if (!mod_active(i)) continue;

    lv_obj_t *nm = lv_label_create(sc);
    lv_obj_set_style_text_color(nm, C(COL_DIM), 0);
    lv_obj_set_pos(nm, 24, y);
    lv_label_set_text(nm, mod_name(i));

    lv_obj_t *vl = lv_label_create(sc);
    lv_obj_set_pos(vl, 150, y);
    fmt_h(b1, sizeof(b1), (*mod_week_p(i)));
    lv_obj_set_style_text_color(vl, C(COL_TEXT), 0);
    lv_label_set_text(vl, b1);

    int d = (int)(*mod_week_p(i)) - (int)(*mod_prev_p(i));
    lv_obj_t *dl = lv_label_create(sc);
    lv_obj_set_pos(dl, 226, y);
    lv_obj_set_style_text_color(dl, C(
        d > 0 ? 0x1EFF00 : (d < 0 ? COL_WARN : COL_DIM)), 0);
    if (d == 0) lv_label_set_text(dl, T(S_NOCHANGE));
    else        lv_label_set_text_fmt(dl, "%s%d.%c h", d > 0 ? "+" : "-",
                                      abs(d) / 60, (abs(d) % 60) ? '5' : '0');
    y += 19;
    if (y > 150) break;
  }

  y = 56;
  for (int i = 0; i < BAD_COUNT; i++) {
    if (!bad_active(i)) continue;

    lv_obj_t *nm = lv_label_create(sc);
    lv_obj_set_style_text_color(nm, C(COL_DIM), 0);
    lv_obj_set_pos(nm, 340, y);
    lv_label_set_text(nm, bad_name(i));

    lv_obj_t *vl = lv_label_create(sc);
    lv_obj_set_pos(vl, 500, y);
    lv_obj_set_style_text_color(vl, C(COL_TEXT), 0);
    lv_label_set_text_fmt(vl, "%u x", (*bad_week_p(i)));

    /* Bei Bad Habits ist weniger besser - Farben gedreht */
    int d = (int)(*bad_week_p(i)) - (int)(*bad_prev_p(i));
    lv_obj_t *dl = lv_label_create(sc);
    lv_obj_set_pos(dl, 560, y);
    lv_obj_set_style_text_color(dl, C(
        d < 0 ? 0x1EFF00 : (d > 0 ? COL_WARN : COL_DIM)), 0);
    if (d == 0) lv_label_set_text(dl, T(S_NOCHANGE));
    else        lv_label_set_text_fmt(dl, "%s%d", d > 0 ? "+" : "", d);
    y += 19;
    if (y > 150) break;
  }

  /* Luecke benennen, damit der Vergleich mit "letzte Woche"
     nachvollziehbar bleibt.                                        */
  if (week_gap) {
    lv_obj_t *gl = lv_label_create(sc);
    lv_obj_set_style_text_color(gl, C(COL_WARN), 0);
    lv_obj_align(gl, LV_ALIGN_BOTTOM_LEFT, 24, -4);
    lv_label_set_text_fmt(gl, T(S_GAP), (unsigned long)week_gap);
    week_gap  = 0;
    gap_dirty = true;
  }

  st.review_pending = 0;
  store_mark_dirty();
  lv_screen_load_anim(sc, back_anim, ANIM_MS, 0, false);
}

/* =====================================================================
   Einstieg
   ===================================================================== */

/* Laeuft im LVGL-Task: haelt die Uhr aktuell, ohne loop() zu belasten */
#define IDLE_LOCK_MS 180000UL      /* 3 min ohne Beruehrung */

static void ui_timer_cb(lv_timer_t *t)
{
  LV_UNUSED(t);

  if (locked && scr_clock && lv_screen_active() == scr_clock) clock_refresh();

  /* Ueberall sperren, ausser wo der Nutzer gerade nebenbei etwas
     erledigt: Wizard und WLAN-Einrichtung bleiben stehen.           */
  if (!locked && last_touch &&
      (lv_tick_get() - last_touch) > IDLE_LOCK_MS) {
    lv_obj_t *act = lv_screen_active();
    if (act && act != scr_wizard && act != scr_net && act != scr_msg)
      ui_build_lock();
  }

  /* Nachtmodus greift erst beim naechsten Zeichnen, also Seite neu
     aufbauen, wenn er umschlaegt.                                   */
  bool n = night_check();
  if (n != night_now) {
    night_now   = n;
    theme_dirty = true;

    lv_obj_t *act = lv_screen_active();
    if (locked && act == scr_clock)      ui_build_lock();
    else if (scr_main && act == scr_main) ui_build_main();
  }
}

#ifndef UI_DEBUG
#define UI_DEBUG 0
#endif
#if UI_DEBUG
  #define UISTEP(n, txt) Serial.printf("[ui %d] %s\n", n, txt)
#else
  #define UISTEP(n, txt) do {} while (0)
#endif

static void ui_init(void)
{
  UISTEP(1, "start");
  night_on  = prefs.getBool("night", true);
  clock12   = prefs.getBool("clock12", false);
  UISTEP(2, "prefs gelesen");

  night_now = night_check();
  UISTEP(3, "nachtmodus geprueft");

  lv_timer_create(ui_timer_cb, 1000, NULL);
  UISTEP(4, "timer");

  if (!st.configured) {
    UISTEP(5, "-> wizard");
    wizard_open(0);
  } else if (!net_has_creds() && !st.net_skipped) {
    UISTEP(5, "-> wlan");
    ui_build_net();
  } else {
    UISTEP(5, "-> hauptseite");
    ui_build_main();
  }
  UISTEP(6, "fertig");
}

/* Aus loop(): WLAN einmal trennen, Zugangsdaten verwerfen, Hotspot
   oeffnen. Genau einmal, nicht in jedem Durchlauf.                */
static void ui_wifi_tick(void)
{
  if (!wifi_reset_pending) return;
  wifi_reset_pending = false;

  net_reset_and_portal();
  if (lvgl_lock(500)) { ui_build_net(); lvgl_unlock(); }
}

/* Aus loop(): Werksreset ausserhalb des UI-Tasks ausfuehren. */
static void ui_reset_tick(void)
{
  if (!reset_pending) return;
  reset_pending = false;

  store_reset();
  log_clear();
  view_bad = false;
  sel_idx  = -1;
  wiz_step = 0;
  wiz_mode = 0;
  if (lvgl_lock(500)) { ui_init(); lvgl_unlock(); }
}

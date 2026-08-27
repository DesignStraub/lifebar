#pragma once
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <time.h>
#include <ESPmDNS.h>
#include "esp_heap_caps.h"
#include "lifebar_model.h"
#include "lifebar_log.h"
#include "lifebar_page.h"

/* =====================================================================
   LifeBar - Netzwerk

   Eigenes Captive Portal statt WiFiManager: softAP + DNSServer +
   WebServer stecken alle schon im ESP32-Core. Kein Fremdpaket, das
   beim naechsten Core-Update umfaellt.

   Auf dem Geraet wird nie ein Passwort getippt - der Nutzer verbindet
   sich mit dem Hotspot, das Portal springt am Handy auf.
   ===================================================================== */

#define NTP_SERVER_1  "pool.ntp.org"
#define NTP_SERVER_2  "time.cloudflare.com"
/* Numerisch als dritter Server: greift auch, wenn der Router DNS
   verbiegt oder blockiert.                                          */
#define NTP_SERVER_3  "162.159.200.123"
/* Mitteleuropa inkl. Sommerzeitregel */
#define TZ_RULE       "CET-1CEST,M3.5.0,M10.5.0/3"

#define CONNECT_TIMEOUT  12000UL

#ifndef USE_WEB
#define USE_WEB 1
#endif
#ifndef USE_MDNS
#define USE_MDNS 1
#endif
#define PORTAL_TIMEOUT   600000UL   /* 10 min ohne Einrichtung -> aus  */
#define RESYNC_EVERY     43200000UL /* alle 12 h die Uhr nachstellen   */

enum {
  NET_IDLE = 0,     /* nichts hinterlegt, nichts aktiv */
  NET_CONNECTING,   /* versucht gerade zu verbinden    */
  NET_PORTAL,       /* Hotspot + Portal laufen         */
  NET_ONLINE        /* verbunden                       */
};

static uint8_t   net_state    = NET_IDLE;
static char      ap_ssid[24]  = "LifeBar";
static DNSServer dns;
static WebServer web(80);
static bool      portal_up    = false;
static uint32_t  connect_t0   = 0;
static bool      creds_pending    = false;   /* neu gespeichert, sofort testen */
static bool      last_try_failed  = false;
static uint32_t  next_retry       = 0;
static uint32_t  portal_stop_at   = 0;       /* Nachlauf nach Erfolg */
static uint32_t  time_check_at    = 0;       /* naechster Blick auf die Uhr */
static uint8_t   ntp_rounds       = 0;
static uint32_t  portal_started   = 0;
static uint32_t  next_resync      = 0;
static bool      web_started      = false;
static bool      mdns_started     = false;

/* ---- Zugangsdaten ---------------------------------------------------- */

static bool net_has_creds(void)
{
  return prefs.getString("ssid", "").length() > 0;
}

static void net_clear_creds(void)
{
  prefs.remove("ssid");
  prefs.remove("pass");
}

/* ---- Zeit ------------------------------------------------------------ */

static bool net_time_ok(void)
{
  struct tm t;
  if (!getLocalTime(&t, 50)) return false;
  return (t.tm_year + 1900) >= 2025;     /* 1970/2000 = ungestellt */
}

/* NTP nur anstossen - der SNTP-Dienst laeuft danach im Hintergrund
   weiter. Auf die Antwort wird NICHT gewartet, das kann 5 bis 15
   Sekunden dauern und wuerde den loop() blockieren.                */
static void net_sync_time(void)
{
  configTzTime(TZ_RULE, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
  ntp_rounds    = 0;
  time_check_at = millis() + 1000;
  Serial.println("[net] NTP angestossen");
}

/* Wird aus net_tick gerufen, bis die Uhr steht. */
static bool net_time_tick(void)
{
  if (time_valid)                 return false;
  if (net_state != NET_ONLINE)    return false;   /* im Schlaf nichts zu tun */
  if (millis() < time_check_at)   return false;

  time_check_at = millis() + 1000;

  if (net_time_ok()) {
    time_valid = true;
    struct tm t;
    getLocalTime(&t);
    Serial.printf("[net] Zeit steht: %04d-%02d-%02d %02d:%02d\n",
                  t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                  t.tm_hour, t.tm_min);
    st.last_seen = (uint32_t)time(NULL);
    store_mark_dirty();
    log_fix_times();              /* undatierte Eintraege nachtragen */
    return true;                  /* UI aktualisieren */
  }

  /* alle 20 Sekunden neu anstossen, maximal dreimal */
  if (++ntp_rounds % 20 == 0 && ntp_rounds <= 60) {
    Serial.printf("[net] NTP noch keine Antwort (%d s), neuer Versuch\n",
                  ntp_rounds);
    configTzTime(TZ_RULE, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
  }
  return false;
}

/* ---- Portal ---------------------------------------------------------- */

static void net_make_ap_name(void)
{
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(ap_ssid, sizeof(ap_ssid), "LifeBar-%02X%02X", mac[4], mac[5]);
}

static void handle_root(void)
{
  if (!portal_up) {          /* verbunden: Auswertungsseite */
    web.send_P(200, "text/html", PAGE_HTML);
    return;
  }

  int n = WiFi.scanNetworks();

  String p = F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
               "<meta name='viewport' content='width=device-width,initial-scale=1'>"
               "<title>LifeBar</title><style>"
               "body{background:#0e0e12;color:#f2f2f7;font-family:system-ui,sans-serif;"
               "margin:0;padding:24px}h1{font-size:20px;margin:0 0 4px}"
               "p{color:#8a8a99;font-size:14px;margin:0 0 20px}"
               "label{display:block;font-size:13px;color:#8a8a99;margin:14px 0 6px}"
               "select,input{width:100%;box-sizing:border-box;padding:12px;"
               "border-radius:8px;border:1px solid #2a2a34;background:#1c1c24;"
               "color:#f2f2f7;font-size:16px}"
               "button{width:100%;margin-top:22px;padding:14px;border:0;"
               "border-radius:8px;background:#2e6bff;color:#fff;font-size:16px}"
               "</style></head><body><h1>LifeBar</h1>"
               "<p>Waehle dein WLAN. Die Zugangsdaten bleiben auf dem Geraet.</p>"
               "<form action='/save' method='POST'>"
               "<label>Netzwerk</label><select name='ssid'>");

  for (int i = 0; i < n; i++) {
    p += "<option value='" + WiFi.SSID(i) + "'>" + WiFi.SSID(i) +
         "  (" + String(WiFi.RSSI(i)) + " dBm)</option>";
  }
  if (n == 0) p += F("<option value=''>kein Netz gefunden</option>");

  p += F("</select><label>Passwort</label>"
         "<input type='password' name='pass' autocomplete='off'>"
         "<button type='submit'>Verbinden</button></form></body></html>");

  web.send(200, "text/html", p);
  WiFi.scanDelete();
}

static void handle_save(void)
{
  String ssid = web.arg("ssid");
  String pass = web.arg("pass");

  if (ssid.length() == 0) { web.sendHeader("Location", "/"); web.send(302); return; }

  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  creds_pending   = true;      /* net_tick verbindet beim naechsten Durchlauf */
  last_try_failed = false;

  web.send(200, "text/html",
    F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<style>body{background:#0e0e12;color:#f2f2f7;font-family:system-ui,"
      "sans-serif;padding:24px}h2{margin:0 0 12px}"
      "#s{font-size:16px;color:#8a8a99;margin-top:18px}"
      ".ok{color:#1eff00!important}.err{color:#ff4d4d!important}"
      "a{color:#2e6bff}</style></head><body>"
      "<h2>Verbinde...</h2><div id='s'>Das kann ein paar Sekunden dauern.</div>"
      "<script>"
      "let n=0;"
      "function p(){n++;fetch('/status').then(r=>r.text()).then(t=>{"
      "let e=document.getElementById('s');"
      "if(t.startsWith('ok')){e.className='ok';"
      "e.innerHTML='Verbunden. IP '+t.slice(3)+'.<br>Weiter mit "
      "<a href=\"/setup\">Skills und Laster einrichten</a>, "
      "danach im Heimnetz erreichbar.';return;}"
      "if(t=='fail'){e.className='err';"
      "e.textContent='Verbindung fehlgeschlagen. Passwort pruefen und "
      "Seite neu laden.';return;}"
      "if(n<40)setTimeout(p,1000);"
      "else{e.className='err';e.textContent='Keine Rueckmeldung.';}"
      "});}"
      "setTimeout(p,1200);"
      "</script></body></html>"));
}

static void handle_status(void)
{
  if (net_state == NET_ONLINE) {
    web.send(200, "text/plain", "ok:" + WiFi.localIP().toString());
  } else if (last_try_failed) {
    web.send(200, "text/plain", "fail");
  } else {
    web.send(200, "text/plain", "wait");
  }
}

/* GET /events            -> alle Eintraege als CSV
   GET /events?since=<ts> -> nur neuere als dieser Zeitstempel

   Wird gestreamt, damit auch ein volles Log nicht in den RAM muss. */
static void handle_events(void)
{
  uint32_t since = 0;
  if (web.hasArg("since")) since = (uint32_t)strtoul(web.arg("since").c_str(),
                                                     NULL, 10);

  web.sendHeader("Content-Disposition", "inline; filename=events.csv");
  web.setContentLength(CONTENT_LENGTH_UNKNOWN);
  web.send(200, "text/csv", "");
  web.sendContent("ts,type,name,delta\n");

  File f = FFat.open(LOG_PATH, "r");
  if (!f) { web.sendContent(""); return; }

  const int BATCH = 64;
  lb_event_t buf[BATCH];
  String out;
  out.reserve(2048);

  while (f.available() >= (int)sizeof(lb_event_t)) {
    int n = f.read((uint8_t *)buf, sizeof(buf)) / sizeof(lb_event_t);
    out = "";
    for (int i = 0; i < n; i++) {
      uint32_t ts = (buf[i].ts & TS_REL) ? 0 : buf[i].ts;
      if (ts <= since) continue;
      out += String(ts);
      out += (buf[i].type == EV_SKILL) ? ",skill," : ",bad,";
      out += log_name(buf[i].type, buf[i].idx);
      out += ",";
      out += String(buf[i].delta);
      out += "\n";
    }
    if (out.length()) web.sendContent(out);
  }
  f.close();
  web.sendContent("");        /* Ende des Chunked-Streams */
}

/* Kurzer Statusblock als JSON, damit die Auswertung weiss, womit
   sie es zu tun hat.                                              */
static void handle_info(void)
{
  uint16_t lvl;
  uint32_t have, need;
  level_from_total(total_min, &lvl, &have, &need);

  String j = "{";
  j += "\"device\":\"lifebar\",";
  j += "\"events\":"     + String(log_count) + ",";
  j += "\"log_ready\":"  + String(log_ready ? "true" : "false") + ",";
  j += "\"log_error\":\"" + String(log_error) + "\",";
  j += "\"fs_used\":"    + String((unsigned)(log_ready ? FFat.usedBytes() : 0)) + ",";
  j += "\"fs_total\":"   + String((unsigned)(log_ready ? FFat.totalBytes() : 0)) + ",";
  j += "\"time_valid\":" + String(time_valid ? "true" : "false") + ",";
  j += "\"now\":"        + String((uint32_t)(time_valid ? time(NULL) : 0)) + ",";
  j += "\"total_min\":"  + String((unsigned long)total_min) + ",";
  j += "\"level\":"      + String((unsigned)lvl) + ",";
  j += "\"have\":"       + String((unsigned long)have) + ",";
  j += "\"need\":"       + String((unsigned long)need) + ",";
  j += "\"bonus\":"      + String((unsigned long)bonus_min) + ",";
  j += "\"week_min\":"   + String((unsigned long)week_total()) + ",";
  j += "\"week_goal\":"  + String(st.week_goal) + ",";
  j += "\"seals\":"      + String(st.seals) + ",";
  j += "\"lang\":\""    + String(LANG_TAG[lang]) + "\",";
  j += "\"clock12\":"  + String(clock12 ? "true" : "false") + ",";
  j += "\"class\":\""   + String(current_class()) + "\",";

  /* Echte Summen aus dem NVS - das Log kennt nur, was seit seiner
     Einrichtung passiert ist.                                       */
  j += "\"skills\":[";
  bool first = true;
  for (int i = 0; i < PRESET_COUNT; i++) {
    if (!st.minutes[i]) continue;
    if (!first) j += ",";
    first = false;
    j += "{\"n\":\"" + String(preset[i].name[lang]) + "\",\"m\":" +
         String((unsigned long)st.minutes[i]) + "}";
  }
  j += "],\"vices\":[";
  first = true;
  for (int i = 0; i < BAD_PRESET_COUNT; i++) {
    if (!st.bad_total[i]) continue;
    if (!first) j += ",";
    first = false;
    j += "{\"n\":\"" + String(bad_preset[i].name[lang]) + "\",\"c\":" +
         String((unsigned long)st.bad_total[i]) + "}";
  }
  j += "],";

  /* Aktive Slots - Grundlage fuer die Buchungsflaechen im Browser */
  j += "\"slots\":[";
  first = true;
  for (int i = 0; i < MOD_COUNT; i++) {
    if (!mod_active(i)) continue;
    if (!first) j += ",";
    first = false;
    j += "{\"i\":" + String(i) +
         ",\"n\":\"" + String(mod_name(i)) + "\"" +
         ",\"m\":" + String((unsigned long)(*mod_min_p(i))) + "}";
  }
  j += "],\"vslots\":[";
  first = true;
  for (int i = 0; i < BAD_COUNT; i++) {
    if (!bad_active(i)) continue;
    if (!first) j += ",";
    first = false;
    j += "{\"i\":" + String(i) +
         ",\"n\":\"" + String(bad_name(i)) + "\"" +
         ",\"c\":" + String((unsigned long)(*bad_tot_p(i))) + "}";
  }
  j += "],";

  /* Titelchronik, jüngste Woche zuerst */
  j += "\"titles\":[";
  first = true;
  for (int k = 1; k <= WEEK_HIST; k++) {
    int p = (st.week_hist_pos - k + WEEK_HIST * 2) % WEEK_HIST;
    uint8_t nn = st.t_hist[p][0], pp = st.t_hist[p][1];
    uint8_t raw = st.t_hist[p][2];
    uint8_t ll  = raw & 0x0F;          /* Leitwort */
    uint8_t tr  = (raw >> 4) & 0x03;   /* Evolutionsstufe */
    if (nn >= PRESET_COUNT) continue;
    if (!first) j += ",";
    first = false;
    String t = "";
    if (ll > 0 && ll < LEAD_COUNT) t += String(LEAD[ll][lang]) + " ";
    if (pp != nn && pp < PRESET_COUNT)
      t += String(preset[pp].pre[lang]) + String(TITLE_SEP[lang]);
    t += String(preset[nn].title[tr][lang]);
    j += "\"" + t + "\"";
  }
  j += "]";
  j += "}";
  web.send(200, "application/json", j);
}


/* =====================================================================
   Export, Import und Nachtragen
   ===================================================================== */

static const char *PAGE_CSS =
  "body{background:#0e0e12;color:#f2f2f7;font-family:system-ui,sans-serif;"
  "margin:0;padding:22px;max-width:760px}"
  "h1{font-size:19px;margin:0 0 4px}"
  "h2{font-size:12px;letter-spacing:.14em;color:#7a7a88;margin:26px 0 8px;"
  "text-transform:uppercase}"
  "textarea{width:100%;height:240px;background:#15151c;color:#f2f2f7;"
  "border:1px solid #2a2a34;border-radius:8px;padding:12px;font-size:13px;"
  "font-family:ui-monospace,monospace}"
  "select,input{width:100%;box-sizing:border-box;padding:11px;"
  "border-radius:8px;background:#1c1c24;color:#f2f2f7;"
  "border:1px solid #2a2a34;font-size:16px;margin:5px 0}"
  "button{width:100%;margin-top:16px;padding:14px;border:0;border-radius:8px;"
  "background:#2e6bff;color:#fff;font-size:16px}"
  "p{color:#7a7a88;font-size:14px}a{color:#2e6bff}"
  ".row{display:flex;gap:10px}.row>*{flex:1}";

/* Textformat statt JSON: von Hand lesbar, von Hand editierbar, und
   ohne Parser-Bibliothek zuverlaessig einzulesen.                  */

/* Eine datierte Buchung anwenden: Summen, passende Woche, Histogramm
   und Ereignisprotokoll. Wird von /add und vom Import genutzt.
   amount = Minuten bei Skills, Anzahl bei Lastern.                 */
static bool apply_entry(bool is_bad, int idx, int32_t amount, uint32_t when)
{
  if (amount <= 0) return false;

  struct tm tv;
  time_t tt = (time_t)when;
  localtime_r(&tt, &tv);

  if (is_bad) {
    if (idx < 0 || idx >= BAD_PRESET_COUNT) return false;
    st.bad_total[idx] += amount;
    if (when >= st.week_start)                 st.bad_week[idx] += amount;
    else if (when >= st.week_start - 604800UL) st.bad_prev[idx] += amount;
    log_add_at(when, EV_BAD, (uint8_t)idx, (int16_t)amount);
    return true;
  }

  if (idx < 0 || idx >= PRESET_COUNT) return false;
  amount = (amount / STEP_MIN) * STEP_MIN;
  if (amount <= 0) return false;

  st.minutes[idx] += amount;
  st.rec_min[idx] += amount;
  if (when >= st.week_start)                 st.week_min[idx]  += amount;
  else if (when >= st.week_start - 604800UL) st.prev_week[idx] += amount;
  if (st.hour_w[tv.tm_hour] < 60000) st.hour_w[tv.tm_hour]++;
  log_add_at(when, EV_SKILL, (uint8_t)idx, (int16_t)amount);
  return true;
}

static uint32_t parse_when(const String &d, const String &t)
{
  struct tm tv = {};
  tv.tm_year  = d.substring(0, 4).toInt() - 1900;
  tv.tm_mon   = d.substring(5, 7).toInt() - 1;
  tv.tm_mday  = d.substring(8, 10).toInt();
  tv.tm_hour  = t.length() >= 5 ? t.substring(0, 2).toInt() : 20;
  tv.tm_min   = t.length() >= 5 ? t.substring(3, 5).toInt() : 0;
  tv.tm_isdst = -1;
  return (uint32_t)mktime(&tv);
}

static String export_text(void)
{
  String t = F("# LifeBar Export\n");
  t += "lang="      + String(LANG_TAG[lang]) + "\n";
  t += "week_goal=" + String(st.week_goal) + "\n";
  t += "bonus="     + String((unsigned long)bonus_min) + "\n";
  t += F("# skill,Name,Minuten gesamt\n");
  for (int i = 0; i < PRESET_COUNT; i++)
    if (st.minutes[i])
      t += "skill," + String(preset[i].name[LANG_EN]) + "," +
           String((unsigned long)st.minutes[i]) + "\n";
  t += F("# vice,Name,Anzahl gesamt\n");
  for (int i = 0; i < BAD_PRESET_COUNT; i++)
    if (st.bad_total[i])
      t += "vice," + String(bad_preset[i].name[LANG_EN]) + "," +
           String((unsigned long)st.bad_total[i]) + "\n";
  return t;
}


/* Wird von loop() abgearbeitet: Oberflaeche nach einer Aenderung
   ueber den Browser neu aufbauen.                                 */
static bool setup_applied = false;

/* Buchung kam ueber den Browser - loop() zeichnet die Hauptseite neu */
static volatile bool ui_refresh_pending = false;

/* GET /setup - Einrichtung vom Handy aus, ohne Touch am Geraet. */
static void handle_setup(void)
{
  String p = F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
               "<meta name='viewport' content='width=device-width,initial-scale=1'>"
               "<title>LifeBar Setup</title><style>");
  p += PAGE_CSS;
  p += F("label{display:flex;align-items:center;gap:10px;padding:9px 10px;"
         "background:#1c1c24;border-radius:8px;margin:6px 0;font-size:15px}"
         "input[type=checkbox]{width:20px;height:20px;margin:0}"
         "</style></head><body><h1>Einrichtung</h1>"
         "<form action='/setup' method='POST'><h2>Sprache</h2>"
         "<select name='lang'>");
  for (int i = 0; i < LANG_COUNT; i++)
    p += "<option value='" + String(i) + "'" +
         (i == lang ? " selected" : "") + ">" + LANG_NAME[i] + "</option>";

  p += F("</select><h2>Skills (bis zu 6)</h2>");
  for (int i = 0; i < PRESET_COUNT; i++) {
    bool on = false;
    for (int k = 0; k < MOD_COUNT; k++) if (st.slot[k] == i) on = true;
    p += "<label><input type='checkbox' name='s" + String(i) + "'" +
         (on ? " checked" : "") + ">" + preset[i].name[lang] + "</label>";
  }

  p += F("<h2>Laster (bis zu 6)</h2>");
  for (int i = 0; i < BAD_PRESET_COUNT; i++) {
    bool on = false;
    for (int k = 0; k < BAD_COUNT; k++) if (st.bad_slot[k] == i) on = true;
    p += "<label><input type='checkbox' name='v" + String(i) + "'" +
         (on ? " checked" : "") + ">" + bad_preset[i].name[lang] + "</label>";
  }

  p += F("<button type='submit'>Speichern</button></form>"
         "<h2>Weiteres</h2><p><a href='/add'>Zeit nachtragen</a><br>"
         "<a href='/import'>Import und Export</a><br>"
         "<a href='/'>Uebersicht</a></p></body></html>");
  web.send(200, "text/html", p);
}

static void handle_setup_post(void)
{
  if (web.hasArg("lang")) {
    int l = web.arg("lang").toInt();
    if (l >= 0 && l < LANG_COUNT) { lang = (uint8_t)l; prefs.putUChar("lang", lang); }
  }

  int n = 0;
  for (int i = 0; i < PRESET_COUNT && n < MOD_COUNT; i++)
    if (web.hasArg(("s" + String(i)).c_str())) st.slot[n++] = (uint8_t)i;
  while (n < MOD_COUNT) st.slot[n++] = SLOT_EMPTY;

  n = 0;
  for (int i = 0; i < BAD_PRESET_COUNT && n < BAD_COUNT; i++)
    if (web.hasArg(("v" + String(i)).c_str())) st.bad_slot[n++] = (uint8_t)i;
  while (n < BAD_COUNT) st.bad_slot[n++] = SLOT_EMPTY;

  st.configured = 1;
  recalc_total();
  store_save_now();
  setup_applied = true;

  String p = F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
               "<meta http-equiv='refresh' content='2; url=/'><style>");
  p += PAGE_CSS;
  p += F("</style></head><body><h1>Gespeichert</h1>"
         "<p>Das Geraet uebernimmt die Auswahl.</p></body></html>");
  web.send(200, "text/html", p);
}

static void handle_export(void)
{
  web.sendHeader("Content-Disposition", "attachment; filename=lifebar.txt");
  web.send(200, "text/plain", export_text());
}

static void handle_import_get(void)
{
  String p = F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
               "<meta name='viewport' content='width=device-width,initial-scale=1'>"
               "<title>LifeBar Import</title><style>");
  p += PAGE_CSS;
  p += F("</style></head><body><h1>Import</h1>"
         "<p>Werte ueberschreiben den aktuellen Stand. Zeilen mit # werden "
         "ignoriert. Nicht genannte Module bleiben unveraendert.</p>"
         "<form action='/import' method='POST'><textarea name='data'>");
  p += export_text();
  p += F("</textarea><button type='submit'>Uebernehmen</button></form>"
         "<h2>Weiteres</h2><p><a href='/export'>Export herunterladen</a><br>"
         "<a href='/add'>Zeit nachtragen</a><br>"
         "<a href='/'>Uebersicht</a></p></body></html>");
  web.send(200, "text/html", p);
}

static void handle_import_post(void)
{
  String d = web.arg("data");
  int applied = 0, unknown = 0;

  int pos = 0;
  while (pos < (int)d.length()) {
    int nl = d.indexOf('\n', pos);
    if (nl < 0) nl = d.length();
    String line = d.substring(pos, nl);
    pos = nl + 1;
    line.trim();
    if (!line.length() || line.startsWith("#")) continue;

    if (line.startsWith("lang=")) {
      String v = line.substring(5); v.trim();
      for (int i = 0; i < LANG_COUNT; i++)
        if (v == LANG_TAG[i]) { lang = (uint8_t)i; prefs.putUChar("lang", lang); }
      applied++; continue;
    }
    if (line.startsWith("week_goal=")) {
      long v = line.substring(10).toInt();
      if (v >= 60 && v <= 4200) st.week_goal = (uint16_t)v;
      applied++; continue;
    }
    if (line.startsWith("bonus=")) {
      bonus_min = (uint32_t)line.substring(6).toInt();
      prefs.putUInt("bonus", bonus_min);
      applied++; continue;
    }

    /* event,JJJJ-MM-TT,HH:MM,skill|vice,Name,Menge
       Menge = Minuten bei Skills, Anzahl bei Lastern. Wird addiert,
       nicht gesetzt - und landet mit Zeitstempel im Protokoll.     */
    if (line.startsWith("event,")) {
      String f[5];
      int at = 6, k = 0;
      while (k < 5) {
        int c = line.indexOf(',', at);
        if (c < 0) { f[k++] = line.substring(at); break; }
        f[k++] = line.substring(at, c);
        at = c + 1;
      }
      if (k < 5) { unknown++; continue; }
      for (int q = 0; q < 5; q++) f[q].trim();
      bool bad = (f[2] == "vice");
      int  i   = bad ? bad_by_name(f[3].c_str()) : preset_by_name(f[3].c_str());
      if (i < 0) { unknown++; continue; }
      if (apply_entry(bad, i, f[4].toInt(), parse_when(f[0], f[1]))) applied++;
      else unknown++;
      continue;
    }

    int c1 = line.indexOf(','), c2 = line.indexOf(',', c1 + 1);
    if (c1 < 0 || c2 < 0) { unknown++; continue; }
    String kind = line.substring(0, c1);
    String name = line.substring(c1 + 1, c2); name.trim();
    long   val  = line.substring(c2 + 1).toInt();
    if (val < 0) { unknown++; continue; }

    if (kind == "skill") {
      int i = preset_by_name(name.c_str());
      if (i < 0) { unknown++; continue; }
      st.minutes[i] = (uint32_t)val;
      applied++;
    } else if (kind == "vice") {
      int i = bad_by_name(name.c_str());
      if (i < 0) { unknown++; continue; }
      st.bad_total[i] = (uint32_t)val;
      applied++;
    } else unknown++;
  }

  recalc_total();
  store_mark_dirty();
  setup_applied = true;          /* Oberflaeche neu aufbauen */

  String p = F("<!DOCTYPE html><html><head><meta charset='utf-8'><style>");
  p += PAGE_CSS;
  p += F("</style></head><body><h1>Uebernommen</h1><p>");
  p += String(applied) + F(" Zeilen angewendet, ") + String(unknown) +
       F(" nicht erkannt.</p><p><a href='/'>Zur Uebersicht</a></p>"
         "</body></html>");
  web.send(200, "text/html", p);
}

/* Nachtragen mit Zeitpunkt: schreibt in die Summen, in die passende
   Woche und mit korrektem Zeitstempel ins Ereignisprotokoll.       */
static void handle_add_get(void)
{
  String p = F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
               "<meta name='viewport' content='width=device-width,initial-scale=1'>"
               "<title>LifeBar nachtragen</title><style>");
  p += PAGE_CSS;
  p += F("</style></head><body><h1>Zeit nachtragen</h1>"
         "<p>Fuer vergessene Eintraege. Der Zeitpunkt landet auch im "
         "Ereignisprotokoll, erscheint also im Kalender.</p>"
         "<form action='/add' method='POST'>"
         "<h2>Was</h2><select name='item'>");
  for (int i = 0; i < PRESET_COUNT; i++)
    p += "<option value='s" + String(i) + "'>" +
         String(preset[i].name[lang]) + "</option>";
  for (int i = 0; i < BAD_PRESET_COUNT; i++)
    p += "<option value='v" + String(i) + "'>" +
         String(bad_preset[i].name[lang]) + " (Laster)</option>";

  p += F("</select><h2>Menge</h2>"
         "<input type='number' name='amount' step='0.5' value='1' "
         "placeholder='Stunden bzw. Anzahl'>"
         "<h2>Wann</h2><div class='row'>"
         "<input type='date' name='d'><input type='time' name='t' value='20:00'>"
         "</div><button type='submit'>Nachtragen</button></form>"
         "<h2>Weiteres</h2><p><a href='/import'>Import und Export</a><br>"
         "<a href='/'>Uebersicht</a></p></body></html>");
  web.send(200, "text/html", p);
}

static void handle_add_post(void)
{
  String item = web.arg("item");
  float  amt  = web.arg("amount").toFloat();
  String ds   = web.arg("d");        /* JJJJ-MM-TT */
  String ts_  = web.arg("t");        /* HH:MM      */

  if (!item.length() || amt == 0 || ds.length() < 10) {
    web.sendHeader("Location", "/add"); web.send(302); return;
  }

  struct tm tmv = {};
  tmv.tm_year = ds.substring(0, 4).toInt() - 1900;
  tmv.tm_mon  = ds.substring(5, 7).toInt() - 1;
  tmv.tm_mday = ds.substring(8, 10).toInt();
  tmv.tm_hour = ts_.length() >= 5 ? ts_.substring(0, 2).toInt() : 20;
  tmv.tm_min  = ts_.length() >= 5 ? ts_.substring(3, 5).toInt() : 0;
  tmv.tm_isdst = -1;
  uint32_t when = (uint32_t)mktime(&tmv);

  bool is_bad = item.startsWith("v");
  int  idx    = item.substring(1).toInt();

  if (is_bad) {
    if (idx < 0 || idx >= BAD_PRESET_COUNT) { web.send(400, "text/plain", "?"); return; }
    int32_t n = (int32_t)amt;
    if (n > 0) {
      st.bad_total[idx] += n;
      if (when >= st.week_start)                   st.bad_week[idx] += n;
      else if (when >= st.week_start - 604800UL)   st.bad_prev[idx] += n;
      log_add_at(when, EV_BAD, (uint8_t)idx, (int16_t)n);
    }
  } else {
    if (idx < 0 || idx >= PRESET_COUNT) { web.send(400, "text/plain", "?"); return; }
    int32_t mins = (int32_t)(amt * 60.0f + 0.5f);
    mins = (mins / STEP_MIN) * STEP_MIN;           /* auf halbe Stunden */
    if (mins > 0) {
      st.minutes[idx]  += mins;
      st.rec_min[idx]  += mins;
      if (when >= st.week_start)                 st.week_min[idx]  += mins;
      else if (when >= st.week_start - 604800UL) st.prev_week[idx] += mins;
      if (tmv.tm_hour >= 0 && tmv.tm_hour < 24 && st.hour_w[tmv.tm_hour] < 60000)
        st.hour_w[tmv.tm_hour]++;
      log_add_at(when, EV_SKILL, (uint8_t)idx, (int16_t)mins);
    }
  }

  recalc_total();
  store_mark_dirty();
  setup_applied = true;

  String p = F("<!DOCTYPE html><html><head><meta charset='utf-8'>"
               "<meta http-equiv='refresh' content='1; url=/add'><style>");
  p += PAGE_CSS;
  p += F("</style></head><body><h1>Nachgetragen</h1></body></html>");
  web.send(200, "text/html", p);
}

/* GET /book?item=s2&delta=1 - buchen wie am Geraet. Slot-Index, nicht
   Preset-Index: die Buchungsflaechen im Browser zeigen dieselben
   sechs Spalten wie das Display.                                   */
static void handle_book(void)
{
  if (!time_valid) { web.send(409, "text/plain", "keine Uhrzeit"); return; }

  String item = web.arg("item");
  int    dir  = web.arg("delta").toInt();
  if (!item.length() || dir == 0) { web.send(400, "text/plain", "?"); return; }

  bool bad  = item.startsWith("v");
  int  slot = item.substring(1).toInt();
  uint32_t now = (uint32_t)time(NULL);

  if (bad) {
    if (slot < 0 || slot >= BAD_COUNT || !bad_active(slot)) {
      web.send(400, "text/plain", "?"); return;
    }
    int idx = st.bad_slot[slot];
    if (dir > 0) apply_entry(true, idx, 1, now);
    else {
      if (*bad_tot_p(slot) == 0) { web.send(200, "text/plain", "ok"); return; }
      (*bad_tot_p(slot))--;
      if (*bad_week_p(slot)) (*bad_week_p(slot))--;
      log_add_at(now, EV_BAD, (uint8_t)idx, -1);
    }
  } else {
    if (slot < 0 || slot >= MOD_COUNT || !mod_active(slot)) {
      web.send(400, "text/plain", "?"); return;
    }
    int idx = st.slot[slot];
    if (dir > 0) apply_entry(false, idx, STEP_MIN, now);
    else {
      if (*mod_min_p(slot) < STEP_MIN) { web.send(200, "text/plain", "ok"); return; }
      (*mod_min_p(slot)) -= STEP_MIN;
      if (*mod_week_p(slot) >= STEP_MIN) (*mod_week_p(slot)) -= STEP_MIN;
      if (st.rec_min[idx] >= STEP_MIN)   st.rec_min[idx] -= STEP_MIN;
      log_add_at(now, EV_SKILL, (uint8_t)idx, -STEP_MIN);
    }
  }

  recalc_total();
  store_mark_dirty();
  ui_refresh_pending = true;      /* Display nachziehen */
  web.send(200, "text/plain", "ok");
}

static void handle_notfound(void)
{
  /* Alles auf die Startseite - so erkennt das Handy das Captive Portal */
  web.sendHeader("Location", String("http://") + WiFi.softAPIP().toString());
  web.send(302, "text/plain", "");
}

/* Nur der Hotspot geht aus - der Webserver laeuft weiter, damit die
   Auswertung im WLAN das Geraet abfragen kann.                     */
static void net_stop_portal(void)
{
  if (!portal_up) return;
  dns.stop();
  WiFi.softAPdisconnect(true);
  portal_up = false;
  Serial.println("[net] Hotspot aus, Server laeuft weiter");
}

static void net_start_web(void)
{
#if !USE_WEB
  return;                    /* Dienste zum Eingrenzen abgeschaltet */
#endif
  if (web_started) return;
  web.on("/",       handle_root);
  web.on("/save",   HTTP_POST, handle_save);
  web.on("/status", handle_status);
  web.on("/events", handle_events);
  web.on("/setup",  HTTP_GET,  handle_setup);
  web.on("/setup",  HTTP_POST, handle_setup_post);
  web.on("/export", handle_export);
  web.on("/import", HTTP_GET,  handle_import_get);
  web.on("/import", HTTP_POST, handle_import_post);
  web.on("/add",    HTTP_GET,  handle_add_get);
  web.on("/add",    HTTP_POST, handle_add_post);
  web.on("/book",   handle_book);
  web.on("/info",   handle_info);
  web.onNotFound(handle_notfound);
  web.begin();
  web_started = true;
  Serial.printf("[net] Webserver laeuft, interner Heap frei %u\n",
                (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
}

static void net_start_portal(void)
{
  if (portal_up) return;

  net_make_ap_name();
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid);
  delay(100);

  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", WiFi.softAPIP());
  net_start_web();

  portal_up      = true;
  portal_started = millis();
  net_state      = NET_PORTAL;
  Serial.printf("[net] Portal auf %s / %s\n",
                ap_ssid, WiFi.softAPIP().toString().c_str());
}

/* ---- Verbinden ------------------------------------------------------- */

static void net_try_connect(void)
{
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  if (ssid.length() == 0) { net_state = NET_IDLE; return; }

  WiFi.mode(portal_up ? WIFI_AP_STA : WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());
  connect_t0 = millis();
  net_state  = NET_CONNECTING;
  Serial.printf("[net] verbinde mit %s\n", ssid.c_str());
}

/* Einmal trennen, Zugangsdaten verwerfen, Hotspot oeffnen. Wird nur
   aus loop() gerufen, damit kein Funkaufruf im UI-Task landet.    */
static void net_reset_and_portal(void)
{
  net_clear_creds();
  WiFi.disconnect(true);
  net_state       = NET_IDLE;
  time_valid      = time_valid;      /* Uhr laeuft weiter */
  last_try_failed = false;
  creds_pending   = false;
  next_retry      = millis() + 60000;   /* kein sofortiger Neuversuch */
  mdns_started    = false;
  net_start_portal();
  Serial.println("[net] Zugang verworfen, Hotspot offen");
}

static void net_begin(void)
{
  /* Ohne Zugangsdaten den Funkteil gar nicht erst anfahren.
     WiFi.setSleep() startet den WLAN-Treiber bereits, auch wenn
     danach nichts verbunden wird - und laufender Funk verfaelscht
     auf diesem Board die I2C-Lesevorgaenge des Touch.            */
  if (!net_has_creds()) {
    net_state = NET_IDLE;
    Serial.println("[net] keine Zugangsdaten, Funk bleibt aus");
    return;
  }

  WiFi.setSleep(true);        /* Stromsparmodus: weniger Funklast */
  net_try_connect();
}

/* Aus loop() aufrufen. Gibt true zurueck, wenn sich der Zustand
   geaendert hat und die UI aktualisiert werden sollte.              */
static bool net_tick(void)
{
  static uint8_t last      = 0xFF;
  static bool    last_time = false;

#if USE_WEB
  if (portal_up) dns.processNextRequest();
  if (web_started) web.handleClient();
#endif

  /* Hotspot laeuft nach dem Verbinden kurz nach, damit die Statusseite
     am Handy den Erfolg noch anzeigen kann.                          */
  if (portal_stop_at && (millis() - portal_stop_at) < 0x80000000UL) {
    portal_stop_at = 0;
    net_stop_portal();
  }

  /* Portal nicht ewig offen lassen, wenn niemand es einrichtet */
  if (portal_up && !portal_stop_at && net_state == NET_PORTAL &&
      st.configured &&
      (millis() - portal_started) > PORTAL_TIMEOUT) {
    Serial.println("[net] Portal-Timeout, Hotspot aus");
    net_stop_portal();
    net_state = NET_IDLE;
  }

  /* Uhr regelmaessig nachstellen. Die Verbindung bleibt bestehen -
     das Geraet ist ein Server und muss erreichbar sein.            */
  if (net_state == NET_ONLINE && (millis() - next_resync) < 0x80000000UL) {
    next_resync = millis() + RESYNC_EVERY;
    net_sync_time();
  }

  if (net_state == NET_CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[net] verbunden, IP %s\n",
                    WiFi.localIP().toString().c_str());
      net_sync_time();
      net_state       = NET_ONLINE;
      last_try_failed = false;
      next_resync     = millis() + RESYNC_EVERY;

      net_start_web();
#if (USE_WEB && USE_MDNS)
      if (!mdns_started && MDNS.begin("lifebar")) {
        MDNS.addService("http", "tcp", 80);
        mdns_started = true;
        Serial.println("[net] erreichbar unter http://lifebar.local");
      }
#endif
      /* Hotspot nur schliessen, wenn die Einrichtung durch ist. Sonst
         waere der einzige Weg in die Konfiguration weg, sobald man
         das WLAN eingetragen hat.                                  */
      if (portal_up && st.configured) portal_stop_at = millis() + 8000;
    } else if (millis() - connect_t0 > CONNECT_TIMEOUT) {
      Serial.println("[net] Timeout");
      WiFi.disconnect();
      last_try_failed = true;
      next_retry      = millis() + 30000;
      net_state       = portal_up ? NET_PORTAL : NET_IDLE;
    }
  } else if (net_state == NET_ONLINE) {
    if (WiFi.status() != WL_CONNECTED) {
      net_state  = NET_IDLE;
      next_retry = millis() + 5000;
      /* time_valid bleibt stehen: die Systemuhr laeuft weiter,
         solange das Board Strom hat.                              */
    }
  } else if (net_state == NET_IDLE && !portal_up &&
             net_has_creds() && (millis() - next_retry) < 0x80000000UL) {
    net_try_connect();
  }

  bool time_changed = net_time_tick();

  bool changed = (net_state != last) || (time_valid != last_time) || time_changed;
  last      = net_state;
  last_time = time_valid;
  return changed;
}

static const char *net_state_text(void)
{
  switch (net_state) {
    case NET_ONLINE:     return time_valid ? "ONLINE" : "ONLINE, KEINE ZEIT";
    case NET_CONNECTING: return "VERBINDET...";
    case NET_PORTAL:     return "PORTAL AKTIV";
    default:             return "OFFLINE";
  }
}

# LifeBar

Ein eigenständiges Gerät, das aus deiner Zeitverwendung eine Identität
ableitet und sie dir auf den Schreibtisch stellt.

Die Idee kam von den Ressourcenleisten aus Strategiespielen: eine
schmale Anzeige, die immer sichtbar ist und auf einen Blick zeigt, wie
man steht. Habit-Tracker als Software gibt es hunderte — der
Unterschied liegt im Objekt, das dasteht und sich nicht wegwischen
lässt.

Alle Daten bleiben auf dem Gerät. Kein Account, keine Cloud.

---

## Funktionen

- **Bis zu 6 Skills** aus 18 Vorlagen, gebucht in Halbstundenschritten
- **Bis zu 6 Laster** aus 12 Kategorien, gezählt als Vorkommnisse
- **EXP entsprechen Minuten.** Dadurch selbstkalibrierend: mehr als der
  Tag hergibt kann niemand eintragen
- **Levelkurve ohne Maximum** — `20 + 2,6·n²` Minuten pro Stufe
- **Verdiente Titel** statt gewählter Klassen
- **Wochenrhythmus** ab Montag 4 Uhr mit Rückblick, Wochenziel, Siegeln
  und Bonus-EXP für Überstunden
- **Lockscreen** mit Uhrzeit, Level und Titel
- **Nachtmodus** mit wärmeren Farben, vierstufige Helligkeit
- **Deutsch und Englisch**, 12- und 24-Stunden-Format
- **Weboberfläche** mit Auswertung, Buchen, Einrichtung, Import/Export
- **Ereignisprotokoll** mit Zeitstempel, als CSV abrufbar

## Das Titelsystem

Das Level schaltet Bausteine frei, die Woche füllt sie.

| Level | Aufbau | Beispiel |
|---|---|---|
| 1–2 | — | NOVIZE |
| 3–9 | Nomen | KRIEGER |
| 10–19 | + Bestimmungswort | WALDBERSERKER |
| ab 20 | + Leitwort | NÄCHTLICHER WALDKRIEGSHERR |

Nomen und Bestimmungswort kommen aus den **Gesamtminuten** — sie
beschreiben dich, nicht deine Woche. Das Nomen steigt mit jeder Stufe
auf: Krieger → Berserker → Kriegsherr, Gelehrter → Magister →
Erzgelehrter. Das Bestimmungswort stammt vom zweitstärksten Skill,
sofern der mindestens 25 % der Minuten des Ersten hat.

Das Leitwort wird **beim Wochenwechsel** neu bestimmt, nach
Auffälligkeit:

1. **REFLEKTIERTER** bei mindestens 10 Laster-Einträgen in der Woche
2. **ERGEBENER** oder **VIELSEITIGER**, gemessen relativ zur Zahl der
   gepflegten Module
3. **NÄCHTLICHER / MORGENDLICHER / MITTÄGLICHER / ABENDLICHER** aus der
   dominanten Tageszeit

Auf Stufe 3 sind über 2000 Titel möglich. Die letzten 26 Wochen werden
als Chronik gespeichert und im Dashboard angezeigt.

## Gestaltungsprinzipien

- **Zeit ist die einzige Währung.** Wer sich verzettelt, sieht es sofort.
- **Laster kosten keine EXP.** Ehrlichkeit muss billig bleiben, sonst
  trägt man sie nicht ein — und dann sind die Daten wertlos. Belohnt
  wird stattdessen das regelmäßige Eintragen selbst.
- **Das Level sinkt nie.** Abgewählte Module behalten ihre Werte und
  zählen weiter ins Level, verschwinden aber aus der Anzeige.
- **Keine Obergrenzen beim Nachtragen.** Abends alles eintragen ist der
  Normalfall, nicht die Ausnahme.
- **Erst auswählen, dann buchen.** Bei 12 mm Spaltenbreite kostet ein
  Fehltreffer so einen Tap statt einer falschen Buchung.
- **Kein Flash-Zugriff aus dem UI-Task.** Alle Schreibvorgänge laufen
  über Flags aus `loop()`.

---

## Hardware

**Waveshare ESP32-S3-Touch-LCD-3.49**, Revision V2

- ESP32-S3R8, 16 MB Flash, 8 MB PSRAM
- 3,49" IPS, 172 × 640, kapazitiver Touch (AXS15231B, QSPI + I2C)
- PCF85063 RTC, QMI8658 IMU, ES8311 Audio, TF-Slot
- Versorgung über USB-C; der 18650-Halter auf dem Board ist optional
  und wird von der Firmware nicht ausgewertet

### Boardrevision beachten

Von diesem Board existieren V1 und V2. Bei V2 sind **LCD_BL und
EXIO_INT sowie LCD_TE und LCD_RESET auf getauschten IOs**. Mit dem
falschen Beispielcode bleibt der Bildschirm schwarz. Erkennbar am
Rev1.1-Silkscreen.

Deshalb ist die **Backlight-PWM hier deaktiviert**:
`EXAMPLE_PIN_NUM_BK_LIGHT` aus dem Demo zeigt auf den V1-Pin,
`setUpduty()` greift ins Leere. Die Helligkeitsregelung läuft
stattdessen über die Farben.

---

## Installation

### 1. Waveshare-Demo besorgen

Ein Teil der Dateien liegt nicht in diesem Repo (siehe *Fremde
Dateien*). Hol dir zuerst das Demo-Paket:

```
github.com/waveshareteam/ESP32-S3-Touch-LCD-3.49
```

Entpacke es an einen Pfad **ohne Leerzeichen und Umlaute**.

### 2. Arduino einrichten

- Boardpaket **esp32 by Espressif Systems ≥ 3.1.0**
- **LVGL 9 offline** aus `Arduino_Libraries` des Demo-Pakets in den
  Sketchbook-Ordner `libraries/` kopieren. Liegt dort schon ein
  `lvgl`-Ordner: löschen. Die mitgelieferte `lv_conf.h` ist angepasst
  und darf nicht von der Library-Manager-Version überschrieben werden
- Der Ordner heißt im Paket `lvgl9` — in `lvgl` umbenennen, `lvgl8`
  aus `libraries/` entfernen
- **SensorLib 0.3.1**
- Arduino danach vollständig beenden und neu starten

In `lv_conf.h` zusätzlich aktivieren:

```c
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_48 1
```

### 3. Fremde Dateien einfügen

Aus `10_LVGL_V9_Test` des Demo-Pakets in den Sketch-Ordner kopieren:

```
lvgl_port.c   lvgl_port.h   i2c_bsp.c   i2c_bsp.h   user_config.h
src/axs15231b/   src/touch/   src/lcd_bl_bsp/
```

Anschließend die Änderungen aus `docs/waveshare-patches.md` anwenden.
Ohne sie startet die Firmware nicht oder der Touch reagiert nicht.

### 4. Tools-Einstellungen

| Einstellung | Wert |
|---|---|
| Board | ESP32S3 Dev Module |
| USB CDC On Boot | **Enabled** |
| Flash Size | 16MB (128Mb) |
| PSRAM | **OPI PSRAM** |
| Partition Scheme | ein Schema mit **FATFS** |

Das Partitionsschema ist wichtig: Das Ereignisprotokoll liegt in einer
FAT-Partition. Ohne FATFS-Anteil meldet der Bootlog
`[log] FEHLER: FFat nicht verfuegbar`.

Eine grafische Referenz liegt im Demo-Paket als
`Tools Configuration.png`.

### 5. Hochladen und einrichten

Ohne hinterlegte WLAN-Daten öffnet das Gerät einen Hotspot
`LifeBar-XXXX`. Verbinden, das Portal springt auf, dort entweder das
WLAN eintragen oder direkt zu `/setup` gehen.

**Reihenfolge beachten:** Erst über `/setup` die Skills einrichten,
dann das WLAN eintragen. Andersherum schließt sich der Hotspot, bevor
man bei der Einrichtung war.

**Werksreset:** BOOT-Taste gedrückt halten, einstecken, zwei Sekunden
weiter halten.

---

## Bedienung

Die Hauptseite in der Mitte, drei Richtungen darum herum:

| Geste | Ziel |
|---|---|
| links / rechts, überall | zwischen Skills und Lastern wechseln |
| von oben nach unten | Wochenrückblick |
| nochmal nach unten | Einstellungen |
| von unten nach oben | Lockscreen |

Auf Unterseiten führt die Gegenrichtung eine Ebene zurück, ausgehend
von der Kante mit dem Griff.

**Buchen:** Tap auf eine Spalte wählt sie aus, dann buchen die großen
Flächen unten — links minus, rechts plus. Nochmal auf die Spalte hebt
die Auswahl auf. Die Auswahl bleibt nach dem Buchen bestehen, 1,5 h
Gym sind also einmal auswählen und dreimal drücken.

Der Lockscreen kommt nach drei Minuten Ruhe von selbst.

---

## Weboberfläche

Erreichbar unter `http://lifebar.local` oder der IP aus dem Bootlog.
Unter Windows braucht `lifebar.local` einen installierten
Bonjour-Dienst.

| Pfad | Zweck |
|---|---|
| `/` | Auswertung: Kalender, Tageszeit, Verteilung, Verlauf, Buchen |
| `/setup` | Skills, Laster, Sprache |
| `/add` | Zeit nachtragen mit Datum und Uhrzeit |
| `/import` | Datensatz bearbeiten und übernehmen |
| `/export` | Datensatz als Textdatei herunterladen |
| `/events` | Ereignisprotokoll als CSV, `?since=UNIXZEIT` für Zuwachs |
| `/info` | Status als JSON |
| `/book` | Buchen, wird von der Auswertungsseite genutzt |

Die Seite ist reines HTML, CSS und SVG ohne Framework und ohne CDN,
rund 19 KB im PROGMEM. Sie funktioniert also auch, wenn der Router kein
Internet hat.

### Import- und Exportformat

Schlichter Text, von Hand lesbar und editierbar. Zeilen mit `#` werden
ignoriert. Diese Zeilen **setzen** Werte:

```
lang=de
week_goal=600
bonus=90
skill,GYM,1230
vice,SOCIAL,12
```

Ereniszeilen **addieren** und tragen einen Zeitstempel:

```
event,2026-08-17,19:30,skill,GYM,90
event,2026-08-17,21:00,vice,STREAMING,1
```

Menge sind Minuten bei Skills und Anzahl bei Lastern. Die Woche wird
aus dem Datum bestimmt, der Eintrag landet also korrekt in laufender
oder Vorwoche und erscheint im Kalender.

---

## Dateien

### Eigener Code (GPLv3)

| Datei | Inhalt |
|---|---|
| `Lifebar.ino` | Start, Hauptschleife, Compile-Schalter |
| `lifebar_i18n.h` | Texttabelle, Sprachen, Leitwörter, Wochentage |
| `lifebar_model.h` | Datenmodell, Levelkurve, Titel, Wochenlogik, NVS |
| `lifebar_log.h` | Ereignisprotokoll auf FAT, 8 Byte je Eintrag |
| `lifebar_net.h` | WLAN, Captive Portal, Webserver, alle Endpunkte |
| `lifebar_page.h` | Auswertungsseite als PROGMEM-String |
| `ui_lifebar.h` | Sämtliche Bildschirme, Gesten, Farben |

Die Include-Reihenfolge ist `i18n → model → log → net → ui`. Alles, was
zwei Ebenen brauchen, gehört in die untere der beiden — sonst
kompiliert es nicht. Das ist bei header-only mit `static` die häufigste
Fehlerquelle.

### Mitgeliefert

`lb_emoji_28.c` — die Icons, als LVGL-Bitmapschrift aus **Noto Emoji**
erzeugt mit dem Font Converter von `lvgl.io`. Liegt fertig im Repo.
Lizenzhinweis in `fonts/`.

### Fremde Dateien

Die Waveshare-Dateien aus Schritt 3 liegen **nicht** im Repo, weil im
Original keine Lizenz angegeben war. Ihre nötigen Änderungen stehen in
`docs/waveshare-patches.md`.

---

## Compile-Schalter

Oben in `Lifebar.ino`:

| Schalter | Wirkung |
|---|---|
| `USE_NET` | 0 = Funkteil bleibt komplett aus |
| `USE_WEB` | 0 = WLAN ohne Webserver, DNS und mDNS |
| `USE_MDNS` | 0 = kein mDNS, Gerät nur über IP erreichbar |
| `BOOT_DEBUG` | Startmarker im Serial Monitor |
| `HEAP_DEBUG` | Speicherverbrauch mitschreiben |

Dazu `UI_DEBUG` in `ui_lifebar.h` und `TOUCH_DEBUG` in `lvgl_port.c`.

Die ersten drei sind das Werkzeug, mit dem sich das unten beschriebene
Touch-Problem eingrenzen lässt.

---

## Bekannte Probleme

### Touch und Netzwerkdienste

Auf diesem Board liefern die I2C-Lesevorgänge des Touch unter
bestimmten Netzwerkkonstellationen verfälschte Daten: alle 32 Bytes
enthalten denselben Wert, während die Übertragung `ESP_OK` meldet. Der
Füllwert wechselt zwischen Sitzungen.

Nachgewiesen: mit `USE_NET 0` funktioniert der Touch zuverlässig, mit
Netzwerkdiensten nicht immer. Der Touch-Pfad selbst ist unverändertes
Waveshare-Original, und das LVGL-8-Beispiel des Herstellers verwendet
identischen Code — ein Wechsel der LVGL-Version ändert daran nichts.

Gegenmaßnahmen im Code:

- **Plausibilitätsprüfung** im Touch-Callback: Lesevorgänge, in denen
  die ersten acht Bytes identisch sind, werden verworfen. Bei einer
  Abfrage alle 30 ms fällt das nicht auf
- Touch-Bus auf 100 kHz statt 300 kHz
- WLAN im Stromsparmodus

### Weitere

- **Backlight-PWM** greift ins Leere, siehe Boardrevision
- **Mehrere Wochen ohne Strom** werden beim Nachrollen als Lücke
  erkannt: Nullwochen in der Historie, Siegelkette reißt, der Rückblick
  benennt die Lücke
- **Ohne gültige Uhr** ruht die Wochenlogik, der Nachtmodus schaltet
  nicht, und Log-Einträge bekommen einen Ersatzzeitstempel. Sie werden
  nachdatiert, sobald NTP durch ist — aber nur innerhalb derselben
  Sitzung. Der PCF85063 auf dem Board wird von der Firmware nicht
  genutzt

---

## Lizenz

Eigener Code unter **GPLv3**.

Fremde Bestandteile behalten ihre Lizenzen:

- `lb_emoji_28.c` — SIL Open Font License 1.1, siehe `OFL.txt`
- `src/touch/esp_lcd_touch.*` — Apache-2.0 (Espressif)

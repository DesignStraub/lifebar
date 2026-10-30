#pragma once

/* =====================================================================
   LifeBar - Compile-Schalter

   Alle Schalter an einer Stelle, und jede Datei, die einen davon
   braucht, bindet diese hier ein. Frueher standen sie in Lifebar.ino
   hinter den Includes - die Header hatten dann laengst ihre eigenen
   Standardwerte gesetzt, und USE_WEB 0 blieb wirkungslos.
   ===================================================================== */

/* ---- Notschalter zum Eingrenzen -------------------------------------
   Bleibt der Start haengen, eines davon auf 0 setzen und neu flashen.
   Der Marker im Serial Monitor zeigt, wie weit er gekommen ist.      */
#define USE_BOOT_RESET  1     /* Werksreset ueber die BOOT-Taste      */
#define BOOT_DEBUG      0     /* Startmarker im Serial Monitor        */
#define HEAP_DEBUG      0     /* Speicherverbrauch mitschreiben       */
#define SHOW_SPLASH     0     /* Startbild                            */

/* ---- Netz -----------------------------------------------------------
   USE_NET 0  -> Funkteil bleibt komplett aus (kein WLAN, kein Portal)
   USE_WEB 0  -> WLAN verbindet, aber ohne Webserver, DNS und mDNS

   Damit laesst sich trennen, ob die Stoerung vom Funkbetrieb selbst
   kommt oder von den Netzwerkdiensten. Reihenfolge zum Testen:
     1) USE_NET 1, USE_WEB 0  - Funk an, keine Dienste
     2) USE_NET 0             - gar kein Funk
   Geht 1) nicht und 2) schon, ist es das Funkteil.
   Geht 1) und 2), ist es der Webserver.                          */
#define USE_NET   1
#define USE_WEB   1
#define USE_MDNS  1           /* belegt viel RAM                      */

/* ---- Bedienteile ----------------------------------------------------
   Drehknopf mit Taster und drei Tasten am 22-Pin-Header, Pins in
   lifebar_hw.h. Ohne angeschlossene Teile harmlos: die Eingaenge
   liegen per Pull-up auf HIGH, es kommt also nie ein Ereignis.
   Die PWR-Taste auf der Rueckseite des Boards gilt immer.          */
#define USE_CONTROLS   1

/* Knopf und Tasten als USB-Mediatasten (Lautstaerke, Play/Pause, Vor,
   Zurueck). Braucht in der Arduino-IDE
     Tools > USB Mode: USB-OTG (TinyUSB)
   und ein Datenkabel zum Rechner. Mit "Hardware CDC and JTAG" baut
   die Firmware trotzdem, nur ohne Mediatasten.                    */
#define USE_USB_MEDIA  1

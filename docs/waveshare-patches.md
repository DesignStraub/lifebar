# Änderungen an den Waveshare-Dateien

Diese Dateien stammen aus `10_LVGL_V9_Test` des Demo-Pakets und liegen
nicht im Repo. Nach dem Kopieren in den Sketch-Ordner sind folgende
Änderungen nötig.

## user_config.h

**Rotation.** Das Demo definiert `Rotated` zweimal — oben auf
`USER_DISP_ROT_NONO`, am Dateiende auf `USER_DISP_ROT_90`. Die untere
Definition entfernen und die obere direkt setzen:

```c
#define Rotated USER_DISP_ROT_90
```

**Task-Stack.** Von 8 auf 16 KB. Im UI-Task laufen NVS-Zugriffe, das
Dateisystem und der Aufbau kompletter Screens; mit 8 KB bleibt der
Start hängen:

```c
#define LVGL_TASK_STACK_SIZE   (16 * 1024)
```

## lvgl_port.h

Den Mutex nach außen geben, damit `loop()` gefahrlos auf LVGL zugreifen
kann:

```c
#include <stdbool.h>

bool lvgl_lock(int timeout_ms);
void lvgl_unlock(void);
```

## lvgl_port.c

**1. Demo-Aufruf entfernen.** Am Ende von `lvgl_port_init()` steht ein
Block, der `lv_demo_widgets()` startet. Ersatzlos löschen, ebenso
`#include "demos/lv_demos.h"` — spart deutlich Flash.

**2. Wrapper anhängen:**

```c
bool lvgl_lock(int timeout_ms) { return example_lvgl_lock(timeout_ms); }
void lvgl_unlock(void)         { example_lvgl_unlock(); }
```

**3. Diagnoseschalter** oben nach den Includes:

```c
#ifndef TOUCH_DEBUG
#define TOUCH_DEBUG 0
#endif
```

**4. Plausibilitätsprüfung im Touch-Callback.** Das ist der wichtigste
Patch — siehe README, Abschnitt *Bekannte Probleme*. In
`TouchInputReadCallback` den Aufruf ersetzen:

```c
esp_err_t _terr = i2c_master_write_read_dev(disp_touch_dev_handle,
                                            read_touchpad_cmd, 11, buff, 32);
```

und direkt vor der Berechnung von `pointX` einfügen:

```c
  {
    bool uniform = true;
    for (int i = 1; i < 8; i++) if (buff[i] != buff[0]) { uniform = false; break; }
    if (_terr != ESP_OK || (uniform && buff[0] != 0)) {
      static uint32_t dropped = 0;
      dropped++;
#if TOUCH_DEBUG
      if (dropped <= 3 || (dropped % 200) == 0)
        printf("[touch] %u verworfen (b0=%u)\n", (unsigned)dropped, buff[0]);
#endif
      indevData->state = LV_INDEV_STATE_RELEASED;
      return;
    }
  }
```

## i2c_bsp.c

Touch-Bus langsamer takten. 100 kHz sind störungsfester, und 43 Byte
brauchen dabei nur 4 ms — bei einer Abfrage alle 30 ms unerheblich:

```c
  dev_cfg.device_address = I2C_TOUCH_ADDR;
  dev_cfg.scl_speed_hz   = 100000;
  ESP_ERROR_CHECK(i2c_master_bus_add_device(user_i2c_port1_handle,
                                            &dev_cfg, &disp_touch_dev_handle));
```

## Nicht ändern

Die Ausgaberoutine `example_lvgl_flush_cb` mit ihren
`portMAX_DELAY`-Wartezeiten bleibt wie sie ist. Ein Timeout dort führt
dazu, dass der Code trotz laufender DMA-Übertragung weiterarbeitet —
danach ist der Handschlag dauerhaft aus dem Tritt und der Bildschirm
bleibt schwarz.

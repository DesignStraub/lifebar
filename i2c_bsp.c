#include <stdio.h>
#include "i2c_bsp.h"
#include "user_config.h"
#include "freertos/FreeRTOS.h"

static i2c_master_bus_handle_t user_i2c_port0_handle = NULL;
static i2c_master_bus_handle_t user_i2c_port1_handle = NULL;
i2c_master_dev_handle_t disp_touch_dev_handle = NULL;
i2c_master_dev_handle_t rtc_dev_handle = NULL;
i2c_master_dev_handle_t imu_dev_handle = NULL;
static i2c_master_dev_handle_t exio_dev_handle = NULL;

/* Schattenkopien der TCA9554-Register, Werte nach dem Einschalten:
   alles Eingang, Ausgangsregister auf HIGH.                        */
static uint8_t exio_out = 0xFF;
static uint8_t exio_dir = 0xFF;


static uint32_t i2c_data_pdMS_TICKS = 0;
static uint32_t i2c_done_pdMS_TICKS = 0;


void i2c_master_Init(void)
{
  /* Waveshare setzt hier 5000 ms. Antwortet ein Chip nicht sauber,
     blockiert damit jeder einzelne Lesevorgang die Oberflaeche fuenf
     Sekunden lang - der Touch wird im LVGL-Task gelesen, der dabei
     den Mutex haelt. 100 ms reichen fuer alles an diesem Bus.      */
  i2c_data_pdMS_TICKS = pdMS_TO_TICKS(100);
  i2c_done_pdMS_TICKS = pdMS_TO_TICKS(100);
  /*i2c_port 0 init*/
  i2c_master_bus_config_t i2c_bus_config = 
  {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_NUM_0,
    .scl_io_num = ESP_SCL_NUM,
    .sda_io_num = ESP_SDA_NUM,
    .glitch_ignore_cnt = 7,
    .flags = {
      .enable_internal_pullup = true,
    },
  };
  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &user_i2c_port0_handle));
  i2c_bus_config.scl_io_num = Touch_SCL_NUM;
  i2c_bus_config.sda_io_num = Touch_SDA_NUM;
  i2c_bus_config.i2c_port = I2C_NUM_1;
  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &user_i2c_port1_handle));
  
  i2c_device_config_t dev_cfg = 
  {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .scl_speed_hz = 300000,
  };
  dev_cfg.device_address = EXAMPLE_RTC_ADDR;
  ESP_ERROR_CHECK(i2c_master_bus_add_device(user_i2c_port0_handle, &dev_cfg, &rtc_dev_handle));

  dev_cfg.device_address = EXAMPLE_IMU_ADDR;
  ESP_ERROR_CHECK(i2c_master_bus_add_device(user_i2c_port0_handle, &dev_cfg, &imu_dev_handle));

  dev_cfg.device_address = EXIO_ADDR;
  ESP_ERROR_CHECK(i2c_master_bus_add_device(user_i2c_port0_handle, &dev_cfg, &exio_dev_handle));

  /* Touch langsamer takten: 100 kHz sind stoerungsfester, und 43 Byte
     brauchen dabei nur 4 ms - bei einer Abfrage alle 30 ms egal.
     Stand so in der Doku, fehlte aber im Code.                     */
  dev_cfg.device_address = I2C_TOUCH_ADDR;
  dev_cfg.scl_speed_hz   = 100000;
  ESP_ERROR_CHECK(i2c_master_bus_add_device(user_i2c_port1_handle, &dev_cfg, &disp_touch_dev_handle));

}

/* TCA9554: Register 1 = Ausgang, 3 = Richtung (1 = Eingang). Erst den
   Pegel, dann die Richtung schreiben - so liegt beim Umschalten auf
   Ausgang sofort der richtige Wert an.
   Fehler werden nur gemeldet, nie mit Abbruch quittiert: ohne
   Expander laeuft das Display weiter wie bisher, nur ohne Reset und
   ohne Abschalten des Backlights.                                  */
esp_err_t exio_set(uint8_t mask, bool level)
{
  if (!exio_dev_handle) return ESP_ERR_INVALID_STATE;

  exio_out = level ? (exio_out | mask) : (exio_out & ~mask);
  uint8_t out[2] = { 0x01, exio_out };
  esp_err_t e = i2c_master_transmit(exio_dev_handle, out, 2, i2c_data_pdMS_TICKS);
  if (e != ESP_OK || !(exio_dir & mask)) return e;

  exio_dir &= ~mask;
  uint8_t dir[2] = { 0x03, exio_dir };
  return i2c_master_transmit(exio_dev_handle, dir, 2, i2c_data_pdMS_TICKS);
}

uint8_t i2c_write_buff(i2c_master_dev_handle_t dev_handle,int reg,uint8_t *buf,uint8_t len)
{
  uint8_t ret;
  uint8_t *pbuf = NULL;
  ret = i2c_master_bus_wait_all_done(user_i2c_port0_handle,i2c_done_pdMS_TICKS);
  if(ret != ESP_OK)
  return ret;
  if(reg == -1)
  {
    ret = i2c_master_transmit(dev_handle,buf,len,i2c_data_pdMS_TICKS);
  }
  else
  {
    pbuf = (uint8_t*)malloc(len+1);
    pbuf[0] = reg;
    for(uint8_t i = 0; i<len; i++)
    {
      pbuf[i+1] = buf[i];
    }
    ret = i2c_master_transmit(dev_handle,pbuf,len+1,i2c_data_pdMS_TICKS);
    free(pbuf);
    pbuf = NULL;
  }
  return ret;
}
uint8_t i2c_master_write_read_dev(i2c_master_dev_handle_t dev_handle,uint8_t *writeBuf,uint8_t writeLen,uint8_t *readBuf,uint8_t readLen)
{
  uint8_t ret;
  ret = i2c_master_bus_wait_all_done(user_i2c_port0_handle,i2c_done_pdMS_TICKS);
  if(ret != ESP_OK)
  return ret;
  ret = i2c_master_transmit_receive(dev_handle,writeBuf,writeLen,readBuf,readLen,i2c_data_pdMS_TICKS);
  return ret;
}
uint8_t i2c_read_buff(i2c_master_dev_handle_t dev_handle,int reg,uint8_t *buf,uint8_t len)
{
  uint8_t ret;
  uint8_t addr = 0;
  ret = i2c_master_bus_wait_all_done(user_i2c_port0_handle,i2c_done_pdMS_TICKS);
  if(ret != ESP_OK)
  return ret;
  if( reg == -1 )
  {ret = i2c_master_receive(dev_handle, buf,len, i2c_data_pdMS_TICKS);}
  else
  {addr = (uint8_t)reg; ret = i2c_master_transmit_receive(dev_handle,&addr,1,buf,len,i2c_data_pdMS_TICKS);}
  return ret;
}


uint8_t i2c_master_touch_write_read(i2c_master_dev_handle_t dev_handle,uint8_t *writeBuf,uint8_t writeLen,uint8_t *readBuf,uint8_t readLen)
{
  uint8_t ret;
  ret = i2c_master_bus_wait_all_done(user_i2c_port1_handle,i2c_done_pdMS_TICKS);
  if(ret != ESP_OK)
  return ret;
  ret = i2c_master_transmit_receive(dev_handle,writeBuf,writeLen,readBuf,readLen,i2c_data_pdMS_TICKS);
  return ret;
}

#ifndef USER_CONFIG_H
#define USER_CONFIG_H

//spi & i2c handle
#define LCD_HOST SPI3_HOST

// touch I2C port
#define Touch_SCL_NUM (GPIO_NUM_18)
#define Touch_SDA_NUM (GPIO_NUM_17)

// touch esp
#define ESP_SCL_NUM (GPIO_NUM_48)
#define ESP_SDA_NUM (GPIO_NUM_47)

//  DISP
#define EXAMPLE_PIN_NUM_LCD_CS     (GPIO_NUM_9) 
#define EXAMPLE_PIN_NUM_LCD_PCLK   (GPIO_NUM_10)
#define EXAMPLE_PIN_NUM_LCD_DATA0  (GPIO_NUM_11)
#define EXAMPLE_PIN_NUM_LCD_DATA1  (GPIO_NUM_12)
#define EXAMPLE_PIN_NUM_LCD_DATA2  (GPIO_NUM_13)
#define EXAMPLE_PIN_NUM_LCD_DATA3  (GPIO_NUM_14)
/* Boardrevision V2 (Rev1.1). Gegenueber V1 sind LCD_BL/EXIO_INT und
   LCD_TE/LCD_RESET getauscht: GPIO 21 ist hier TE, ein Ausgang des
   Panels, und darf nicht getrieben werden. Der Reset liegt am
   IO-Expander. Werte aus dem V2-Schaltplan und Waveshares V2-Demo. */
#define EXAMPLE_PIN_NUM_LCD_TE     (GPIO_NUM_21)
#define EXAMPLE_PIN_NUM_BK_LIGHT   (GPIO_NUM_42)
#define EXAMPLE_PIN_NUM_EXIO_INT   (GPIO_NUM_8)

/* TCA9554 IO-Expander an I2C 0 */
#define EXIO_ADDR          0x20
#define EXIO_PIN_TOUCH_INT (1u << 0)
#define EXIO_PIN_BL_EN     (1u << 1)   /* Backlight-Treiber an/aus   */
#define EXIO_PIN_LCD_RST   (1u << 5)
#define EXIO_PIN_SYS_EN    (1u << 6)   /* nur mit Akku von Belang    */


#define I2C_TOUCH_ADDR                    0x3b
#define EXAMPLE_PIN_NUM_TOUCH_RST         (-1)
#define EXAMPLE_PIN_NUM_TOUCH_INT         (-1)


#define LVGL_TICK_PERIOD_MS    5
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 5
#define LVGL_TASK_STACK_SIZE   (16 * 1024)  /* NVS- und Dateizugriffe brauchen Luft */
#define LVGL_TASK_PRIORITY     2


/*bl test*/
#define Backlight_Testing 0

/*ADDR*/
#define EXAMPLE_RTC_ADDR 0x51

#define EXAMPLE_IMU_ADDR 0x6b


#define USER_DISP_ROT_90    1
#define USER_DISP_ROT_NONO  0
#define Rotated USER_DISP_ROT_90   // Softwarerotation: Querformat 640x172




#define EXAMPLE_LCD_H_RES 172   
#define EXAMPLE_LCD_V_RES 640


#define LCD_NOROT_HRES     172
#define LCD_NOROT_VRES     640
#define LVGL_DMA_BUFF_LEN (LCD_NOROT_HRES * 64 * 2)
#define LVGL_SPIRAM_BUFF_LEN (EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * 2)


#endif
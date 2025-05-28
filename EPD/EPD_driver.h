/*****************************************************************************
* | File      	: DEV_Config.h
* | Author      : Waveshare team
* | Function    :	debug with prntf
* | Info        :
*   Image scanning
*      Please use progressive scanning to generate images or fonts
*----------------
* |	This version:   V1.0
* | Date        :   2018-01-11
* | Info        :   Basic version
*
******************************************************************************/

#ifndef __EPD_DRIVER_H
#define __EPD_DRIVER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "EPD_config.h"

#define BIT(n)  (1UL << (n))

/**@brief EPD driver structure.
 *
 * @details This structure contains epd driver functions.
 */
typedef struct
{
    void (*init)();                                   /**< Initialize the e-Paper register */
    void (*clear)(bool refresh);                      /**< Clear screen */
    void (*write_image)(uint8_t *black, uint8_t *color, uint16_t x, uint16_t y, uint16_t w, uint16_t h); /**< write image */
    void (*refresh)(void);                            /**< Sends the image buffer in RAM to e-Paper and displays */
    void (*sleep)(void);                              /**< Enter sleep mode */
    int8_t (*read_temp)(void);                        /**< Read temperature from driver chip */
    void (*force_temp)(int8_t value);                 /**< Force temperature (will trigger OTP LUT switch) */
    uint8_t cmd_write_ram1;                           /**< Command to write black ram */
    uint8_t cmd_write_ram2;                           /**< Command to write red ram */
} epd_driver_t;

typedef enum
{
    EPD_UC8176_420_BW = 1,
    EPD_UC8176_420_BWR = 3,
    EPD_SSD1619_420_BWR = 2,
    EPD_SSD1619_420_BW = 4,
} epd_model_id_t;

typedef struct
{
    epd_model_id_t id;
    epd_driver_t *drv;
    uint16_t width;
    uint16_t height;
    bool bwr;
} epd_model_t;

#define LOW             (0x0)
#define HIGH            (0x1)

#define DEFAULT         (0xFF)
#define INPUT           (0x0)
#define OUTPUT          (0x1)
#define INPUT_PULLUP    (0x2)
#define INPUT_PULLDOWN  (0x3)

// Arduino like function wrappers
void pinMode(uint32_t pin, uint32_t mode);
#define digitalWrite(pin, value) nrf_gpio_pin_write(pin, value)
#define digitalRead(pin) nrf_gpio_pin_read(pin)
#define delay(ms) nrf_delay_ms(ms)

// GPIO
void EPD_GPIO_Load(epd_config_t *cfg);
void EPD_GPIO_Init(void);
void EPD_GPIO_Uninit(void);

// SPI
void EPD_SPI_Write(uint8_t *value, uint8_t len);
void EPD_SPI_Read(uint8_t *value, uint8_t len);

// EPD
void EPD_WriteCmd(uint8_t cmd);
void EPD_WriteData(uint8_t *value, uint8_t len);
void EPD_ReadData(uint8_t *value, uint8_t len);
void EPD_WriteByte(uint8_t value);
uint8_t EPD_ReadByte(void);
#define EPD_Write(cmd, ...) \
    do { \
        uint8_t _data[] = {__VA_ARGS__}; \
        EPD_WriteCmd(cmd); \
        EPD_WriteData(_data, sizeof(_data)); \
    } while (0)
void EPD_FillRAM(uint8_t cmd, uint8_t value);
void EPD_Reset(uint32_t value, uint16_t duration);
void EPD_WaitBusy(uint32_t value, uint16_t timeout);

// LED
void EPD_LED_ON(void);
void EPD_LED_OFF(void);
void EPD_LED_Toggle(void);
void EPD_LED_BLINK(void);

// VDD voltage
float EPD_ReadVoltage(void);

epd_model_t *epd_get(void);
epd_model_t *epd_init(epd_model_id_t id);

#endif

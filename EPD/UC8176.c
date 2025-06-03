/*****************************************************************************
* | File        :   EPD_4in2.c
* | Author      :   Waveshare team
* | Function    :   4.2inch e-paper
* | Info        :
*----------------
* | This version:   V3.0
* | Date        :   2019-06-13
* -----------------------------------------------------------------------------
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
#include "EPD_driver.h"
#include "nrf_log.h"

// Driver command list.
#define CMD_PSR 0x00   // Panel Setting
#define CMD_PWR 0x01   // Power Setting
#define CMD_POF 0x02   // Power OFF
#define CMD_PFS 0x03   // Power OFF Sequence Setting
#define CMD_PON 0x04   // Power ON
#define CMD_PMES 0x05  // Power ON Measure
#define CMD_BTST 0x06  // Booster Soft Start
#define CMD_DSLP 0x07  // Deep sleep
#define CMD_DTM1 0x10  // Display Start Transmission 1
#define CMD_DSP 0x11   // Data Stop
#define CMD_DRF 0x12   // Display Refresh
#define CMD_DTM2 0x13  // Display Start transmission 2
#define CMD_LUTC 0x20  // VCOM LUT (LUTC)
#define CMD_LUTWW 0x21 // W2W LUT (LUTWW)
#define CMD_LUTBW 0x22 // B2W LUT (LUTBW / LUTR)
#define CMD_LUTWB 0x23 // W2B LUT (LUTWB / LUTW)
#define CMD_LUTBB 0x24 // B2B LUT (LUTBB / LUTB)
#define CMD_PLL 0x30   // PLL control
#define CMD_TSC 0x40   // Temperature Sensor Calibration
#define CMD_TSE 0x41   // Temperature Sensor Selection
#define CMD_TSW 0x42   // Temperature Sensor Write
#define CMD_TSR 0x43   // Temperature Sensor Read
#define CMD_CDI 0x50   // Vcom and data interval setting
#define CMD_LPD 0x51   // Lower Power Detection
#define CMD_TCON 0x60  // TCON setting
#define CMD_TRES 0x61  // Resolution setting
#define CMD_GSST 0x65  // GSST Setting
#define CMD_REV 0x70   // Revision
#define CMD_FLG 0x71   // Get Status
#define CMD_AMV 0x80   // Auto Measurement Vcom
#define CMD_VV 0x81    // Read Vcom Value
#define CMD_VDCS 0x82  // VCM_DC Setting
#define CMD_PTL 0x90   // Partial Window
#define CMD_PTIN 0x91  // Partial In
#define CMD_PTOUT 0x92 // Partial Out
#define CMD_PGM 0xA0   // Program Mode
#define CMD_APG 0xA1   // Active Progrmming
#define CMD_ROTP 0xA2  // Read OTP
#define CMD_CCSET 0xE0 // Cascade Setting
#define CMD_PWS 0xE3   // Power Saving
#define CMD_TSSET 0xE5 // Force Temperauture

static void UC8176_WaitBusy(uint16_t timeout)
{
    EPD_WaitBusy(LOW, timeout);
}

static void UC8176_PowerOn(void)
{
    EPD_WriteCmd(CMD_PON);
    UC8176_WaitBusy(100);
}

static void UC8176_PowerOff(void)
{
    EPD_WriteCmd(CMD_POF);
    UC8176_WaitBusy(100);
}

// Read temperature from driver chip
int8_t UC8176_Read_Temp(void)
{
    EPD_WriteCmd(CMD_TSC);
    UC8176_WaitBusy(100);
    return (int8_t)EPD_ReadByte();
}

// Force temperature (will trigger OTP LUT switch)
void UC8176_Force_Temp(int8_t value)
{
    EPD_Write(CMD_CCSET, 0x02);
    EPD_Write(CMD_TSSET, value);
}

void UC8176_Refresh(void)
{
    NRF_LOG_DEBUG("[EPD]: refresh begin\n");
    UC8176_PowerOn();

    NRF_LOG_DEBUG("[EPD]: temperature: %d\n", UC8176_Read_Temp());
    EPD_WriteCmd(CMD_DRF);
    delay(100);
    UC8176_WaitBusy(30000);
    UC8176_PowerOff();
    NRF_LOG_DEBUG("[EPD]: refresh end\n");
}

static void _setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t xe = (x + w - 1) | 0x0007; // byte boundary inclusive (last byte)
    uint16_t ye = y + h - 1;
    x &= 0xFFF8;       // byte boundary
    xe |= 0x0007;      // byte boundary
    EPD_Write(CMD_PTL, // partial window
              x / 256, x % 256,
              xe / 256, xe % 256,
              y / 256, y % 256,
              ye / 256, ye % 256,
              0x01);
}

void UC8176_Dump_OTP(void)
{
    uint8_t data[128];

    UC8176_PowerOn();
    EPD_Write(CMD_ROTP, 0x00);

    NRF_LOG_DEBUG("=== OTP BEGIN ===\n");
    for (int i = 0; i < 0xFFF; i += sizeof(data)) {
        EPD_ReadData(data, sizeof(data));
        NRF_LOG_HEXDUMP_DEBUG(data, sizeof(data));
    }
    NRF_LOG_DEBUG("=== OTP END ===\n");

    UC8176_PowerOff();
}

void UC8176_Init()
{
    epd_model_t *EPD = epd_get();

    EPD_Reset(HIGH, 10);
    
//    UC8176_Dump_OTP();

    EPD_Write(CMD_PSR, EPD->color == BWR ? 0x0F : 0x1F);
    EPD_Write(CMD_CDI, EPD->color == BWR ? 0x77 : 0x97);
}

void UC8176_Clear(bool refresh)
{
    epd_model_t *EPD = epd_get();
    uint32_t ram_bytes = ((EPD->width + 7) / 8) * EPD->height;

    EPD_FillRAM(CMD_DTM1, 0xFF, ram_bytes);
    EPD_FillRAM(CMD_DTM2, 0xFF, ram_bytes);

    if (refresh)
        UC8176_Refresh();
}

void UC8176_Write_Image(uint8_t *black, uint8_t *color, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    epd_model_t *EPD = epd_get();
    uint16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
    x -= x % 8;                // byte boundary
    w = wb * 8;                // byte boundary
    if (x + w > EPD->width || y + h > EPD->height)
        return;

    EPD_WriteCmd(CMD_PTIN); // partial in
    _setPartialRamArea(x, y, w, h);
    if (EPD->color == BWR)
    {
        EPD_WriteCmd(CMD_DTM1);
        for (uint16_t i = 0; i < h; i++)
        {
            for (uint16_t j = 0; j < w / 8; j++)
                EPD_WriteByte(black ? black[j + i * wb] : 0xFF);
        }
    }
    EPD_WriteCmd(CMD_DTM2);
    for (uint16_t i = 0; i < h; i++)
    {
        for (uint16_t j = 0; j < w / 8; j++)
        {
            if (EPD->color == BWR)
                EPD_WriteByte(color ? color[j + i * wb] : 0xFF);
            else
                EPD_WriteByte(black[j + i * wb]);
        }
    }
    EPD_WriteCmd(CMD_PTOUT); // partial out
}

void UC8176_Sleep(void)
{
    UC8176_PowerOff();

    EPD_Write(CMD_DSLP, 0xA5);
}

// Declare driver and models
static epd_driver_t epd_drv_uc8176 = {
    .init = UC8176_Init,
    .clear = UC8176_Clear,
    .write_image = UC8176_Write_Image,
    .refresh = UC8176_Refresh,
    .sleep = UC8176_Sleep,
    .read_temp = UC8176_Read_Temp,
    .force_temp = UC8176_Force_Temp,
    .cmd_write_ram1 = CMD_DTM1,
    .cmd_write_ram2 = CMD_DTM2,
};

// UC8176 400x300 Black/White
const epd_model_t epd_uc8176_420_bw = {
    .id = EPD_UC8176_420_BW,
    .color = BW,
    .drv = &epd_drv_uc8176,
    .width = 400,
    .height = 300,
};

// UC8176 400x300 Black/White/Red
const epd_model_t epd_uc8176_420_bwr = {
    .id = EPD_UC8176_420_BWR,
    .color = BWR,
    .drv = &epd_drv_uc8176,
    .width = 400,
    .height = 300,
};

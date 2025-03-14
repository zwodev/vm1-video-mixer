/*****************************************************************************
* | File      	:   OLED_1in5_rgb.c
* | Author      :   Waveshare team
* | Function    :   1.5inch OLED Module Drive function
* | Info        :
*----------------
* |	This version:   V2.0
* | Date        :   2020-08-17
* | Info        :
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
#include "OLED_1in5_rgb.h"
#include "stdio.h"

/*******************************************************************************
function:
            Hardware reset
*******************************************************************************/
static void OLED_Reset(void)
{
    OLED_RST_1;
    DEV_Delay_ms(100);
    OLED_RST_0;
    DEV_Delay_ms(100);
    OLED_RST_1;
    DEV_Delay_ms(100);
}

/*******************************************************************************
function:
            Write register address and data
*******************************************************************************/
static void OLED_WriteReg(uint8_t Reg)
{
#if USE_SPI
    OLED_DC_0;
    DEV_SPI_WriteByte(Reg);
#endif
}

static void OLED_WriteData(uint8_t Data)
{
#if USE_SPI
    OLED_DC_1;
    int result = DEV_SPI_WriteByte(Data);
    if (result < 0)
    {
        printf("Error %d while writing data.\n", result);
    }
#endif
}

static void OLED_WriteData_n(uint8_t *Data, uint32_t Len)
{
#if USE_SPI
    OLED_DC_1;
    int result = DEV_SPI_Write_nByte(Data, Len);
    if (result < 0)
    {
        printf("Error %d while writing chunk.\n", result);
    }
#endif
}

/*******************************************************************************
function:
        Common register initialization
*******************************************************************************/
static void OLED_InitReg(void)
{
    OLED_WriteReg(0xfd); // command lock
    OLED_WriteData(0x12);
    OLED_WriteReg(0xfd); // command lock
    OLED_WriteData(0xB1);

    OLED_WriteReg(0xae); // display off
    OLED_WriteReg(0xa4); // Normal Display mode

    OLED_WriteReg(0x15);  // set column address
    OLED_WriteData(0x00); // column address start 00
    OLED_WriteData(0x7f); // column address end 95
    OLED_WriteReg(0x75);  // set row address
    OLED_WriteData(0x00); // row address start 00
    OLED_WriteData(0x7f); // row address end 63

    OLED_WriteReg(0xB3);
    OLED_WriteData(0xF1);

    OLED_WriteReg(0xCA);
    OLED_WriteData(0x7F);

    OLED_WriteReg(0xa0);  // set re-map & data format
    OLED_WriteData(0x74); // Horizontal address increment

    OLED_WriteReg(0xa1);  // set display start line
    OLED_WriteData(0x00); // start 00 line

    OLED_WriteReg(0xa2); // set display offset
    OLED_WriteData(0x00);

    OLED_WriteReg(0xAB);
    OLED_WriteReg(0x01);

    OLED_WriteReg(0xB4);
    OLED_WriteData(0xA0);
    OLED_WriteData(0xB5);
    OLED_WriteData(0x55);

    OLED_WriteReg(0xC1);
    OLED_WriteData(0xC8);
    OLED_WriteData(0x80);
    OLED_WriteData(0xC0);

    OLED_WriteReg(0xC7);
    OLED_WriteData(0x0F);

    OLED_WriteReg(0xB1);
    OLED_WriteData(0x32);

    OLED_WriteReg(0xB2);
    OLED_WriteData(0xA4);
    OLED_WriteData(0x00);
    OLED_WriteData(0x00);

    OLED_WriteReg(0xBB);
    OLED_WriteData(0x17);

    OLED_WriteReg(0xB6);
    OLED_WriteData(0x01);

    OLED_WriteReg(0xBE);
    OLED_WriteData(0x05);

    OLED_WriteReg(0xA6);
}

/********************************************************************************
function:
            initialization
********************************************************************************/
void OLED_1in5_rgb_Init(void)
{
    // Hardware reset
    OLED_Reset();

    // Set the initialization register
    OLED_InitReg();
    DEV_Delay_ms(200);

    // Turn on the OLED display
    OLED_WriteReg(0xAF);
}

/********************************************************************************
function:
            Clear screen
********************************************************************************/
void OLED_1in5_rgb_Clear(void)
{
    UWORD i;

    OLED_WriteReg(0x15);
    OLED_WriteData(0);
    OLED_WriteData(127);
    OLED_WriteReg(0x75);
    OLED_WriteData(0);
    OLED_WriteData(127);
    // fill!
    OLED_WriteReg(0x5C);

    for (i = 0; i < OLED_1in5_RGB_WIDTH * OLED_1in5_RGB_HEIGHT * 2; i++)
    {
        OLED_WriteData(0x00);
    }
}

/********************************************************************************
function:   Update all memory to OLED
********************************************************************************/
void OLED_1in5_rgb_Display_chunkwise(UBYTE *Image)
{
    OLED_WriteReg(0x15);
    OLED_WriteData(0);
    OLED_WriteData(127);
    OLED_WriteReg(0x75);
    OLED_WriteData(0);
    OLED_WriteData(127);
    // fill!
    OLED_WriteReg(0x5C);

    UWORD Imagesize = (OLED_1in5_RGB_WIDTH * 2) * OLED_1in5_RGB_HEIGHT;
    uint32_t spi_buffer_size = 4096;

    uint16_t chunks = (Imagesize + spi_buffer_size - 1) / spi_buffer_size;

    for (uint16_t i = 0; i < chunks; i++)
    {
        uint32_t chunk_size = (i == chunks - 1) ? (Imagesize % spi_buffer_size) : spi_buffer_size;
        if (chunk_size == 0)
            chunk_size = spi_buffer_size;
        OLED_WriteData_n(Image + i * spi_buffer_size, chunk_size);
    }
    // OLED_WriteReg(0xAF);
    return;
}

/********************************************************************************
function:   Update all memory to OLED
********************************************************************************/
void OLED_1in5_rgb_Display(UBYTE *Image)
{
    UWORD i, j, temp;

    OLED_WriteReg(0x15);
    OLED_WriteData(0);
    OLED_WriteData(127);
    OLED_WriteReg(0x75);
    OLED_WriteData(0);
    OLED_WriteData(127);
    // fill!
    OLED_WriteReg(0x5C);

    for (i = 0; i < OLED_1in5_RGB_HEIGHT; i++)
        for (j = 0; j < OLED_1in5_RGB_WIDTH * 2; j++)
        {
            temp = Image[j + i * 256];
            OLED_WriteData(temp);
        }
}

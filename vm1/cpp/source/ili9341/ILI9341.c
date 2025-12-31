/*****************************************************************************
* | File      	:   ILI9341.c
* | Author      :   ILI9341 Driver
* | Function    :   2.8-inch ILI9341 Module Drive function
******************************************************************************/
#include "ILI9341.h"
#include "DEV_Config.h"
#include "stdio.h"
#include <stdlib.h>

#if USE_DEV_LIB
#include <lgpio.h>
extern int ILI9341_SPI_Handle;  // Defined in DEV_Config.c
#endif

/*******************************************************************************
function:
            Hardware reset
*******************************************************************************/
void ILI9341_Reset(void)
{
    ILI9341_RST_1;
    DEV_Delay_ms(50);
    ILI9341_RST_0;
    DEV_Delay_ms(100);
    ILI9341_RST_1;
    DEV_Delay_ms(50);
}

/*******************************************************************************
function:
            Write register (command)
*******************************************************************************/
static void ILI9341_WriteReg(uint8_t Reg)
{
#if USE_SPI
    ILI9341_CS_0;  // Select chip
    ILI9341_RS_0;  // Command mode
    DEV_SPI_WriteByte(Reg);
    ILI9341_CS_1;  // Deselect chip
#endif
}

/*******************************************************************************
function:
            Write data
*******************************************************************************/
static void ILI9341_WriteData(uint8_t Data)
{
#if USE_SPI
    ILI9341_CS_0;  // Select chip
    ILI9341_RS_1;  // Data mode
    DEV_SPI_WriteByte(Data);
    ILI9341_CS_1;  // Deselect chip
#endif
}

static void ILI9341_WriteData_16(uint16_t Data)
{
#if USE_SPI
    ILI9341_CS_0;  // Select chip
    ILI9341_RS_1;  // Data mode
    DEV_SPI_WriteByte(Data >> 8);    // High byte
    DEV_SPI_WriteByte(Data & 0xFF); // Low byte
    ILI9341_CS_1;  // Deselect chip
#endif
}

/*******************************************************************************
function:
            Set display direction
*******************************************************************************/
static void ILI9341_direction(uint8_t direction)
{
    uint8_t reg_value = 0x08; // BGR=1, default
    
    switch(direction % 4) {
        case 0: // Portrait
            reg_value |= (0 << 6) | (0 << 7); // MY=0, MX=0, MV=0
            break;
        case 1: // Landscape
            reg_value |= (0 << 7) | (1 << 6) | (1 << 5); // MY=1, MX=0, MV=1
            break;
        case 2: // Portrait inverted
            reg_value |= (1 << 6) | (1 << 7); // MY=1, MX=1, MV=0
            break;
        case 3: // Landscape inverted
            reg_value |= (1 << 7) | (1 << 5); // MY=1, MX=1, MV=1
            break;
    }
    
    ILI9341_WriteReg(0x36); // Memory Access Control
    ILI9341_WriteData(reg_value);
}

/*******************************************************************************
function:
            ILI9341 initialization
*******************************************************************************/
void ILI9341_Init(void)
{
    ILI9341_Reset(); // LCD reset

    //*************2.8 ILI9341 IPS initialization**********//
    ILI9341_WriteReg(0xCF);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0xC1);
    ILI9341_WriteData(0x30);

    ILI9341_WriteReg(0xED);
    ILI9341_WriteData(0x64);
    ILI9341_WriteData(0x03);
    ILI9341_WriteData(0X12);
    ILI9341_WriteData(0X81);

    ILI9341_WriteReg(0xE8);
    ILI9341_WriteData(0x85);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x78);

    ILI9341_WriteReg(0xCB);
    ILI9341_WriteData(0x39);
    ILI9341_WriteData(0x2C);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x34);
    ILI9341_WriteData(0x02);

    ILI9341_WriteReg(0xF7);
    ILI9341_WriteData(0x20);

    ILI9341_WriteReg(0xEA);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x00);

    ILI9341_WriteReg(0xC0);       // Power control
    ILI9341_WriteData(0x13);     // VRH[5:0]

    ILI9341_WriteReg(0xC1);       // Power control
    ILI9341_WriteData(0x13);     // SAP[2:0];BT[3:0]

    ILI9341_WriteReg(0xC5);       // VCM control
    ILI9341_WriteData(0x22);   // 22
    ILI9341_WriteData(0x35);   // 35

    ILI9341_WriteReg(0xC7);       // VCM control2
    ILI9341_WriteData(0xBD);  // AF

    ILI9341_WriteReg(0x21);       // Display Inversion ON

    ILI9341_WriteReg(0x36);       // Memory Access Control
    ILI9341_WriteData(0x08);

    ILI9341_WriteReg(0xB6);
    ILI9341_WriteData(0x0A);
    ILI9341_WriteData(0xA2);

    ILI9341_WriteReg(0x3A);       // Pixel Format Set
    ILI9341_WriteData(0x55);     // 16-bit/pixel

    ILI9341_WriteReg(0xF6);  // Interface Control
    ILI9341_WriteData(0x01);
    ILI9341_WriteData(0x30);  // MCU

    ILI9341_WriteReg(0xB1);       // Frame Rate Control
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x1B);

    ILI9341_WriteReg(0xF2);       // 3Gamma Function Disable
    ILI9341_WriteData(0x00);

    ILI9341_WriteReg(0x26);       // Gamma curve selected
    ILI9341_WriteData(0x01);

    ILI9341_WriteReg(0xE0);       // Set Gamma
    ILI9341_WriteData(0x0F);
    ILI9341_WriteData(0x35);
    ILI9341_WriteData(0x31);
    ILI9341_WriteData(0x0B);
    ILI9341_WriteData(0x0E);
    ILI9341_WriteData(0x06);
    ILI9341_WriteData(0x49);
    ILI9341_WriteData(0xA7);
    ILI9341_WriteData(0x33);
    ILI9341_WriteData(0x07);
    ILI9341_WriteData(0x0F);
    ILI9341_WriteData(0x03);
    ILI9341_WriteData(0x0C);
    ILI9341_WriteData(0x0A);
    ILI9341_WriteData(0x00);

    ILI9341_WriteReg(0XE1);       // Set Gamma
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x0A);
    ILI9341_WriteData(0x0F);
    ILI9341_WriteData(0x04);
    ILI9341_WriteData(0x11);
    ILI9341_WriteData(0x08);
    ILI9341_WriteData(0x36);
    ILI9341_WriteData(0x58);
    ILI9341_WriteData(0x4D);
    ILI9341_WriteData(0x07);
    ILI9341_WriteData(0x10);
    ILI9341_WriteData(0x0C);
    ILI9341_WriteData(0x32);
    ILI9341_WriteData(0x34);
    ILI9341_WriteData(0x0F);

    ILI9341_WriteReg(0x11);       // Exit Sleep
    DEV_Delay_ms(120);
    ILI9341_WriteReg(0x29);       // Display on
    
    ILI9341_direction(USE_HORIZONTAL); // Set LCD display direction
}

/*******************************************************************************
function:
            Set display window
*******************************************************************************/
void ILI9341_SetWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend)
{
    ILI9341_WriteReg(0x2A); // Column Address Set
    ILI9341_WriteData(Xstart >> 8);
    ILI9341_WriteData(Xstart & 0xFF);
    ILI9341_WriteData((Xend - 1) >> 8);
    ILI9341_WriteData((Xend - 1) & 0xFF);

    ILI9341_WriteReg(0x2B); // Row Address Set
    ILI9341_WriteData(Ystart >> 8);
    ILI9341_WriteData(Ystart & 0xFF);
    ILI9341_WriteData((Yend - 1) >> 8);
    ILI9341_WriteData((Yend - 1) & 0xFF);

    ILI9341_WriteReg(0x2C); // Memory Write
}

/*******************************************************************************
function:
            Clear screen with color (fast bulk write version)
*******************************************************************************/
void ILI9341_Clear(uint16_t Color)
{
    uint32_t bufferSize = ILI9341_WIDTH * ILI9341_HEIGHT * 2;
    uint8_t *clearBuffer = (uint8_t *)malloc(bufferSize);
    
    if (clearBuffer == NULL) {
        // Fallback to slow method if malloc fails
        uint16_t i, j;
        ILI9341_SetWindow(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT);
        ILI9341_CS_0;
        ILI9341_RS_1;
        for (i = 0; i < ILI9341_HEIGHT; i++) {
            for (j = 0; j < ILI9341_WIDTH; j++) {
                DEV_SPI_WriteByte(Color >> 8);
                DEV_SPI_WriteByte(Color & 0xFF);
            }
        }
        ILI9341_CS_1;
        return;
    }
    
    // Fill the buffer with the color
    uint8_t highByte = Color >> 8;
    uint8_t lowByte = Color & 0xFF;
    for (uint32_t i = 0; i < ILI9341_WIDTH * ILI9341_HEIGHT; i++) {
        clearBuffer[i * 2] = highByte;
        clearBuffer[i * 2 + 1] = lowByte;
    }
    
    // Use the existing fast display function
    ILI9341_Display_nByte(clearBuffer, bufferSize);
    
    free(clearBuffer);
}

/*******************************************************************************
function:
            Display image (16-bit color data)
*******************************************************************************/
void ILI9341_Display(uint16_t *Image)
{
    uint32_t i;
    ILI9341_SetWindow(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT);
    
    ILI9341_CS_0;
    ILI9341_RS_1; // Data mode
    
    for (i = 0; i < ILI9341_WIDTH * ILI9341_HEIGHT; i++) {
        ILI9341_WriteData_16(Image[i]);
    }
    
    ILI9341_CS_1;
}

/*******************************************************************************
function:
            Display image from byte buffer (RGB565 format)
            Uses chunked writes for large transfers
*******************************************************************************/
void ILI9341_Display_nByte(uint8_t *Image, uint32_t Len)
{
    ILI9341_SetWindow(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT);
    
    ILI9341_CS_0;
    ILI9341_RS_1; // Data mode
    
#if defined(USE_DEV_LIB) && USE_DEV_LIB
    // Use chunked writes directly with lgSpiWrite for better performance
    if (ILI9341_SPI_Handle >= 0) {
        const uint32_t CHUNK_SIZE = 4096;
        uint32_t remaining = Len;
        uint8_t *ptr = Image;
        
        while (remaining > 0) {
            uint32_t chunk_len = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
            int result = lgSpiWrite(ILI9341_SPI_Handle, (char *)ptr, chunk_len);
            if (result < 0) {
                printf("ILI9341: SPI write error %d\n", result);
                break;
            }
            ptr += chunk_len;
            remaining -= chunk_len;
        }
    }
#else
    // Fallback for other libraries
    DEV_SPI_Write_nByte(Image, Len);
#endif
    
    ILI9341_CS_1;
}

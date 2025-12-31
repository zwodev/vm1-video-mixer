#ifndef _ILI9341_DEV_CONFIG_H_
#define _ILI9341_DEV_CONFIG_H_
/***********************************************************************************************************************
            ------------------------------------------------------------------------
            |\\\																///|
            |\\\					Hardware interface for ILI9341				///|
            ------------------------------------------------------------------------
***********************************************************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef USE_BCM2835_LIB
#include <bcm2835.h>
#elif USE_WIRINGPI_LIB
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <wiringPiI2C.h>
#elif USE_DEV_LIB
#include <lgpio.h>
#define LFLAGS 0
#define NUM_MAXBUF 4
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#define USE_SPI 1

/**
 * data types
 **/
#define UBYTE uint8_t
#define UWORD uint16_t
#define UDOUBLE uint32_t

// ILI9341 GPIO Pin Definitions
#define ILI9341_CS 5      // Chip Select
#define ILI9341_RST 17   // Reset
#define ILI9341_RS 22    // Register Select / Data/Command
#define ILI9341_LED 18   // Backlight control

// ILI9341-specific functions (prefixed to avoid conflicts with OLED driver)
UBYTE ILI9341_DEV_ModuleInit(void);
void ILI9341_DEV_ModuleExit(void);
void ILI9341_DEV_Digital_Write(UWORD Pin, UBYTE Value);
UBYTE ILI9341_DEV_Digital_Read(UWORD Pin);
void ILI9341_DEV_GPIO_Mode(UWORD Pin, UWORD Mode);
void ILI9341_DEV_Delay_ms(UDOUBLE xms);
int ILI9341_DEV_SPI_WriteByte(UBYTE Value);
int ILI9341_DEV_SPI_Write_nByte(uint8_t *pData, uint32_t Len);

// Macros using ILI9341-specific functions (NOT the generic DEV_* names)
#define ILI9341_CS_0 ILI9341_DEV_Digital_Write(ILI9341_CS, 0)
#define ILI9341_CS_1 ILI9341_DEV_Digital_Write(ILI9341_CS, 1)

#define ILI9341_RST_0 ILI9341_DEV_Digital_Write(ILI9341_RST, 0)
#define ILI9341_RST_1 ILI9341_DEV_Digital_Write(ILI9341_RST, 1)

#define ILI9341_RS_0 ILI9341_DEV_Digital_Write(ILI9341_RS, 0)  // Command
#define ILI9341_RS_1 ILI9341_DEV_Digital_Write(ILI9341_RS, 1)  // Data

#define ILI9341_LED_0 ILI9341_DEV_Digital_Write(ILI9341_LED, 0)
#define ILI9341_LED_1 ILI9341_DEV_Digital_Write(ILI9341_LED, 1)

// Compatibility aliases for ILI9341.c (these call the ILI9341-specific versions)
#define DEV_Delay_ms ILI9341_DEV_Delay_ms
#define DEV_SPI_WriteByte ILI9341_DEV_SPI_WriteByte
#define DEV_SPI_Write_nByte ILI9341_DEV_SPI_Write_nByte

#ifdef __cplusplus
}
#endif

#endif

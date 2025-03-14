#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_
/***********************************************************************************************************************
            ------------------------------------------------------------------------
            |\\\																///|
            |\\\					Hardware interface							///|
            ------------------------------------------------------------------------
***********************************************************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

#include "Debug.h"
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
#define USE_IIC 0

#define IIC_CMD 0X00
#define IIC_RAM 0X40

/**
 * data
 **/
#define UBYTE uint8_t
#define UWORD uint16_t
#define UDOUBLE uint32_t

// OLED Define
#define OLED_CS 8
#define OLED_RST 27
#define OLED_DC 25

#define OLED_CS_0 DEV_Digital_Write(OLED_CS, 0)
#define OLED_CS_1 DEV_Digital_Write(OLED_CS, 1)

#define OLED_RST_0 DEV_Digital_Write(OLED_RST, 0)
#define OLED_RST_1 DEV_Digital_Write(OLED_RST, 1)

#define OLED_DC_0 DEV_Digital_Write(OLED_DC, 0)
#define OLED_DC_1 DEV_Digital_Write(OLED_DC, 1)

    /*------------------------------------------------------------------------------------------------------*/

    UBYTE DEV_ModuleInit(void);
    void DEV_ModuleExit(void);

    void DEV_GPIO_Mode(UWORD Pin, UWORD Mode);
    void DEV_Digital_Write(UWORD Pin, UBYTE Value);
    UBYTE DEV_Digital_Read(UWORD Pin);
    void DEV_Delay_ms(UDOUBLE xms);

    void I2C_Write_Byte(uint8_t value, uint8_t Cmd);
    int DEV_SPI_WriteByte(UBYTE Value);
    int DEV_SPI_Write_nByte(uint8_t *pData, uint32_t Len);

#ifdef __cplusplus
}
#endif

#endif
/*****************************************************************************
 * | File      	:   DEV_Config.c
 * | Author      :   Waveshare team
 * | Function    :   Hardware underlying interface
 * | Info        :
 *----------------
 * |	This version:   V2.0
 * | Date        :   2020-06-17
 * | Info        :   Basic version
 *
 ******************************************************************************/
#include "DEV_Config.h"
#include <unistd.h>

uint32_t fd;
#if USE_DEV_LIB

int GPIO_Handle;
int SPI_Handle;
int I2C_Handle;

#endif
/*****************************************
                GPIO
*****************************************/
void DEV_Digital_Write(UWORD Pin, UBYTE Value)
{
#ifdef USE_BCM2835_LIB
    bcm2835_gpio_write(Pin, Value);

#elif USE_WIRINGPI_LIB
    digitalWrite(Pin, Value);

#elif USE_DEV_LIB
    lgGpioWrite(GPIO_Handle, Pin, Value);

#endif
}

UBYTE DEV_Digital_Read(UWORD Pin)
{
    UBYTE Read_value = 0;
#ifdef USE_BCM2835_LIB
    Read_value = bcm2835_gpio_lev(Pin);

#elif USE_WIRINGPI_LIB
    Read_value = digitalRead(Pin);

#elif USE_DEV_LIB
    Read_value = lgGpioRead(GPIO_Handle, Pin);
#endif
    return Read_value;
}

void DEV_GPIO_Mode(UWORD Pin, UWORD Mode)
{
#ifdef USE_BCM2835_LIB
    if (Mode == 0 || Mode == BCM2835_GPIO_FSEL_INPT)
    {
        bcm2835_gpio_fsel(Pin, BCM2835_GPIO_FSEL_INPT);
    }
    else
    {
        bcm2835_gpio_fsel(Pin, BCM2835_GPIO_FSEL_OUTP);
    }
#elif USE_WIRINGPI_LIB
    if (Mode == 0 || Mode == INPUT)
    {
        pinMode(Pin, INPUT);
        pullUpDnControl(Pin, PUD_UP);
    }
    else
    {
        pinMode(Pin, OUTPUT);
        // printf (" %d OUT \r\n",Pin);
    }
#elif USE_DEV_LIB
    if (Mode == 0 || Mode == LG_SET_INPUT)
    {
        lgGpioClaimInput(GPIO_Handle, LFLAGS, Pin);
        // printf("IN Pin = %d\r\n",Pin);
    }
    else
    {
        lgGpioClaimOutput(GPIO_Handle, LFLAGS, Pin, LG_LOW);
        // printf("OUT Pin = %d\r\n",Pin);
    }
#endif
}

/**
 * delay x ms
 **/
void DEV_Delay_ms(UDOUBLE xms)
{
#ifdef USE_BCM2835_LIB
    bcm2835_delay(xms);
#elif USE_WIRINGPI_LIB
    delay(xms);
#elif USE_DEV_LIB
    lguSleep(xms / 1000.0);
#endif
}

static void DEV_GPIO_Init(void)
{
    DEV_GPIO_Mode(OLED_CS, 1);
    DEV_GPIO_Mode(OLED_RST, 1);
    DEV_GPIO_Mode(OLED_DC, 1);
}

/******************************************************************************
function:	Module Initialize, the library and initialize the pins, SPI protocol
parameter:
Info:
******************************************************************************/
UBYTE DEV_ModuleInit(void)
{
#ifdef USE_BCM2835_LIB
    if (!bcm2835_init())
    {
        printf("bcm2835 init failed  !!! \r\n");
        return 1;
    }
    else
    {
        printf("bcm2835 init success !!! \r\n");
    }
    DEV_GPIO_Init();
#if USE_SPI
    printf("USE_SPI\r\n");
    bcm2835_spi_begin();                                       // Start spi interface, set spi pin for the reuse function
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);   // High first transmission
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE3);                // spi mode 3
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32); // Frequency
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                   // set CE0
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);   // enable cs0
#elif USE_IIC
    OLED_DC_0;
    OLED_CS_0;
    printf("USE_IIC\r\n");
    bcm2835_i2c_begin();
    bcm2835_i2c_setSlaveAddress(0x3c);
    /**********************************************************/
#endif

#elif USE_WIRINGPI_LIB
    // if(wiringPiSetup() < 0) {//use wiringpi Pin number table
    if (wiringPiSetupGpio() < 0)
    { // use BCM2835 Pin number table
        printf("set wiringPi lib failed	!!! \r\n");
        return 1;
    }
    else
    {
        printf("set wiringPi lib success  !!! \r\n");
    }
    DEV_GPIO_Init();
#if USE_SPI
    printf("USE_SPI\r\n");
    // wiringPiSPISetup(0,9000000);
    wiringPiSPISetupMode(0, 20000000, 3);
#elif USE_IIC
    OLED_DC_0;
    OLED_CS_0;
    printf("USE_IIC\r\n");
    fd = wiringPiI2CSetup(0x3c);
#endif

#elif USE_DEV_LIB
    char buffer[NUM_MAXBUF];
    FILE *fp1;

    fp1 = popen("cat /proc/cpuinfo | grep 'Raspberry Pi 5'", "r");
    if (fp1 == NULL)
    {
        printf("It is not possible to determine the model of the Raspberry PI\n");
        return -1;
    }

    if (fgets(buffer, sizeof(buffer), fp1) != NULL)
    {
        GPIO_Handle = lgGpiochipOpen(4);
        if (GPIO_Handle < 0)
        {
            printf("gpiochip4 Export Failed\n");
            return -1;
        }
    }
    else
    {
        GPIO_Handle = lgGpiochipOpen(0);
        if (GPIO_Handle < 0)
        {
            printf("gpiochip0 Export Failed\n");
            return -1;
        }
    }

    DEV_GPIO_Init();

#if USE_SPI
    printf("USE_SPI\r\n");
    SPI_Handle = lgSpiOpen(0, 0, 20000000, 0);

#elif USE_IIC
    printf("USE_IIC\r\n");
    OLED_DC_0;
    OLED_CS_0;
    I2C_Handle = lgI2cOpen(1, 0x3c, 0);
#endif
#endif
    return 0;
}

int DEV_SPI_WriteByte(uint8_t Value)
{
#ifdef USE_BCM2835_LIB
    bcm2835_spi_transfer(Value);
    return 0;

#elif USE_WIRINGPI_LIB
    wiringPiSPIDataRW(0, &Value, 1);
    return 0;

#elif USE_DEV_LIB
    // printf("write data is %d\r\n", Value);
    return lgSpiWrite(SPI_Handle, (char *)&Value, 1);

#endif
}

int DEV_SPI_Write_nByte(uint8_t *pData, uint32_t Len)
{
#ifdef USE_BCM2835_LIB
    char rData[Len];
    bcm2835_spi_transfernb(pData, rData, Len);
    return 0;

#elif USE_WIRINGPI_LIB
    wiringPiSPIDataRW(0, pData, Len);
    return 0;

#elif USE_DEV_LIB
    return lgSpiWrite(SPI_Handle, (char *)pData, Len);
#endif
}

void I2C_Write_Byte(uint8_t value, uint8_t Cmd)
{
#ifdef USE_BCM2835_LIB
    char wbuf[2] = {Cmd, value};
    bcm2835_i2c_write(wbuf, 2);
#elif USE_WIRINGPI_LIB
    int ref;
    // wiringPiI2CWrite(fd,Cmd);
    ref = wiringPiI2CWriteReg8(fd, (int)Cmd, (int)value);
    while (ref != 0)
    {
        ref = wiringPiI2CWriteReg8(fd, (int)Cmd, (int)value);
        if (ref == 0)
            break;
    }
#elif USE_DEV_LIB
    lgI2cWriteI2CBlockData(I2C_Handle, Cmd, (char *)&value, 1);

#endif
}

/******************************************************************************
function:	Module exits, closes SPI and BCM2835 library
parameter:
Info:
******************************************************************************/
void DEV_ModuleExit(void)
{
#ifdef USE_BCM2835_LIB
    bcm2835_spi_end();
    bcm2835_i2c_end();
    bcm2835_close();

#elif USE_WIRINGPI_LIB
    OLED_CS_0;
    OLED_RST_1;
    OLED_DC_0;

#elif USE_DEV_LIB

#endif
}

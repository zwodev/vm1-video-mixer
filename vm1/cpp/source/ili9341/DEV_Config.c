/*****************************************************************************
 * | File      	:   DEV_Config.c
 * | Author      :   Hardware interface for ILI9341
 * | Function    :   Hardware underlying interface
 ******************************************************************************/
#include "DEV_Config.h"
#include <unistd.h>

#if USE_DEV_LIB

static int ILI9341_GPIO_Handle = -1;
int ILI9341_SPI_Handle = -1;  // Non-static so ILI9341.c can access it

#endif

/*****************************************
                GPIO
*****************************************/
void ILI9341_DEV_Digital_Write(UWORD Pin, UBYTE Value)
{
#ifdef USE_BCM2835_LIB
    bcm2835_gpio_write(Pin, Value);

#elif USE_WIRINGPI_LIB
    digitalWrite(Pin, Value);

#elif USE_DEV_LIB
    lgGpioWrite(ILI9341_GPIO_Handle, Pin, Value);

#endif
}

UBYTE ILI9341_DEV_Digital_Read(UWORD Pin)
{
    UBYTE Read_value = 0;
#ifdef USE_BCM2835_LIB
    Read_value = bcm2835_gpio_lev(Pin);

#elif USE_WIRINGPI_LIB
    Read_value = digitalRead(Pin);

#elif USE_DEV_LIB
    Read_value = lgGpioRead(ILI9341_GPIO_Handle, Pin);
#endif
    return Read_value;
}

void ILI9341_DEV_GPIO_Mode(UWORD Pin, UWORD Mode)
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
    }
#elif USE_DEV_LIB
    if (Mode == 0 || Mode == LG_SET_INPUT)
    {
        lgGpioClaimInput(ILI9341_GPIO_Handle, LFLAGS, Pin);
    }
    else
    {
        lgGpioClaimOutput(ILI9341_GPIO_Handle, LFLAGS, Pin, LG_LOW);
    }
#endif
}

/**
 * delay x ms
 **/
void ILI9341_DEV_Delay_ms(UDOUBLE xms)
{
#ifdef USE_BCM2835_LIB
    bcm2835_delay(xms);
#elif USE_WIRINGPI_LIB
    delay(xms);
#elif USE_DEV_LIB
    lguSleep(xms / 1000.0);
#endif
}

static void ILI9341_DEV_GPIO_Init(void)
{
    ILI9341_DEV_GPIO_Mode(ILI9341_CS, 1);
    ILI9341_DEV_GPIO_Mode(ILI9341_RST, 1);
    ILI9341_DEV_GPIO_Mode(ILI9341_RS, 1);
    ILI9341_DEV_GPIO_Mode(ILI9341_LED, 1);
    
    // Set initial states
    ILI9341_CS_1;  // CS high (inactive)
    ILI9341_RST_1; // Reset high
    ILI9341_RS_0;  // Command mode
    ILI9341_LED_1; // Backlight on
}

/******************************************************************************
function:	Module Initialize, the library and initialize the pins, SPI protocol
parameter:
Info:
******************************************************************************/
UBYTE ILI9341_DEV_ModuleInit(void)
{
#ifdef USE_BCM2835_LIB
    if (!bcm2835_init())
    {
        printf("ILI9341: bcm2835 init failed !!! \r\n");
        return 1;
    }
    else
    {
        printf("ILI9341: bcm2835 init success !!! \r\n");
    }
    ILI9341_DEV_GPIO_Init();
#if USE_SPI
    printf("ILI9341: USE_SPI\r\n");
    bcm2835_spi_begin();
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_4);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);
#endif

#elif USE_WIRINGPI_LIB
    if (wiringPiSetupGpio() < 0)
    {
        printf("ILI9341: set wiringPi lib failed !!! \r\n");
        return 1;
    }
    else
    {
        printf("ILI9341: set wiringPi lib success !!! \r\n");
    }
    ILI9341_DEV_GPIO_Init();
#if USE_SPI
    printf("ILI9341: USE_SPI\r\n");
    wiringPiSPISetupMode(0, 32000000, 0);
#endif

#elif USE_DEV_LIB
    ILI9341_GPIO_Handle = lgGpiochipOpen(0);
    if (ILI9341_GPIO_Handle < 0)
    {
        printf("ILI9341: gpiochip0 Export Failed\n");
        return -1;
    }

    ILI9341_DEV_GPIO_Init();

#if USE_SPI
    printf("ILI9341: USE_SPI\r\n");
    printf("ILI9341: Attempting to open SPI bus 0, device 0...\n");
    ILI9341_SPI_Handle = lgSpiOpen(0, 0, 40000000, 0);  // 40MHz for faster refresh
    if (ILI9341_SPI_Handle < 0)
    {
        printf("ILI9341: SPI open failed with error %d\n", ILI9341_SPI_Handle);
        printf("ILI9341: Trying SPI bus 0, device 1 instead...\n");
        ILI9341_SPI_Handle = lgSpiOpen(0, 1, 40000000, 0);  // 40MHz
        if (ILI9341_SPI_Handle < 0)
        {
            printf("ILI9341: SPI device 1 also failed with error %d\n", ILI9341_SPI_Handle);
            return -1;
        }
        printf("ILI9341: SPI device 1 opened successfully, handle=%d\n", ILI9341_SPI_Handle);
    } else {
        printf("ILI9341: SPI opened successfully, handle=%d\n", ILI9341_SPI_Handle);
    }
#endif
#endif
    return 0;
}

int ILI9341_DEV_SPI_WriteByte(uint8_t Value)
{
#ifdef USE_BCM2835_LIB
    bcm2835_spi_transfer(Value);
    return 0;

#elif USE_WIRINGPI_LIB
    wiringPiSPIDataRW(0, &Value, 1);
    return 0;

#elif USE_DEV_LIB
    if (ILI9341_SPI_Handle < 0) {
        printf("ILI9341: SPI handle invalid (%d)\n", ILI9341_SPI_Handle);
        return -1;
    }
    int result = lgSpiWrite(ILI9341_SPI_Handle, (char *)&Value, 1);
    if (result < 0 && result != -43) {
        printf("ILI9341: SPI write error %d\n", result);
    }
    return result;

#endif
}

int ILI9341_DEV_SPI_Write_nByte(uint8_t *pData, uint32_t Len)
{
#ifdef USE_BCM2835_LIB
    char rData[Len];
    bcm2835_spi_transfernb(pData, rData, Len);
    return 0;

#elif USE_WIRINGPI_LIB
    wiringPiSPIDataRW(0, pData, Len);
    return 0;

#elif USE_DEV_LIB
    if (ILI9341_SPI_Handle < 0) {
        printf("ILI9341: SPI handle invalid (%d) for nByte write\n", ILI9341_SPI_Handle);
        return -1;
    }
    int result = lgSpiWrite(ILI9341_SPI_Handle, (char *)pData, Len);
    if (result < 0 && result != -43) {
        printf("ILI9341: SPI nByte write error %d\n", result);
    }
    return result;
#endif
}

/******************************************************************************
function:	Module exits, closes SPI
parameter:
Info:
******************************************************************************/
void ILI9341_DEV_ModuleExit(void)
{
#ifdef USE_BCM2835_LIB
    bcm2835_spi_end();
    bcm2835_close();

#elif USE_WIRINGPI_LIB
    ILI9341_CS_1;
    ILI9341_RST_1;
    ILI9341_RS_0;
    ILI9341_LED_0;

#elif USE_DEV_LIB
    if (ILI9341_SPI_Handle >= 0) {
        lgSpiClose(ILI9341_SPI_Handle);
        ILI9341_SPI_Handle = -1;
    }
#endif

    if (ILI9341_GPIO_Handle >= 0) {
        lgGpiochipClose(ILI9341_GPIO_Handle);
        ILI9341_GPIO_Handle = -1;
    }
}

#include <stdio.h>
#include <string.h>
#include <lgpio.h>
#include <unistd.h> // For sleep()

int main()
{
    // Open SPI0, using CS0 (GPIO 8), 20 MHz, Mode 0
    int SPI_Handle = lgSpiOpen(0, 0, 1000000, 0);
    if (SPI_Handle < 0)
    {
        printf("Error creating SPI Handle.\n");
        return -1;
    }
    else
    {
        printf("SPI initialized successfully. ¯\\_(ツ)_/¯\n");
    }

    const char *msg = "hello spi\n";

    int i = 0;

    while (true)
    {
        int result = lgSpiWrite(SPI_Handle, msg, strlen(msg));

        if (result < 0)
        {
            printf("Error writing to SPI\n");
        }
        else
        {
            printf("#%d | Wrote %d bytes to SPI\n", i, result);
            i++;
            ;
        }
        sleep(1);
    }

    lgSpiClose(SPI_Handle);
    return 0;
}

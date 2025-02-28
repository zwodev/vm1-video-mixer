#include <stdlib.h> //exit()
#include <signal.h> //signal()
#include <math.h>
#include <string.h>
#include <time.h>

#include "OLED_1in5_rgb.h"
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "GUI_BMPfile.h"
#include "Debug.h"

#define IMAGE_SEQ_LENGTH 70

void Handler(int signo)
{
    // System Exit
    printf("\r\nHandler:exit\r\n");
    DEV_ModuleExit();

    exit(0);
}

int OLED_1in5_play_bmp_sequence(void)
{
    if (DEV_ModuleInit() != 0)
    {
        return -1;
    }

    if (USE_IIC)
    {
        printf("Only USE_SPI, Please revise DEV_Config.h !!!\r\n");
        return -1;
    }

    printf("OLED Init...\r\n");
    OLED_1in5_rgb_Init();
    DEV_Delay_ms(50);
    // 0.Create a new image cache
    UBYTE *BlackImage;
    UWORD Imagesize = (OLED_1in5_RGB_WIDTH * 2) * OLED_1in5_RGB_HEIGHT;
    if ((BlackImage = (UBYTE *)malloc(Imagesize + 300)) == NULL)
    {
        printf("Failed to apply for black memory...\r\n");
        return -1;
    }

    Paint_NewImage(BlackImage, OLED_1in5_RGB_WIDTH, OLED_1in5_RGB_HEIGHT, 0, BLACK);
    Paint_SetScale(65);

    // 1.Select Image
    Paint_SelectImage(BlackImage);
    DEV_Delay_ms(50);
    // Paint_Clear(BLACK);

    // OLED_1in5_rgb_Display(BlackImage);

    char filename[64];

    struct timespec start, end;
    long long elapsed_ns;
    double elapsed_ms;

    for (uint8_t i = 1; i < 70; i++)
    {
        clock_gettime(CLOCK_MONOTONIC, &start);

        // OLED_1in5_rgb_Display(BlackImage);
        // Paint_Clear(BLACK);
        snprintf(filename, sizeof(filename), "./pic/seq/frame%03d.bmp", i);
        GUI_ReadBmp_65K(filename, 0, 0);
        OLED_1in5_rgb_Display(BlackImage);
        // DEV_Delay_ms(40);

        clock_gettime(CLOCK_MONOTONIC, &end);
        elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
        elapsed_ms = elapsed_ns / 1000000.0; // Convert to milliseconds
        printf("%.2f\n", elapsed_ms);
    }
    // Drawing on the image
    // Show image on page4
    // OLED_1in5_rgb_Display(BlackImage);
    // DEV_Delay_ms(500);
    // Paint_Clear(BLACK);

    // DEV_Delay_ms(500);

    OLED_1in5_rgb_Clear(); // this is still really slow
    return 0;
}

int OLED_1in5_preload_play_bmp_sequence(void)
{
    if (DEV_ModuleInit() != 0)
    {
        return -1;
    }

    if (USE_IIC)
    {
        printf("Only USE_SPI, Please revise DEV_Config.h !!!\r\n");
        return -1;
    }

    printf("OLED Init...\r\n");
    OLED_1in5_rgb_Init();
    DEV_Delay_ms(50);

    UBYTE *BlackImage[IMAGE_SEQ_LENGTH];
    UWORD Imagesize = (OLED_1in5_RGB_WIDTH * 2) * OLED_1in5_RGB_HEIGHT;
    for (int i = 0; i < IMAGE_SEQ_LENGTH; i++)
    {
        if ((BlackImage[i] = (UBYTE *)malloc(Imagesize + 300)) == NULL)
        {
            printf("Failed to allocate memory for BlackImage[%d]...\r\n", i);

            // Free previously allocated memory
            for (int j = 0; j < i; j++)
            {
                free(BlackImage[j]);
            }

            return -1;
        }
        Paint_NewImage(BlackImage[i], OLED_1in5_RGB_WIDTH, OLED_1in5_RGB_HEIGHT, 0, BLACK);
        Paint_SetScale(65);
    }
    printf("Initialized BlackImage-Array\n");
    // Paint_SetScale(65);

    // 1.Select Image
    DEV_Delay_ms(50);
    // Paint_Clear(BLACK);

    // OLED_1in5_rgb_Display(BlackImage);

    char filename[64];

    struct timespec start, end;
    long long elapsed_ns;
    double elapsed_ms;

    for (int i = 0; i < IMAGE_SEQ_LENGTH; i++)
    {
        Paint_SelectImage(BlackImage[i]);
        Paint_SetScale(65);
        snprintf(filename, sizeof(filename), "./pic/seq/frame%03d.bmp", i);
        printf("%s\n", filename);
        GUI_ReadBmp_65K(filename, 0, 0);
        printf("read done\n");
        DEV_Delay_ms(50);
    }

    printf("loaded all files\n");

    for (uint8_t i = 1; i < IMAGE_SEQ_LENGTH; i++)
    {
        clock_gettime(CLOCK_MONOTONIC, &start);

        // OLED_1in5_rgb_Display(BlackImage);
        // Paint_Clear(BLACK);
        OLED_1in5_rgb_Display(BlackImage[i]);
        // DEV_Delay_ms(40);

        clock_gettime(CLOCK_MONOTONIC, &end);
        elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
        elapsed_ms = elapsed_ns / 1000000.0; // Convert to milliseconds
        printf("%.2f\n", elapsed_ms);
    }
    for (int j = 0; j < IMAGE_SEQ_LENGTH; j++)
    {
        free(BlackImage[j]);
    }
    // Drawing on the image
    // Show image on page4
    // OLED_1in5_rgb_Display(BlackImage);
    // DEV_Delay_ms(500);
    // Paint_Clear(BLACK);

    // DEV_Delay_ms(500);

    OLED_1in5_rgb_Clear(); // this is still really slow

    return 0;
}

int main(int argc, char *argv[])
{
    // Exception handling:ctrl + c

    signal(SIGINT, Handler);
    OLED_1in5_play_bmp_sequence();
    // OLED_1in5_preload_play_bmp_sequence(); // WIP
    return 0;
}

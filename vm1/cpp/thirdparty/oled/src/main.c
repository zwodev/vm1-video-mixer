#include <stdlib.h> //exit()
#include <signal.h> //signal()
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "OLED_1in5_rgb.h"
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "GUI_BMPfile.h"
#include "Debug.h"

#define MAX_FILES 100
#define IMAGE_SEQ_LENGTH 70

char *bmpFiles[MAX_FILES];
int fileCount = 0;
int currentIndex = 0;

UBYTE *BlackImage;
UWORD Imagesize;

void Handler(int signo)
{
    // System Exit
    printf("\r\nHandler:exit\r\n");
    DEV_ModuleExit();

    exit(0);
}

int OLED_init()
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
    Imagesize = (OLED_1in5_RGB_WIDTH * 2) * OLED_1in5_RGB_HEIGHT;
    if ((BlackImage = (UBYTE *)malloc(Imagesize + 300)) == NULL)
    {
        printf("Failed to apply for black memory...\r\n");
        return -1;
    }
    Paint_NewImage(BlackImage, OLED_1in5_RGB_WIDTH, OLED_1in5_RGB_HEIGHT, 0, BLACK);
    Paint_SetScale(65);
    return 0;
}

int OLED_1in5_show_image(char *filename_)
{

    // 1.Select Image
    Paint_SelectImage(BlackImage);
    DEV_Delay_ms(50);
    // Paint_Clear(BLACK);

    // OLED_1in5_rgb_Display(BlackImage);

    char filename[128];
    snprintf(filename, sizeof(filename), "img/%s", filename_);
    printf("Loading %s...\n", filename);
    GUI_ReadBmp_65K(filename, 0, 0);

    OLED_1in5_rgb_Display(BlackImage);
    return 0;
}

int OLED_1in5_play_bmp_sequence(void)
{
    // 1.Select Image
    Paint_SelectImage(BlackImage);
    DEV_Delay_ms(50);
    // Paint_Clear(BLACK);

    // OLED_1in5_rgb_Display(BlackImage);

    char filename[128];

    struct timespec start, end;
    long long elapsed_ns;
    double elapsed_ms;

    for (uint8_t i = 1; i < 70; i++)
    {
        clock_gettime(CLOCK_MONOTONIC, &start);

        // OLED_1in5_rgb_Display(BlackImage);
        // Paint_Clear(BLACK);
        // snprintf(filename, sizeof(filename), "./pic/seq/frame%03d.bmp", i);
        snprintf(filename, sizeof(filename), "/home/pi/Documents/coding/vm1-support/OLED_vm1/pic/seq/frame%03d.bmp", i);
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
        Paint_SelectImage(BlackImage[i]);
    }
    printf("Initialized BlackImage-Array\n");
    // Paint_SetScale(65);

    // 1.Select Image
    DEV_Delay_ms(50);
    // Paint_Clear(BLACK);

    // OLED_1in5_rgb_Display(BlackImage);

    char filename[128];

    struct timespec start, end;
    long long elapsed_ns;
    double elapsed_ms;

    for (int i = 0; i < IMAGE_SEQ_LENGTH; i++)
    {
        clock_gettime(CLOCK_MONOTONIC, &start);
        Paint_SetScale(65);
        Paint_SelectImage(BlackImage[i]);
        // snprintf(filename, sizeof(filename), "~/Documents/coding/vm1-support/OLED_vm1/pic/seq/frame%03d.bmp", i);
        snprintf(filename, sizeof(filename), "/home/pi/Documents/coding/vm1-support/OLED_vm1/pic/seq/frame%03d.bmp", i + 1);

        GUI_ReadBmp_65K(filename, 0, 0);

        clock_gettime(CLOCK_MONOTONIC, &end);

        elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000LL + (end.tv_nsec - start.tv_nsec);
        elapsed_ms = elapsed_ns / 1000000.0; // Convert to milliseconds
        printf("%.2f\n", elapsed_ms);
        // DEV_Delay_ms(50);
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

int compareFilenames(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

void scanBmpFiles()
{
    struct dirent *entry;
    DIR *dir = opendir("img");

    if (!dir)
    {
        perror("Cannot open directory 'img'");
        exit(EXIT_FAILURE);
    }

    fileCount = 0;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strstr(entry->d_name, ".bmp") && fileCount < MAX_FILES)
        {
            bmpFiles[fileCount] = strdup(entry->d_name);
            printf("%s\n", entry->d_name);
            fileCount++;
        }
    }
    closedir(dir);

    if (fileCount == 0)
    {
        printf("No BMP files found in 'img/'\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Found %d bmp-files\n", fileCount);
    }

    qsort(bmpFiles, fileCount, sizeof(char *), compareFilenames);
}

void printBmpFiles()
{
    printf("\nSorted BMP Files:\n");
    for (int i = 0; i < fileCount; i++)
    {
        printf("%d: %s\n", i + 1, bmpFiles[i]);
    }
}

void processInput()
{
    printf("\nPress 'n' for next, 'p' for previous, 'q' to quit: ");
    char ch;
    while (1)
    {
        ch = getchar();
        while (getchar() != '\n')
            ; // Clear input buffer

        if (ch == 'q')
        {
            break;
        }
        else if (ch == 'n')
        {
            currentIndex = (currentIndex + 1) % fileCount;
            OLED_1in5_show_image(bmpFiles[currentIndex]);
        }
        else if (ch == 'p')
        {
            currentIndex = (currentIndex - 1 + fileCount) % fileCount;
            OLED_1in5_show_image(bmpFiles[currentIndex]);
        }
    }
}

int main(int argc, char *argv[])
{
    signal(SIGINT, Handler);

    OLED_init();

    /*
        1) shuffle through all bmp-files with 'n' (next) and 'p' (previous)
           press 'q' to quit
    */
    scanBmpFiles();
    printBmpFiles();

    OLED_1in5_show_image(bmpFiles[currentIndex]);

    processInput();

    for (int i = 0; i < fileCount; i++)
    {
        free(bmpFiles[i]);
    }

    /*
        option 2) load the bmp-file named in the argument
    */
    // if (strcmp(argv[1], "") != 0)
    // {
    //     printf("%s\n", argv[1]);
    //     OLED_1in5_show_image(argv[1]);
    //     return 0;
    // }

    /*
        option 3) play a sequence of bmp images either preloaded
                  note: the preload-function has it's own init-procedure,
                  so you don't need to call OLED_init() in the beginning.
    */
    // OLED_1in5_play_bmp_sequence();
    // OLED_1in5_preload_play_bmp_sequence();

    free(BlackImage);

    return 0;
}

#ifndef __ILI9341_H
#define __ILI9341_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "DEV_Config.h"

/********************************************************************************
	function:
			Define the full screen height length of the display
********************************************************************************/

#define ILI9341_WIDTH 320	 // ILI9341 width (landscape mode after 90° CCW rotation)
#define ILI9341_HEIGHT 240  // ILI9341 height (landscape mode after 90° CCW rotation)

// Display orientation
#define USE_HORIZONTAL 3  // Landscape inverted mode (90° CCW rotation from portrait)

	void ILI9341_Init(void);
	void ILI9341_Reset(void);
	void ILI9341_Clear(uint16_t Color);
	void ILI9341_SetWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend);
	void ILI9341_Display(uint16_t *Image);
	void ILI9341_Display_nByte(uint8_t *Image, uint32_t Len);

#ifdef __cplusplus
}
#endif

#endif


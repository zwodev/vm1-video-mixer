# ILI9341 Display Driver

This directory contains the driver for the 2.8-inch ILI9341 SPI LCD display.

## Pin Connections

The driver uses the following GPIO pins (BCM numbering):

- **CS (Chip Select)**: GPIO 7
- **RST (Reset)**: GPIO 17  
- **RS/DC (Register Select/Data Command)**: GPIO 22
- **LED (Backlight)**: GPIO 18
- **MOSI**: SPI0 MOSI (GPIO 10)
- **SCK**: SPI0 SCK (GPIO 11)
- **MISO**: SPI0 MISO (GPIO 9) - optional, not used for display writes

## Hardware Setup

1. Connect the display to the Raspberry Pi according to the pin mapping above
2. The display uses SPI bus 0 (same as OLED, but different CS pin)
3. Power: Connect VCC to 5V and GND to ground

## Usage

The `ILI9341Controller` class provides a high-level interface similar to `OledController`:

```cpp
#include "ILI9341Controller.h"

ILI9341Controller display;
display.setStbRenderer(stbRenderer);  // Optional: for rendering UI
display.start();  // Start the display thread
// ... use display ...
display.stop();   // Stop when done
```

## Display Specifications

- **Resolution**: 240x320 pixels
- **Color Format**: RGB565 (16-bit color)
- **SPI Mode**: Mode 0
- **SPI Speed**: 32 MHz

## Notes

- The display driver uses **weak linkage** to avoid conflicts with the OLED driver
- Both displays can coexist on the same SPI bus using different CS pins
- The display will NOT be used as the main system display - it's a pure SPI device
- Touch functionality is not implemented (as requested)

## Integration

To use the ILI9341 display in your application, you can:

1. Create an instance of `ILI9341Controller`
2. Optionally connect it to a `StbRenderer` for UI rendering
3. Call `start()` to begin displaying content
4. The display will automatically update from the renderer

Example integration in `VM1Application`:

```cpp
ILI9341Controller m_ili9341Display;

// In initialization:
m_ili9341Display.setStbRenderer(&m_stbRenderer);
m_ili9341Display.start();
```


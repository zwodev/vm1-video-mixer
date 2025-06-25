#include <string>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#include <iomanip>

#include "source/SoftwareRenderer.h"

// Set terminal to raw mode for non-blocking keyboard input
void setRawMode(bool enable)
{
    static struct termios oldt, newt;
    if (enable)
    {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO); // disable canonical mode and echo
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK); // non-blocking input
    }
    else
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // restore
        fcntl(STDIN_FILENO, F_SETFL, 0);         // blocking input
    }
}

char getKeyPress()
{
    char ch = 0;
    if (read(STDIN_FILENO, &ch, 1) == 1)
    {
        return ch;
    }
    return 0;
}

void renderFontTestTiles(SoftwareRenderer &renderer)
{
    // Renders png image tiles with all fonts in fontNames
    // in the given size range

    std::vector<std::string> fontNames;
    fontNames.push_back("Cousine-Regular.ttf");
    // fontNames.push_back("DroidSans.ttf");
    // fontNames.push_back("Karla-Regular.ttf");
    // fontNames.push_back("ProggyClean.ttf");
    // fontNames.push_back("ProggyTiny.ttf");
    // fontNames.push_back("Roboto-Medium.ttf");
    // fontNames.push_back("slkscr.ttf");
    float minFontSize = 28;
    float maxFontSize = 32;
    float fontSizeStep = .5;

    int fontNameIndex = 0;
    bool running = true;

    for (std::string fontName : fontNames)
    {
        if (!renderer.loadFont(fontName))
        {
            std::cerr << "Font load failed\n";
            return;
        }
        for (float fontSize = minFontSize; fontSize <= maxFontSize; fontSize += fontSizeStep)
        {

            renderer.clear();
            // renderer.drawText(10, 25, "Nils!", 255, 255, 255);
            // renderer.drawText(10, 115, "cooler Typ", 255, 255, 255, false);
            // renderer.drawText(10, 45, fontName, 255, 255, 255);
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << fontSize;
            std::string fontSizeStr = ss.str();
            // renderer.drawText(10, 65, fontSizeStr, 255, 255, 255);

            std::string fileName = "render/" + fontName + "-" + fontSizeStr + ".png";
            renderer.savePNG(fileName);

            std::cout << "Rendered image " << fileName << std::endl;
        }
    }
}

int main()
{
    const int width = 128;
    const int height = 128;
    Image img(width, height);
    SoftwareRenderer renderer(img);

    // tests for Font Rendering
    // ------------------------
    renderer.loadFont("fonts/ProggyClean.ttf");
    // renderer.loadFont("slkscr2.ttf");
    renderer.drawText("HALLO", 10, 0, 8); // blurry
    renderer.drawText("HALLO", 10, 20, 16);
    renderer.drawText("HALLO", 10, 40, 24); // blurry
    renderer.drawText("HALLO", 10, 80, 32);

    renderer.savePNG("fonttest.png");
    return 0;

    // Render font-test-tiles to png
    // -----------------------------
    // renderFontTestTiles(renderer);
    // return 0;

    // a main loop
    // -----------
    setRawMode(true);
    bool running = true;
    bool needsRedraw = true;
    while (running)
    {
        char key = getKeyPress();
        if (key)
        {
            if (key == 'q')
            {
                running = false; // quit on 'q'
            }
            else
            {
                needsRedraw = true;
                std::cout << "Key pressed: " << key << "\n";
            }
        }

        if (needsRedraw)
        {
            // todo: update to new font-rendering-methods:
            /*
            renderer.clear();
            // renderer.drawRect(32, 32, 64, 64, 255, 255, 0); // yellow square
            // renderer.drawRect(10, 10, 10, 10, 255, 0, 0);   // red square
            renderer.drawText(10, 25, "Nils!", 255, 255, 255);
            renderer.drawText(10, 115, "cooler Typ", 255, 255, 255, false);
            renderer.drawText(10, 45, fontNames.at(fontNameIndex), 255, 255, 255);
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1) << fontSize;
            std::string fontSizeStr = ss.str();
            renderer.drawText(10, 65, fontSizeStr, 255, 255, 255);

            std::string fileName = "render/" + fontNames.at(fontNameIndex) + "-" + fontSizeStr + ".png";
            renderer.savePNG(fileName);
            std::cout << "Rendered image " << fileName << std::endl;

            needsRedraw = false;
            */
        }

        usleep(250);
    }

    setRawMode(false);
    return 0;
}


#include "Settings.h"
#include "Context.h"

/*
void renderFontTestTiles(StbRenderer &renderer)
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
*/

int main()
{
    Settings settings;
    settings.foo = 0.75;
    settings.bar = 10;
    settings.selectedBank = 0;
    settings.selectedMediaSlotID = 0;

    Context ctx(settings);
    ctx.run();

    return 0;

    // tests for Font Rendering
    // ------------------------
    /*
    renderer->loadFont("fonts/ProggyClean.ttf");
    // renderer.loadFont("slkscr2.ttf");
    renderer->drawText("HALLO", 10, 0, 8); // blurry
    renderer->drawText("HALLO", 10, 20, 16);
    renderer->drawText("HALLO", 10, 40, 24); // blurry
    renderer->drawText("HALLO", 10, 80, 32);

    renderer->savePNG("fonttest.png");
    return 0;
    */

    // Render font-test-tiles to png
    // -----------------------------
    // renderFontTestTiles(renderer);
    // return 0;
}

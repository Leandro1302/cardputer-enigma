#pragma once
#include <M5Cardputer.h>
#include "UiModel.h"

class DisplayRenderer {
public:
    static constexpr int Width = 240;
    static constexpr int Height = 135;
    // M5GFX 0.3.0 includes the Cardputer panel mounting offset in the driver.
    static constexpr int Rotation = 0;
    DisplayRenderer(): canvas_(&M5Cardputer.Display) {}
    bool begin();
    void start(const app::UiModel& ui, uint32_t now);
    void tick(const app::UiModel& ui, uint32_t now);
    void render(const app::UiModel& ui);
    std::string rowHex(unsigned row);
    bool startupActive() const { return startupActive_; }
    uint32_t frames = 0;
    uint32_t startupFrames = 0;
    bool ready = false;
private:
    M5Canvas canvas_;
    bool startupActive_ = false;
    uint32_t startupAt_ = 0;
    uint32_t frameAt_ = 0;
    unsigned renderedRevision_ = 0;
    void drawStartup(uint32_t elapsed);
    void drawMenu(const app::UiModel& ui);
    void text(int x,int y,const std::string& s,uint16_t color,int size=1);
    void title(const char* title);
    void footer(const std::string& text);
    void present();
};

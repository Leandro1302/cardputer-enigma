#include "Keyboard.h"
#include <M5Cardputer.h>
void KeyboardInput::poll(app::UiModel& ui) {
    std::array<bool,56> now{};
    auto& keyboard=M5Cardputer.Keyboard;
    const bool fn=keyboard.keysState().fn;
    for(const auto& p:keyboard.keyList()) {
        if(p.x<0||p.x>=14||p.y<0||p.y>=4)continue;
        const unsigned index=p.y*14+p.x;now[index]=true;
        if(previous_[index])continue;
        uint8_t c=keyboard.getKey(p);
        std::string key;
        if(c==KEY_ENTER)key="ENTER";
        else if(c==KEY_BACKSPACE)key=fn?"RESET":"BACKSPACE";
        else if(c=='`'||c=='~')key="ESC";
        else if(c==KEY_TAB)key="DOWN";
        else if((fn||ui.screen==app::Screen::Menu||ui.screen==app::Screen::Configure)&&(c==';'||c=='.'))key=c==';'?"UP":"DOWN";
        else if((fn||ui.screen==app::Screen::Menu)&&(c==','||c=='/'))key=c==','?"LEFT":"RIGHT";
        else if(c>=32&&c<=126)key=std::string(1,static_cast<char>(c));
        if(!key.empty()){ui.key(key);++events;lastKey=key;}
    }
    previous_=now;
}

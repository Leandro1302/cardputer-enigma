#pragma once
#include "UiModel.h"
#include <array>
class KeyboardInput {
public:
    void poll(app::UiModel& ui);
    uint32_t events=0;
    std::string lastKey;
private:
    std::array<bool,56> previous_{};
};

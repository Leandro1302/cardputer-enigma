#pragma once
#include <ArduinoJson.h>
#include "Enigma.h"
#include "UiModel.h"
#include "ui/Display.h"
#include "ui/Keyboard.h"
class SerialProtocol {
public:
    SerialProtocol(app::UiModel& ui,DisplayRenderer& display,KeyboardInput& keys):ui_(ui),display_(display),keys_(keys){}
    void poll();
    uint32_t requests=0, errors=0;
private:
    app::UiModel& ui_;
    DisplayRenderer& display_;
    KeyboardInput& keys_;
    enigma::Machine machine_;
    std::string line_;
    bool overflow_=false;
    uint32_t lastByte_=0;
    void handle();
    void send(JsonDocument& doc);
    void failure(JsonDocument& doc,const char* code,const char* message);
    void state(JsonDocument& doc);
    void uiState(JsonDocument& doc);
};

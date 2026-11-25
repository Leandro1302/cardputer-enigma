#include <Arduino.h>
#include <M5Cardputer.h>
#include "UiModel.h"
#include "ui/Display.h"
#include "ui/Keyboard.h"
#include "serial/Protocol.h"
#include "wifi/WifiBroadcast.h"

WifiBroadcast broadcast;
app::UiModel ui(broadcast);
DisplayRenderer display;
KeyboardInput keyboard;
SerialProtocol protocol(ui,display,keyboard);

void setup() {
    // HWCDC otherwise defaults to 256 bytes and drops bursts larger than its
    // receive queue. One complete protocol frame is at most 2048 bytes.
    Serial.setRxBufferSize(4096);
    Serial.setTxBufferSize(4096);
    Serial.begin(115200);
    Serial.setTxTimeoutMs(100);
    auto config=M5.config();
    config.internal_spk=false;config.internal_mic=false;
    M5Cardputer.begin(config,true);
    display.begin();
    if(!enigma::selfTest())ui.notice="Cipher self-test failed";
    display.start(ui, millis());
}
void loop() {
    M5Cardputer.update();
    keyboard.poll(ui);
    if(M5Cardputer.BtnA.wasPressed())ui.key("ESC");
    protocol.poll();
    display.tick(ui, millis());
    delay(1);
}

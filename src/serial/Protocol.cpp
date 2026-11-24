#include "Protocol.h"
#include <esp_mac.h>
#include <esp_system.h>
#include <cstring>

namespace {
void configJson(JsonObject obj,const enigma::Config& c) {
    auto rotors=obj["rotors"].to<JsonArray>();for(auto id:c.rotors)rotors.add(id);
    obj["rings"]=enigma::letters(c.rings);obj["initial"]=enigma::letters(c.initial);
    obj["reflector"]=std::string(1,c.reflector);obj["plugboard"]=c.plugboard();
}
bool allowedKeys(JsonDocument& d,const char* allowed) {
    for(JsonPair pair:d.as<JsonObject>()) {
        std::string key="|"+std::string(pair.key().c_str())+"|";
        if(std::string(allowed).find(key)==std::string::npos)return false;
    }
    return true;
}
}
void SerialProtocol::send(JsonDocument& d) { serializeJson(d,Serial);Serial.print('\n'); }
void SerialProtocol::failure(JsonDocument& d,const char* code,const char* msg) {
    ++errors;d["ok"]=false;d["error"]["code"]=code;d["error"]["message"]=msg;send(d);
}
void SerialProtocol::state(JsonDocument& d) {
    d["positions"]=enigma::letters(machine_.state().positions);
    d["letters_processed"]=machine_.state().lettersProcessed;
    configJson(d["config"].to<JsonObject>(),machine_.config());
}
void SerialProtocol::uiState(JsonDocument& d) {
    auto o=d["ui"].to<JsonObject>();
    o["screen"]=ui_.screenName();o["selected"]=ui_.selected;o["field"]=ui_.field;
    o["positions"]=enigma::letters(ui_.machine.state().positions);
    o["letters_processed"]=ui_.machine.state().lettersProcessed;
    o["input"]=ui_.input;o["output"]=ui_.output;o["editor"]=ui_.editor;o["notice"]=ui_.notice;
    o["frames"]=display_.frames;o["display_ready"]=display_.ready;
    o["physical_key_events"]=keys_.events;o["last_physical_key"]=keys_.lastKey;
    configJson(o["config"].to<JsonObject>(),ui_.machine.config());
}
void SerialProtocol::poll() {
    unsigned budget=256;
    while(budget--&&Serial.available()) {
        int value=Serial.read(); if(value<0)break;
        lastByte_=millis();char c=static_cast<char>(value);
        if(c=='\n') {
            if(overflow_) { JsonDocument d;d["id"]=nullptr;failure(d,"LINE_TOO_LONG","Maximum frame is 2048 bytes"); }
            else if(!line_.empty())handle();
            line_.clear();overflow_=false;
        } else if(c!='\r'&&!overflow_) {
            if(line_.size()>=2048){line_.clear();overflow_=true;}
            else line_+=c;
        }
    }
    if((overflow_||!line_.empty())&&uint32_t(millis()-lastByte_)>2000) {
        line_.clear();overflow_=false;JsonDocument d;d["id"]=nullptr;failure(d,"FRAME_TIMEOUT","Incomplete frame discarded after 2 seconds");
    }
}
void SerialProtocol::handle() {
    ++requests;JsonDocument request,response;response["id"]=nullptr;response["ok"]=true;
    if(deserializeJson(request,line_)||!request.is<JsonObject>()) {failure(response,"BAD_JSON","Expected a JSON object");return;}
    if(!request["id"].is<int32_t>()||request["id"].as<int32_t>()<0||!request["cmd"].is<const char*>()) {
        failure(response,"BAD_REQUEST","id must be a nonnegative integer; cmd must be a string");return;
    }
    response["id"]=request["id"].as<int32_t>();
    const std::string cmd=request["cmd"].as<const char*>();
    const char* allowed="|id||cmd|";
    if(cmd=="CONFIG")allowed="|id||cmd||rotors||rings||positions||reflector||plugboard|";
    else if(cmd=="ENCRYPT")allowed="|id||cmd||text|";
    else if(cmd=="UI_KEY")allowed="|id||cmd||key|";
    else if(cmd=="SCREEN")allowed="|id||cmd||row|";
    if(!allowedKeys(request,allowed)){failure(response,"UNKNOWN_FIELD","Unexpected request field");return;}
    if(cmd=="PING") {response["reply"]="PONG";response["protocol"]=1;}
    else if(cmd=="INFO") {
        response["firmware"]="enigma-cardputer";response["version"]=ENIGMA_VERSION;response["protocol"]=1;
        response["chip"]=ESP.getChipModel();response["chip_revision"]=ESP.getChipRevision();
        response["flash_bytes"]=ESP.getFlashChipSize();response["flash_hz"]=ESP.getFlashChipSpeed();
        response["flash_mode"]=static_cast<unsigned>(ESP.getFlashChipMode());
        uint8_t mac[6];esp_read_mac(mac,ESP_MAC_WIFI_STA);char text[18];
        snprintf(text,sizeof(text),"%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
        response["mac"]=text;response["board_id"]=static_cast<int>(M5.getBoard());
        response["board_name"]=M5.getBoard()==m5::board_t::board_M5CardputerADV?"M5Stack Cardputer-ADV":"M5Stack Cardputer";
        response["board_compatible"]=M5.getBoard()==m5::board_t::board_M5Cardputer||M5.getBoard()==m5::board_t::board_M5CardputerADV;
        response["display_width"]=M5Cardputer.Display.width();response["display_height"]=M5Cardputer.Display.height();
        response["display_rotation"]=M5Cardputer.Display.getRotation();
        response["display_ready"]=display_.ready;response["max_encrypt_chars"]=512;
        response["state_scope"]="serial session; GUI has independent state";
    } else if(cmd=="CONFIG") {
        enigma::Config c;
        auto rotors=request["rotors"].as<JsonArray>();
        if(rotors.size()!=3||!request["rings"].is<const char*>()||!request["positions"].is<const char*>()||
           !request["reflector"].is<const char*>()||!request["plugboard"].is<const char*>()) {
            failure(response,"INVALID_CONFIG","Required: rotors[3], rings, positions, reflector, plugboard");return;
        }
        for(unsigned i=0;i<3;++i) {
            if(!rotors[i].is<int>()||rotors[i].as<int>()<1||rotors[i].as<int>()>8){failure(response,"INVALID_ROTOR","Rotor IDs must be integers 1..8");return;}
            c.rotors[i]=rotors[i].as<int>();
        }
        if(!enigma::parseLetters(request["rings"].as<std::string>(),c.rings)){failure(response,"INVALID_RINGS","Expected three A-Z letters");return;}
        if(!enigma::parseLetters(request["positions"].as<std::string>(),c.initial)){failure(response,"INVALID_POSITIONS","Expected three A-Z letters");return;}
        auto r=request["reflector"].as<std::string>();
        if(r!="B"&&r!="C"){failure(response,"INVALID_REFLECTOR","Use B or C");return;} c.reflector=r[0];
        if(!c.setPlugboard(request["plugboard"].as<std::string>())){failure(response,"INVALID_PLUGBOARD","Use up to ten disjoint letter pairs");return;}
        auto error=machine_.configure(c);
        if(error!=enigma::Error::None){failure(response,enigma::errorName(error),"Configuration rejected; state unchanged");return;}
        state(response);
    } else if(cmd=="ENCRYPT") {
        if(!request["text"].is<const char*>()){failure(response,"INVALID_TEXT","text must be a string");return;}
        const auto text=request["text"].as<std::string>();
        if(text.size()>512){failure(response,"TEXT_TOO_LONG","Maximum 512 bytes per command; stream consecutive chunks");return;}
        for(unsigned char c:text)if(c<32||c>126){failure(response,"INVALID_TEXT","Use printable ASCII; only letters advance rotors");return;}
        response["ciphertext"]=machine_.process(text);state(response);
    } else if(cmd=="STATE")state(response);
    else if(cmd=="RESET"){machine_.reset();state(response);}
    else if(cmd=="SELFTEST"){response["passed"]=enigma::selfTest();response["tests"]=2;}
    else if(cmd=="UI_STATE")uiState(response);
    else if(cmd=="UI_KEY") {
        if(!request["key"].is<const char*>()){failure(response,"INVALID_KEY","key must be a string");return;}
        std::string k=request["key"].as<std::string>();
        const bool named=k=="ENTER"||k=="ESC"||k=="RESET"||k=="BACKSPACE"||k=="UP"||k=="DOWN"||k=="LEFT"||k=="RIGHT";
        if(!named&&(k.size()!=1||k[0]<32||k[0]>126)){failure(response,"INVALID_KEY","Unknown UI action");return;}
        ui_.key(k);display_.render(ui_);uiState(response);
    } else if(cmd=="SCREEN") {
        if(!request["row"].is<int>()||request["row"].as<int>()<0||request["row"].as<int>()>=135||!display_.ready){failure(response,"INVALID_ROW","Display row must be 0..134");return;}
        response["row"]=request["row"];response["rgb565"]=display_.rowHex(request["row"].as<unsigned>());
    } else if(cmd=="DIAG") {
        response["uptime_ms"]=millis();response["heap_free"]=ESP.getFreeHeap();response["heap_min"]=ESP.getMinFreeHeap();
        response["heap_max_block"]=ESP.getMaxAllocHeap();response["reset_reason"]=static_cast<int>(esp_reset_reason());
        response["requests"]=requests;response["errors"]=errors;response["frames"]=display_.frames;response["physical_key_events"]=keys_.events;
        response["startup_active"]=display_.startupActive();response["startup_frames"]=display_.startupFrames;
    } else {failure(response,"UNKNOWN_COMMAND","Unknown command");return;}
    send(response);
}

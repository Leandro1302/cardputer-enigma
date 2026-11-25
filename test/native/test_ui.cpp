#include "UiModel.h"
#include <cstdlib>
#include <iostream>
#define CHECK(e) do { if(!(e)) {std::cerr<<"UI FAIL "<<__LINE__<<": " #e "\n";std::exit(1);} } while(0)

class FakeBroadcast final : public app::Broadcast {
public:
    bool start(const std::string& ssid) override {
        ++starts;
        if (failStart) return false;
        current = {true, ssid, true};
        return true;
    }
    bool stop() override {
        ++stops;
        if (failStop) return false;
        current = {};
        return true;
    }
    app::BroadcastStatus status() const override { return current; }

    app::BroadcastStatus current;
    int starts = 0, stops = 0;
    bool failStart = false, failStop = false;
};

static void testBroadcast() {
    FakeBroadcast broadcast;
    app::UiModel ui(broadcast);
    ui.key("RIGHT");
    ui.key("ENTER");
    ui.key("ENTER");
    CHECK(ui.screen == app::Screen::MessageActions && ui.messageAction == 0);
    ui.key("ENTER");
    CHECK(broadcast.starts == 0 && ui.notice == "Type a message first.");
    ui.key("DOWN");
    CHECK(ui.messageAction == 1);
    ui.key("ENTER");
    CHECK(ui.screen == app::Screen::Message);

    for (char c : std::string("AAAAA")) ui.key(std::string(1, c));
    const auto expected = ui.machine.state();
    ui.key("ENTER");
    broadcast.failStart = true;
    ui.key("ENTER");
    CHECK(ui.screen == app::Screen::MessageActions && !ui.broadcastStatus().active);
    CHECK(ui.notice == "Could not start Wi-Fi. Retry.");
    broadcast.failStart = false;
    ui.key("ENTER");
    CHECK(ui.screen == app::Screen::Broadcasting && ui.broadcastStatus().ssid == "BDZGO");
    ui.key("RESET");
    ui.key("A");
    CHECK(ui.broadcastStatus().ssid == "BDZGO");
    broadcast.failStop = true;
    ui.key("ESC");
    CHECK(ui.screen == app::Screen::Broadcasting && ui.broadcastStatus().active);
    CHECK(ui.notice == "Could not stop Wi-Fi. Retry.");
    broadcast.failStop = false;
    ui.key("ENTER");
    CHECK(ui.screen == app::Screen::MessageActions && broadcast.stops == 2);
    CHECK(!ui.broadcastStatus().active);
    ui.key("ESC");
    CHECK(ui.screen == app::Screen::Message && ui.input == "AAAAA" && ui.output == "BDZGO");
    CHECK(ui.machine.state().positions == expected.positions);
    CHECK(ui.machine.state().lettersProcessed == expected.lettersProcessed);
    ui.key("BACKSPACE");
    CHECK(ui.output == "BDZG" && enigma::letters(ui.machine.state().positions) == "AAE");

    ui.key("RESET");
    for (size_t i = 0; i < app::Broadcast::MaxSsidBytes; ++i) ui.key("A");
    ui.key("ENTER");
    ui.key("ENTER");
    CHECK(ui.broadcastStatus().ssid.size() == app::Broadcast::MaxSsidBytes);
    ui.key("ESC");
    CHECK(ui.screen == app::Screen::MessageActions && !ui.broadcastStatus().active);
    ui.key("ESC");
    ui.key("A");
    ui.key("ENTER");
    ui.key("ENTER");
    CHECK(ui.notice == "SSID limit: 32 bytes." && broadcast.starts == 3);
    CHECK(ui.screen == app::Screen::MessageActions && ui.input.size() == 33);
}

int main() {
    FakeBroadcast broadcast;
    app::UiModel u(broadcast);
    u.key("LEFT"); CHECK(u.selected==4);
    u.key("RIGHT"); CHECK(u.selected==0);
    u.key("UP"); CHECK(u.selected==4);
    u.key("DOWN"); CHECK(u.selected==0);
    u.key("RIGHT"); u.key("ENTER"); CHECK(u.screen==app::Screen::Message);
    for(char c:std::string("AAAAA")) u.key(std::string(1,c));
    CHECK(u.output=="BDZGO"); CHECK(enigma::letters(u.machine.state().positions)=="AAF");
    u.key("BACKSPACE"); CHECK(u.output=="BDZG"); CHECK(enigma::letters(u.machine.state().positions)=="AAE");
    u.key("A"); CHECK(u.output=="BDZGO");
    u.key(" "); u.key("BACKSPACE"); CHECK(u.machine.state().lettersProcessed==5);
    u.key("RESET"); CHECK(u.input.empty()); CHECK(enigma::letters(u.machine.state().positions)=="AAA");
    enigma::Config c; c.rotors={{3,2,1}}; enigma::parseLetters("KDQ",c.initial); u.machine.configure(c);
    u.key("A"); u.key("A"); CHECK(enigma::letters(u.machine.state().positions)=="LFS");
    auto output=u.output; u.key("BACKSPACE"); CHECK(enigma::letters(u.machine.state().positions)=="KER");
    u.key("A"); CHECK(u.output==output); CHECK(enigma::letters(u.machine.state().positions)=="LFS");
    u.key("ESC"); CHECK(u.screen==app::Screen::Menu);
    u.key("LEFT"); u.key("ENTER"); CHECK(u.screen==app::Screen::Configure);
    u.key("ENTER"); CHECK(u.screen==app::Screen::Edit);
    for(int i=0;i<3;++i) u.key("BACKSPACE");
    u.key("1");u.key("1");u.key("1");u.key("ENTER");CHECK(u.screen==app::Screen::Edit);
    u.key("ESC");CHECK(u.screen==app::Screen::Configure);
    for(int i=0;i<5;++i) { u.key("DOWN"); }
    u.key("ENTER");CHECK(u.screen==app::Screen::Message);
    CHECK(u.input.empty());
    for(size_t i=0;i<app::UiModel::MaxMessage;++i)u.key("A");
    auto state=u.machine.state();u.key("A");CHECK(u.input.size()==app::UiModel::MaxMessage);
    CHECK(state.positions==u.machine.state().positions);CHECK(state.lettersProcessed==u.machine.state().lettersProcessed);
    for(size_t i=0;i<app::UiModel::MaxMessage;++i)u.key("BACKSPACE");
    CHECK(u.input.empty()); CHECK(u.machine.state().positions==c.initial); CHECK(u.machine.state().lettersProcessed==0);
    testBroadcast();
    std::cout<<"PASS UI navigation, boundaries, exact undo, and Wi-Fi broadcast lifecycle\n";
}

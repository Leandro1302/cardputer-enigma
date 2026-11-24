#include "UiModel.h"
#include <cstdlib>
#include <iostream>
#define CHECK(e) do { if(!(e)) {std::cerr<<"UI FAIL "<<__LINE__<<": " #e "\n";std::exit(1);} } while(0)
int main() {
    app::UiModel u;
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
    std::cout<<"PASS UI navigation, validation, boundaries, and exact undo across double-step\n";
}

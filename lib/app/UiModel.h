#pragma once
#include "Broadcast.h"
#include "Enigma.h"
#include <string>
#include <vector>

namespace app {
enum class Screen { Menu, Configure, Edit, Message, MessageActions, Broadcasting, Summary, Help };
class UiModel {
public:
    explicit UiModel(Broadcast& broadcast) : broadcast_(broadcast) {}
    static constexpr int MenuCount = 5;
    static constexpr size_t MaxMessage = 1024;
    enigma::Machine machine;
    Screen screen = Screen::Menu;
    int selected = 0;
    int field = 0;
    int messageAction = 0;
    size_t scroll = 0;
    std::string input, output, editor, notice;
    enigma::Config draft;
    unsigned revision = 0;
    void key(const std::string& key);
    void resetMessage();
    BroadcastStatus broadcastStatus() const { return broadcast_.status(); }
    const char* screenName() const;
    std::string fieldValue(int index) const;
    static const char* fieldName(int index);
private:
    Broadcast& broadcast_;
    std::vector<enigma::State> history_;
    void enter();
    void commitEdit();
};
}

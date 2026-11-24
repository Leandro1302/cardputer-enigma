#pragma once
#include "Enigma.h"
#include <string>
#include <vector>

namespace app {
enum class Screen { Menu, Configure, Edit, Message, Summary, Help };
class UiModel {
public:
    static constexpr int MenuCount = 5;
    static constexpr size_t MaxMessage = 1024;
    enigma::Machine machine;
    Screen screen = Screen::Menu;
    int selected = 0;
    int field = 0;
    size_t scroll = 0;
    std::string input, output, editor, notice;
    enigma::Config draft;
    unsigned revision = 0;
    void key(const std::string& key);
    void resetMessage();
    const char* screenName() const;
    std::string fieldValue(int index) const;
    static const char* fieldName(int index);
private:
    std::vector<enigma::State> history_;
    void enter();
    void commitEdit();
};
}

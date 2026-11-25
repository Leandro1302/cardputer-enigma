#include "Display.h"
#include "Artwork.h"

#include <algorithm>
#include <array>

namespace {
using namespace artwork;
constexpr uint32_t StartupMs = 2600;
constexpr uint32_t FrameMs = 30;

struct MenuCard {
    Icon icon;
    const char* label;
};
constexpr std::array<MenuCard, app::UiModel::MenuCount> MenuCards{{
    {Icon::Gear, "Configure machine"},
    {Icon::Cipher, "Encrypt / decrypt"},
    {Icon::Settings, "Current configuration"},
    {Icon::Reset, "Reset message & rotors"},
    {Icon::Help, "Help & controls"},
}};

std::string rotorOrder(const enigma::Config& config) {
    return std::string(enigma::rotorName(config.rotors[0])) + " / " +
           enigma::rotorName(config.rotors[1]) + " / " + enigma::rotorName(config.rotors[2]);
}
} // namespace

bool DisplayRenderer::begin() {
    auto& display = M5Cardputer.Display;
    display.setRotation(Rotation);
    display.setBrightness(100);
    canvas_.setColorDepth(16);
    ready = display.width() == Width && display.height() == Height &&
            canvas_.createSprite(Width, Height) != nullptr;
    canvas_.setTextWrap(false);
    canvas_.setFont(&lgfx::fonts::Font0);
    return ready;
}

void DisplayRenderer::start(const app::UiModel& ui, uint32_t now) {
    if (!ready) return;
    if (!ui.notice.empty()) {
        render(ui);
        return;
    }
    startupActive_ = true;
    startupAt_ = now;
    frameAt_ = now;
    drawStartup(0);
}

void DisplayRenderer::tick(const app::UiModel& ui, uint32_t now) {
    if (!ready) return;
    if (startupActive_) {
        const uint32_t elapsed = now - startupAt_;
        if (elapsed >= StartupMs || ui.revision != renderedRevision_) {
            render(ui);
        } else if (now - frameAt_ >= FrameMs) {
            frameAt_ = now;
            drawStartup(elapsed);
        }
    } else if (ui.revision != renderedRevision_ && now - frameAt_ >= FrameMs) {
        render(ui);
    }
}

void DisplayRenderer::present() {
    canvas_.pushSprite(0, 0);
    ++frames;
}

void DisplayRenderer::drawStartup(uint32_t elapsed) {
    caseBackground(canvas_);
    canvas_.fillRoundRect(10, 8, 220, 119, 10, Enamel);
    canvas_.drawRoundRect(10, 8, 220, 119, 10, Border);
    for (int x : {19, 221}) {
        canvas_.fillCircle(x, 17, 2, BrassDark);
        canvas_.fillCircle(x, 118, 2, BrassDark);
    }

    // The smaller gears counter-rotate in proportion to their tooth counts.
    const float rotation = elapsed * .0024f;
    const float outerRotation = -rotation * 16.f / 12.f + .261799f;
    gear(canvas_, 74, 52, 22, 12, outerRotation, Enamel);
    gear(canvas_, 166, 52, 22, 12, outerRotation, Enamel);
    gear(canvas_, 120, 52, 28, 16, rotation, Enamel);

    if (elapsed >= 210) {
        canvas_.setTextSize(1);
        canvas_.setTextColor(elapsed < 340 ? BrassDark : elapsed < 460 ? Brass : BrassLight);
        canvas_.drawCenterString("En?gma", 120, 86, &lgfx::fonts::FreeSerifBold18pt7b);
        if (elapsed >= 460) {
            canvas_.drawFastHLine(44, 107, 19, BrassDark);
            canvas_.drawFastHLine(177, 107, 19, BrassDark);
        }
    }
    present();
    ++startupFrames;
}

void DisplayRenderer::text(int x, int y, const std::string& value, uint16_t color, int size) {
    canvas_.setTextSize(size);
    canvas_.setTextColor(color);
    canvas_.setCursor(x, y);
    canvas_.print(value.c_str());
}

void DisplayRenderer::title(const char* value) {
    canvas_.fillRect(0, 0, Width, 17, Walnut);
    canvas_.drawFastHLine(0, 16, Width, BrassDark);
    text(7, 4, value, BrassLight);
    text(217, 4, "M3", Muted);
}

void DisplayRenderer::footer(const std::string& value) {
    canvas_.fillRect(0, 121, Width, 14, Walnut);
    canvas_.drawFastHLine(0, 121, Width, Border);
    text(5, 125, value.substr(0, 38), Muted);
}

void DisplayRenderer::drawMenu(const app::UiModel& ui) {
    caseBackground(canvas_);
    title("ENIGMA");
    canvas_.fillRoundRect(12, 23, 216, 93, 7, Enamel);
    canvas_.drawRoundRect(12, 23, 216, 93, 7, Border);
    chevron(canvas_, 30, 53, -1, Brass);
    chevron(canvas_, 210, 53, 1, Brass);

    const auto& card = MenuCards[ui.selected];
    icon(canvas_, card.icon, 120, 53);
    canvas_.setTextSize(1);
    canvas_.setTextColor(Cream);
    canvas_.drawCenterString(card.label, 120, 81, &lgfx::fonts::Font2);
    for (int i = 0; i < app::UiModel::MenuCount; ++i) {
        const int x = 120 + (i - app::UiModel::MenuCount / 2) * 13;
        if (i == ui.selected) canvas_.fillRoundRect(x - 4, 105, 9, 4, 2, BrassLight);
        else canvas_.fillCircle(x, 107, 2, Border);
    }
    footer(ui.notice.empty() ? ", / Move      Enter Select" : ui.notice);
}

void DisplayRenderer::render(const app::UiModel& ui) {
    if (!ready) return;
    startupActive_ = false;
    renderedRevision_ = ui.revision;
    canvas_.fillScreen(Enamel);
    const auto& config = ui.machine.config();
    switch (ui.screen) {
    case app::Screen::Menu:
        drawMenu(ui);
        break;
    case app::Screen::Configure:
        title("MACHINE SETUP");
        for (int i = 0; i < 7; ++i) {
            const bool selected = i == ui.field;
            if (selected) {
                const int y = 21 + i * 14;
                canvas_.fillRoundRect(3, y, 234, 14, 2, Walnut);
                canvas_.drawFastVLine(3, y + 2, 10, Brass);
            }
            auto value = ui.fieldValue(i);
            if (value.size() > 14) value = value.substr(0, 11) + "...";
            text(8, 24 + i * 14, app::UiModel::fieldName(i), selected ? BrassLight : Cream);
            text(143, 24 + i * 14, value, selected ? Cream : Muted);
        }
        footer("Enter Edit   ` Cancel");
        break;
    case app::Screen::Edit: {
        title("EDIT SETTING");
        text(6, 26, app::UiModel::fieldName(ui.field), BrassLight);
        const char* hints[] = {
            "3 distinct digits, e.g. 368", "3 letters, e.g. UZV",
            "3 letters: A=01 ... Z=26", "B or C (wide M3 reflector)",
            "Up to 10 pairs: AB CD EF"
        };
        text(6, 43, hints[ui.field], Muted);
        canvas_.fillRoundRect(3, 61, 234, 34, 3, Panel);
        canvas_.drawRoundRect(3, 61, 234, 34, 3, Border);
        text(7, 67, ui.editor.substr(0, 37), Cream);
        if (ui.editor.size() > 37) {
            text(7, 79, ui.editor.substr(37), Cream);
            text(7 + (ui.editor.size() - 37) * 6, 79, "_", BrassLight);
        } else {
            text(7 + ui.editor.size() * 6, 67, "_", BrassLight);
        }
        text(6, 104, ui.notice.substr(0, 38), Muted);
        footer("Enter Save   ` Cancel");
        break;
    }
    case app::Screen::Message: {
        title("ENCIPHER / DECIPHER");
        for (int i = 0; i < 3; ++i) {
            const int x = 6 + i * 58;
            canvas_.fillRoundRect(x, 21, 52, 25, 3, Panel);
            canvas_.drawRoundRect(x, 21, 52, 25, 3, Border);
            text(x + 4, 30, enigma::rotorName(config.rotors[i]), Muted);
            text(x + 31, 26, std::string(1, 'A' + ui.machine.state().positions[i]), BrassLight, 2);
        }
        text(184, 25, "LETTERS", Muted);
        text(184, 37, std::to_string(ui.machine.state().lettersProcessed), Cream);
        const size_t end = ui.input.size() > ui.scroll ? ui.input.size() - ui.scroll : 0;
        const size_t start = end > 76 ? end - 76 : 0;
        const auto input = ui.input.substr(start, end - start);
        const auto output = ui.output.substr(start, end - start);
        text(6, 51, "IN", Muted);
        text(6, 62, input.substr(0, 38), Cream);
        if (input.size() > 38) text(6, 72, input.substr(38), Cream);
        canvas_.drawFastHLine(6, 81, 228, Border);
        text(6, 85, "OUT", Brass);
        text(6, 96, output.substr(0, 38), BrassLight);
        if (output.size() > 38) text(6, 106, output.substr(38), BrassLight);
        footer(ui.notice.empty() ? "Enter Actions  Fn+BS Reset  ` Menu" : ui.notice);
        break;
    }
    case app::Screen::MessageActions: {
        title("MESSAGE ACTIONS");
        const char* actions[] = {"Broadcast as Wi-Fi", "Back"};
        for (int i = 0; i < 2; ++i) {
            const bool selected = ui.messageAction == i;
            const int y = 27 + i * 29;
            canvas_.fillRoundRect(11, y, 218, 23, 4, selected ? Walnut : Panel);
            canvas_.drawRoundRect(11, y, 218, 23, 4, selected ? Brass : Border);
            if (selected) canvas_.fillCircle(22, y + 11, 2, BrassLight);
            text(31, y + 8, actions[i], selected ? BrassLight : Cream);
        }
        text(12, 93, "SSID is visible to nearby devices.", Muted);
        text(12, 106, ui.notice.empty() ? "1-32 bytes of ciphertext" : ui.notice, Brass);
        footer("; . Move    Enter Select    ` Back");
        break;
    }
    case app::Screen::Broadcasting: {
        title("WI-FI BROADCAST");
        const auto status = ui.broadcastStatus();
        canvas_.fillCircle(14, 30, 3, status.active ? BrassLight : Muted);
        text(23, 27, status.active ? "ACTIVE   /   OPEN NETWORK" : "INACTIVE", BrassLight);
        text(12, 45, "NETWORK NAME (SSID)", Muted);
        canvas_.fillRoundRect(11, 56, 218, 21, 4, Panel);
        canvas_.drawRoundRect(11, 56, 218, 21, 4, Border);
        text(17, 63, ui.output, Cream);
        canvas_.fillRoundRect(11, 85, 218, 22, 4, Walnut);
        canvas_.drawRoundRect(11, 85, 218, 22, 4, Brass);
        text(20, 92, "Stop Broadcasting", BrassLight);
        if (!ui.notice.empty()) text(12, 111, ui.notice, Brass);
        footer("Enter Stop    ` Back");
        break;
    }
    case app::Screen::Summary:
        title("CURRENT CONFIGURATION");
        text(6, 26, "Rotors: " + rotorOrder(config), BrassLight);
        text(6, 42, "Start: " + enigma::letters(config.initial) + "    Rings: " + enigma::letters(config.rings), Cream);
        text(6, 58, "Now:   " + enigma::letters(ui.machine.state().positions) + "    UKW: " + std::string(1, config.reflector), Cream);
        canvas_.drawFastHLine(6, 70, 228, Border);
        text(6, 76, "Plugboard", Muted);
        text(6, 90, config.plugboard().empty() ? "No cables" : config.plugboard(), Cream);
        text(6, 108, "Settings stay in RAM", Muted);
        footer("Enter / ` Back");
        break;
    case app::Screen::Help:
        title("QUICK GUIDE");
        text(6, 25, "Same settings encrypt AND decrypt.", BrassLight);
        text(6, 39, "Reset rotors before a new message.", Cream);
        text(6, 53, "Letters step; spaces/punctuation", Cream);
        text(6, 64, "are preserved without stepping.", Cream);
        text(6, 78, "Backspace undoes the last key.", Cream);
        text(6, 92, "Fn+Backspace clears the message.", Cream);
        text(6, 106, "Enter actions; Fn+;/. scrolls.", Muted);
        footer("Enter / ` Back");
        break;
    }
    present();
    frameAt_ = millis();
}

std::string DisplayRenderer::rowHex(unsigned row) {
    if (!ready || row >= Height) return "";
    static const char* hex = "0123456789abcdef";
    std::string result;
    result.reserve(Width * 4);
    for (unsigned x = 0; x < Width; ++x) {
        const uint16_t pixel = canvas_.readPixel(x, row);
        for (int shift = 12; shift >= 0; shift -= 4) result += hex[(pixel >> shift) & 15];
    }
    return result;
}

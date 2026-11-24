#pragma once
#include <M5Cardputer.h>

namespace artwork {
constexpr uint16_t rgb(unsigned r, unsigned g, unsigned b) {
    return ((r & 0xf8U) << 8) | ((g & 0xfcU) << 3) | (b >> 3);
}
// Walnut case, dark enamel panels, aged brass and cream engraving.
inline constexpr auto Walnut = rgb(43, 27, 21);
inline constexpr auto Grain = rgb(52, 33, 25);
inline constexpr auto Enamel = rgb(23, 18, 15);
inline constexpr auto Panel = rgb(35, 25, 20);
inline constexpr auto Border = rgb(122, 86, 54);
inline constexpr auto Brass = rgb(219, 177, 104);
inline constexpr auto BrassLight = rgb(246, 212, 146);
inline constexpr auto BrassDark = rgb(158, 113, 65);
inline constexpr auto Cream = rgb(248, 234, 207);
inline constexpr auto Muted = rgb(201, 176, 145);

enum class Icon { Gear, Cipher, Settings, Reset, Help };
void caseBackground(M5Canvas& canvas);
void gear(M5Canvas& canvas, int x, int y, int radius, int teeth, float angle, uint16_t cutout);
void icon(M5Canvas& canvas, Icon kind, int x, int y);
void chevron(M5Canvas& canvas, int x, int y, int direction, uint16_t color);
} // namespace artwork

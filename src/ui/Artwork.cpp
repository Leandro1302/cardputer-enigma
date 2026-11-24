#include "Artwork.h"
#include <cmath>

namespace artwork {
namespace {
constexpr float Turn = 6.2831853f;
struct Point { int x, y; };

Point polar(int x, int y, int radius, float angle) {
    return {x + static_cast<int>(std::lround(radius * std::cos(angle))),
            y + static_cast<int>(std::lround(radius * std::sin(angle)))};
}

void quad(M5Canvas& canvas, Point a, Point b, Point c, Point d, uint16_t color) {
    canvas.fillTriangle(a.x, a.y, b.x, b.y, c.x, c.y, color);
    canvas.fillTriangle(a.x, a.y, c.x, c.y, d.x, d.y, color);
}
} // namespace

void caseBackground(M5Canvas& canvas) {
    canvas.fillScreen(Walnut);
    for (int y = 4; y < 135; y += 5) {
        const int start = (y * 11) % 43;
        canvas.drawFastHLine(start, y, 240 - start - (y * 7) % 22, Grain);
    }
}

void gear(M5Canvas& canvas, int x, int y, int radius, int teeth, float angle, uint16_t cutout) {
    const int root = radius - 4;
    const float pitch = Turn / teeth;
    canvas.fillCircle(x, y, root, BrassDark);
    for (int tooth = 0; tooth < teeth; ++tooth) {
        const float offset = angle + pitch * tooth;
        const auto a = polar(x, y, root - 2, offset - pitch * .34f);
        const auto b = polar(x, y, radius, offset - pitch * .19f);
        const auto c = polar(x, y, radius, offset + pitch * .19f);
        const auto d = polar(x, y, root - 2, offset + pitch * .34f);
        quad(canvas, a, b, c, d, Brass);
        canvas.drawLine(b.x, b.y, c.x, c.y, BrassLight);
    }
    canvas.fillCircle(x, y, root - 1, Brass);
    canvas.drawCircle(x, y, root - 3, BrassDark);
    canvas.drawCircle(x, y, root - 4, BrassLight);

    // Three moving openings make rotation apparent even at small radii.
    for (int spoke = 0; spoke < 3; ++spoke) {
        const auto hole = polar(x, y, root * 3 / 5, angle + spoke * Turn / 3);
        const int holeRadius = root / 4;
        canvas.fillCircle(hole.x, hole.y, holeRadius, BrassDark);
        canvas.fillCircle(hole.x, hole.y, holeRadius - 1, cutout);
    }
    canvas.fillCircle(x, y, root / 4 + 1, BrassLight);
    canvas.fillCircle(x, y, root / 5, BrassDark);
    canvas.drawCircle(x, y, root / 5, Enamel);
}

void chevron(M5Canvas& canvas, int x, int y, int direction, uint16_t color) {
    for (int offset = 0; offset < 2; ++offset) {
        canvas.drawLine(x - direction * 4, y - 6 + offset, x + direction * 2, y + offset, color);
        canvas.drawLine(x + direction * 2, y + offset, x - direction * 4, y + 6 + offset, color);
    }
}

void icon(M5Canvas& canvas, Icon kind, int x, int y) {
    if (kind == Icon::Gear) {
        gear(canvas, x, y, 22, 12, .2f, Enamel);
        return;
    }

    canvas.drawCircle(x, y, 22, Border);
    canvas.drawCircle(x, y, 20, BrassDark);
    switch (kind) {
    case Icon::Cipher:
        // Two wheels with opposite arrows for the reversible cipher.
        canvas.drawRoundRect(x - 14, y - 12, 12, 24, 3, Brass);
        canvas.drawRoundRect(x + 2, y - 12, 12, 24, 3, Brass);
        canvas.drawFastHLine(x - 11, y - 5, 6, BrassLight);
        canvas.drawFastHLine(x + 5, y + 5, 6, BrassLight);
        canvas.fillTriangle(x - 11, y + 2, x - 5, y + 2, x - 8, y + 7, Brass);
        canvas.fillTriangle(x + 5, y - 2, x + 11, y - 2, x + 8, y - 7, Brass);
        break;
    case Icon::Settings:
        canvas.drawRoundRect(x - 11, y - 14, 22, 28, 3, Brass);
        for (int line = -7; line <= 7; line += 7) {
            canvas.fillCircle(x - 6, y + line, 1, BrassLight);
            canvas.drawFastHLine(x - 2, y + line, 8, Brass);
        }
        break;
    case Icon::Reset:
        for (int i = 0; i < 44; ++i) {
            const float angle = .16f + i * .12f;
            const auto a = polar(x, y, 12, angle);
            const auto b = polar(x, y, 12, angle + .12f);
            canvas.drawLine(a.x, a.y, b.x, b.y, BrassLight);
        }
        canvas.fillTriangle(x + 12, y - 11, x + 14, y - 3, x + 6, y - 6, BrassLight);
        break;
    case Icon::Help:
        canvas.drawLine(x, y - 9, x, y + 12, Brass);
        canvas.drawLine(x, y - 9, x - 12, y - 12, Brass);
        canvas.drawLine(x, y - 9, x + 12, y - 12, Brass);
        canvas.drawLine(x, y + 12, x - 12, y + 9, Brass);
        canvas.drawLine(x, y + 12, x + 12, y + 9, Brass);
        for (int direction : {-1, 1}) {
            canvas.drawFastVLine(x + direction * 12, y - 12, 22, Brass);
            for (int line = -6; line <= 5; line += 5) {
                canvas.drawLine(x + direction * 3, y + line, x + direction * 9, y + line - 2, BrassLight);
            }
        }
        break;
    case Icon::Gear:
        break;
    }
}
} // namespace artwork

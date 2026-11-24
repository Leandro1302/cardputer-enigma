#include "Enigma.h"
#include <cstdlib>
#include <iostream>
#include <string>
using namespace enigma;
static unsigned assertions = 0;
#define CHECK(expr) do { ++assertions; if (!(expr)) { std::cerr << "FAIL line " << __LINE__ << ": " #expr "\n"; std::exit(1); } } while (0)

int main() {
    const char* wiring[] = {"EKMFLGDQVZNTOWYHXUSPAIBRCJ", "AJDKSIRUXBLHWTMCQGZNPYFVOE",
        "BDFHJLCPRTXVZNYEIWGAKMUSQO", "ESOVPZJAYQUIRHXLNFTGKDCMWB", "VZBRGITYUPSDNHLXAWMJQOFECK",
        "JPGVOUMFYQBENHZRDKASXLICTW", "NZJHGRCXMYSWBOUFAIVLPEKQDT", "FKQHTLXOCBJSPDZRAMEWNIUYGV"};
    const char* notch[] = {"Q", "E", "V", "J", "Z", "MZ", "MZ", "MZ"};
    for (uint8_t id = 1; id <= 8; ++id) {
        CHECK(std::string(rotorWiring(id)) == wiring[id-1]);
        for (uint8_t p = 0; p < 26; ++p) {
            CHECK(atNotch(id, p) == (std::string(notch[id-1]).find('A'+p) != std::string::npos));
            for (uint8_t r = 0; r < 26; ++r) for (uint8_t x = 0; x < 26; ++x) {
                auto y = transform(id, x, p, r, false);
                CHECK(y < 26);
                CHECK(transform(id, y, p, r, true) == x);
                if (p == r) CHECK(y == wiring[id-1][x] - 'A');
            }
        }
    }
    CHECK(transform(1, 0, 1, 0, false) == 9);
    CHECK(transform(1, 0, 0, 1, false) == 10);
    CHECK(transform(0, 0, 0, 0, false) == 255);
    CHECK(transform(9, 0, 0, 0, true) == 255);
    CHECK(transform(1, 26, 0, 0, false) == 255);
    for (char r : {'B', 'C'}) for (uint8_t x = 0; x < 26; ++x) {
        CHECK(reflect(r, x) != x); CHECK(reflect(r, reflect(r, x)) == x);
    }
    CHECK(reflect('B', 0) == 24); CHECK(reflect('C', 0) == 5);
    CHECK(reflect('A', 0) == 255);
    Config c;
    CHECK(c.setPlugboard("av, bs CG DL FU HZ IN KM OW RX"));
    CHECK(c.plugs[0] == 21 && c.plugs[21] == 0 && c.plugs[4] == 4);
    const auto plugs = c.plugs;
    for (auto bad : {"AA", "AB AC", "AB AB", "A", "ABC", "12", "AB-CD", "AB CD EF GH IJ KL MN OP QR ST UV"}) {
        CHECK(!c.setPlugboard(bad)); CHECK(c.plugs == plugs);
    }
    CHECK(c.setPlugboard("")); CHECK(c.validate() == Error::None);
    Machine m;
    CHECK(m.process(std::string("AAAAA")) == "BDZGO");
    m.reset(); CHECK(m.process(std::string("BDZGO")) == "AAAAA");
    m.reset(); CHECK(m.process(std::string("hello world!")) == "ILBDA AMTAZ!");
    CHECK(m.state().lettersProcessed == 10);
    m.reset(); CHECK(m.process(std::string("hello")) == "ILBDA");
    CHECK(m.process(std::string("world")) == "AMTAZ");
    const auto before = m.state();
    m.process('X'); CHECK(m.restore(before)); CHECK(m.state().positions == before.positions);
    State invalid = before; invalid.positions[0] = 26;
    CHECK(!m.restore(invalid)); CHECK(m.state().positions == before.positions);
    c.rotors = {{3, 2, 1}}; parseLetters("KDO", c.initial);
    CHECK(m.configure(c) == Error::None);
    for (const char* expected : {"KDP", "KDQ", "KER", "LFS", "LFT", "LFU"}) {
        m.process('A'); CHECK(letters(m.state().positions) == expected);
    }
    // Non-default rings must not shift the turnover letter in the window.
    parseLetters("BUL", c.rings); CHECK(m.configure(c) == Error::None);
    for (const char* expected : {"KDP", "KDQ", "KER", "LFS"}) {
        m.process('A'); CHECK(letters(m.state().positions) == expected);
    }
    for (uint8_t naval : {6, 7, 8}) {
        c = Config(); c.rotors = {{1, naval, 2}};
        for (char n : {'M', 'Z'}) {
            parseLetters(std::string("A") + n + "A", c.initial);
            CHECK(m.configure(c) == Error::None); m.process('A');
            CHECK(m.state().positions[0] == 1);
            CHECK(m.state().positions[1] == (n-'A'+1)%26);
        }
        c.rotors = {{1, 2, naval}};
        for (char n : {'M', 'Z'}) {
            parseLetters(std::string("AA") + n, c.initial);
            CHECK(m.configure(c) == Error::None); m.process('A');
            CHECK(m.state().positions[0] == 0 && m.state().positions[1] == 1);
        }
    }
    // Published historical Scharnhorst intercept: all settings in docs/testing.md.
    c = Config(); c.rotors = {{3, 6, 8}}; parseLetters("AHM", c.rings); parseLetters("UZV", c.initial);
    CHECK(c.setPlugboard("AN EZ HK IJ LR MQ OT PV SW UX")); CHECK(m.configure(c) == Error::None);
    const std::string cipher = "YKAENZAPMSCHZBFOCUVMRMDPYCOFHADZIZMEFXTHFLOLPZLFGGBOTGOXGRETDWTJIQHLMXVJWKZUASTR";
    const std::string plain = "STEUEREJTANAFJORDJANSTANDORTQUAAACCCVIERNEUNNEUNZWOFAHRTZWONULSMXXSCHARNHORSTHCO";
    CHECK(m.process(cipher) == plain);
    m.reset(); CHECK(m.process(plain) == cipher);
    // Published Py-Enigma manual example: message key and nontrivial ring/plug settings.
    c = Config(); c.rotors = {{2,4,5}}; parseLetters("BUL", c.rings); parseLetters("WXC", c.initial);
    CHECK(c.setPlugboard("AV BS CG DL FU HZ IN KM OW RX")); CHECK(m.configure(c) == Error::None);
    CHECK(m.process(std::string("KCH")) == "BLA");
    parseLetters("BLA", c.initial); CHECK(m.configure(c) == Error::None);
    CHECK(m.process(std::string("NIBLFMYMLLUFWCASCSSNVHAZ")) == "THEXRUSSIANSXAREXCOMINGX");
    // Invalid configurations are transactional.
    auto state = m.state();
    auto bad = c; bad.rotors[0] = 0; CHECK(m.configure(bad) == Error::Rotor);
    bad = c; bad.rotors[1] = bad.rotors[0]; CHECK(m.configure(bad) == Error::DuplicateRotor);
    bad = c; bad.rings[0] = 26; CHECK(m.configure(bad) == Error::Ring);
    bad = c; bad.initial[0] = 26; CHECK(m.configure(bad) == Error::Position);
    bad = c; bad.reflector = 'D'; CHECK(m.configure(bad) == Error::Reflector);
    bad = c; bad.plugs[0] = 255; CHECK(m.configure(bad) == Error::Plugboard);
    CHECK(m.state().positions == state.positions && m.state().lettersProcessed == state.lettersProcessed);
    CHECK(selfTest());
    std::cout << "PASS " << assertions << " native assertions\n";
}

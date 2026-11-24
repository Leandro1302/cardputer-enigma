#include "Enigma.h"
#include <cstring>

namespace enigma {
namespace {
constexpr const char* names[] = {"", "I", "II", "III", "IV", "V", "VI", "VII", "VIII"};
constexpr const char* wirings[] = {"",
    "EKMFLGDQVZNTOWYHXUSPAIBRCJ", "AJDKSIRUXBLHWTMCQGZNPYFVOE",
    "BDFHJLCPRTXVZNYEIWGAKMUSQO", "ESOVPZJAYQUIRHXLNFTGKDCMWB",
    "VZBRGITYUPSDNHLXAWMJQOFECK", "JPGVOUMFYQBENHZRDKASXLICTW",
    "NZJHGRCXMYSWBOUFAIVLPEKQDT", "FKQHTLXOCBJSPDZRAMEWNIUYGV"};
constexpr const char* notches[] = {"", "Q", "E", "V", "J", "Z", "ZM", "ZM", "ZM"};
constexpr const char* reflectorB = "YRUHQSLDPXNGOKMIEBFZCWVJAT";
constexpr const char* reflectorC = "FVPJIAOYEDRZXWGCTKUQSBNMHL";
uint8_t mod26(int x) { return static_cast<uint8_t>((x % 26 + 26) % 26); }
}
const char* errorName(Error e) {
    switch (e) {
    case Error::None: return "OK";
    case Error::Rotor: return "INVALID_ROTOR";
    case Error::DuplicateRotor: return "DUPLICATE_ROTOR";
    case Error::Position: return "INVALID_POSITIONS";
    case Error::Ring: return "INVALID_RINGS";
    case Error::Reflector: return "INVALID_REFLECTOR";
    case Error::Plugboard: return "INVALID_PLUGBOARD";
    }
    return "INVALID_CONFIG";
}
char uppercase(char c) { return c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c; }
bool isLetter(char c) { c = uppercase(c); return c >= 'A' && c <= 'Z'; }
std::string letters(const Positions& p) {
    std::string result;
    for (auto x : p) result += static_cast<char>('A' + x);
    return result;
}
bool parseLetters(const std::string& text, Positions& values) {
    if (text.size() != 3) return false;
    Positions parsed{};
    for (unsigned i = 0; i < 3; ++i) {
        if (!isLetter(text[i])) return false;
        parsed[i] = uppercase(text[i]) - 'A';
    }
    values = parsed;
    return true;
}
const char* rotorName(uint8_t id) { return id >= 1 && id <= 8 ? names[id] : "?"; }
const char* rotorWiring(uint8_t id) { return id >= 1 && id <= 8 ? wirings[id] : ""; }
bool atNotch(uint8_t id, uint8_t window) {
    // The notch is fixed to the alphabet ring. Test the visible letter, not
    // window-ring. Changing Ringstellung changes wiring alignment, not this test.
    return id >= 1 && id <= 8 && window < 26 && std::strchr(notches[id], 'A' + window);
}
uint8_t transform(uint8_t id, uint8_t input, uint8_t window, uint8_t ring, bool reverse) {
    if (id < 1 || id > 8 || input > 25 || window > 25 || ring > 25) return 0xff;
    const auto shifted = mod26(input + window - ring);
    int mapped;
    if (reverse) mapped = static_cast<int>(std::strchr(wirings[id], 'A' + shifted) - wirings[id]);
    else mapped = wirings[id][shifted] - 'A';
    return mod26(mapped - window + ring);
}
uint8_t reflect(char r, uint8_t input) {
    if (input > 25 || (r != 'B' && r != 'C')) return 0xff;
    return (r == 'B' ? reflectorB : reflectorC)[input] - 'A';
}
Config::Config() { for (unsigned i = 0; i < 26; ++i) plugs[i] = i; }
Error Config::validate() const {
    for (unsigned i = 0; i < 3; ++i) {
        if (rotors[i] < 1 || rotors[i] > 8) return Error::Rotor;
        for (unsigned j = 0; j < i; ++j) if (rotors[i] == rotors[j]) return Error::DuplicateRotor;
        if (rings[i] > 25) return Error::Ring;
        if (initial[i] > 25) return Error::Position;
    }
    if (reflector != 'B' && reflector != 'C') return Error::Reflector;
    unsigned connected = 0;
    for (unsigned i = 0; i < 26; ++i) {
        if (plugs[i] > 25 || plugs[plugs[i]] != i) return Error::Plugboard;
        if (plugs[i] != i) ++connected;
    }
    return connected <= 20 ? Error::None : Error::Plugboard;
}
bool Config::setPlugboard(const std::string& pairs) {
    std::array<uint8_t, 26> next{};
    std::array<bool, 26> used{};
    for (unsigned i = 0; i < 26; ++i) next[i] = i;
    unsigned count = 0;
    size_t pos = 0;
    while (pos < pairs.size()) {
        while (pos < pairs.size() && (pairs[pos] == ' ' || pairs[pos] == ',')) ++pos;
        if (pos == pairs.size()) break;
        if (pos + 1 >= pairs.size() || !isLetter(pairs[pos]) || !isLetter(pairs[pos + 1])) return false;
        auto a = uppercase(pairs[pos]) - 'A';
        auto b = uppercase(pairs[pos + 1]) - 'A';
        pos += 2;
        if (pos < pairs.size() && pairs[pos] != ' ' && pairs[pos] != ',') return false;
        if (a == b || used[a] || used[b] || ++count > 10) return false;
        used[a] = used[b] = true;
        next[a] = b; next[b] = a;
    }
    plugs = next;
    return true;
}
std::string Config::plugboard() const {
    std::string result;
    for (unsigned i = 0; i < 26; ++i) if (plugs[i] > i) {
        if (!result.empty()) result += ' ';
        result += static_cast<char>('A' + i);
        result += static_cast<char>('A' + plugs[i]);
    }
    return result;
}
Machine::Machine() { reset(); }
Error Machine::configure(const Config& c) {
    const auto error = c.validate();
    if (error != Error::None) return error;
    config_ = c; reset(); return Error::None;
}
bool Machine::restore(const State& s) {
    for (auto p : s.positions) if (p > 25) return false;
    state_ = s; return true;
}
void Machine::reset() { state_.positions = config_.initial; state_.lettersProcessed = 0; }
void Machine::step() {
    auto& p = state_.positions;
    const bool middle = atNotch(config_.rotors[1], p[1]);
    const bool right = atNotch(config_.rotors[2], p[2]);
    if (middle) p[0] = (p[0] + 1) % 26;
    if (middle || right) p[1] = (p[1] + 1) % 26;
    p[2] = (p[2] + 1) % 26;
}
char Machine::process(char c) {
    if (!isLetter(c)) return c;
    step();
    uint8_t x = config_.plugs[uppercase(c) - 'A'];
    for (int i = 2; i >= 0; --i) x = transform(config_.rotors[i], x, state_.positions[i], config_.rings[i], false);
    x = reflect(config_.reflector, x);
    for (int i = 0; i < 3; ++i) x = transform(config_.rotors[i], x, state_.positions[i], config_.rings[i], true);
    ++state_.lettersProcessed;
    return static_cast<char>('A' + config_.plugs[x]);
}
std::string Machine::process(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    for (char c : input) output += process(c);
    return output;
}
bool selfTest() {
    Machine m;
    if (m.process(std::string("AAAAA")) != "BDZGO") return false;
    Config c;
    c.rotors = {{3, 2, 1}}; c.initial = {{10, 3, 14}};
    if (m.configure(c) != Error::None) return false;
    for (const char* expected : {"KDP", "KDQ", "KER", "LFS", "LFT", "LFU"}) {
        m.process('A'); if (letters(m.state().positions) != expected) return false;
    }
    return true;
}
} // namespace enigma

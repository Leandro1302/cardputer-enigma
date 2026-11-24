#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace enigma {
using Positions = std::array<uint8_t, 3>;
enum class Error { None, Rotor, DuplicateRotor, Position, Ring, Reflector, Plugboard };
const char* errorName(Error error);
char uppercase(char c);
bool isLetter(char c);
std::string letters(const Positions& values);
bool parseLetters(const std::string& text, Positions& values);
const char* rotorName(uint8_t id);
const char* rotorWiring(uint8_t id);
bool atNotch(uint8_t id, uint8_t window);
uint8_t transform(uint8_t id, uint8_t input, uint8_t window, uint8_t ring, bool reverse);
uint8_t reflect(char reflector, uint8_t input);

struct Config {
    std::array<uint8_t, 3> rotors{{1, 2, 3}}; // Left to right from operator viewpoint.
    Positions rings{{0, 0, 0}};
    Positions initial{{0, 0, 0}};
    char reflector = 'B'; // Wide M3 reflectors, never M4 thin reflectors.
    std::array<uint8_t, 26> plugs{};
    Config();
    Error validate() const;
    bool setPlugboard(const std::string& pairs);
    std::string plugboard() const;
};

struct State {
    Positions positions{{0, 0, 0}};
    uint64_t lettersProcessed = 0;
};

class Machine {
public:
    Machine();
    Error configure(const Config& config); // Atomic: invalid input changes nothing.
    const Config& config() const { return config_; }
    const State& state() const { return state_; }
    bool restore(const State& state);
    void reset();
    char process(char input); // ASCII letters step; other bytes pass through unchanged.
    std::string process(const std::string& input);
    void step();
private:
    Config config_;
    State state_;
};
bool selfTest();
} // namespace enigma

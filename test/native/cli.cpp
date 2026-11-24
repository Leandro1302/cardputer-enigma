#include "Enigma.h"
#include <iostream>
#include <string>
int main(int argc, char** argv) {
    if (argc != 7) return 2;
    enigma::Config c;
    const std::string rotors = argv[1], reflector = argv[4];
    if (rotors.size() != 3 || reflector.size() != 1) return 2;
    for (unsigned i=0;i<3;++i) c.rotors[i] = rotors[i] - '0';
    c.reflector = reflector[0];
    if (!enigma::parseLetters(argv[2], c.rings) || !enigma::parseLetters(argv[3], c.initial) || !c.setPlugboard(argv[5])) return 2;
    enigma::Machine m;
    if (m.configure(c) != enigma::Error::None) return 2;
    std::cout << m.process(std::string(argv[6])) << '\n' << enigma::letters(m.state().positions) << '\n';
}

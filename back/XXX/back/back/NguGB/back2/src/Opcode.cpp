#include "Opcode.hpp"

#include <iostream>

using namespace std;

SimpleOpcode::SimpleOpcode(function<void()> opcode) : opcode(opcode) {}

void SimpleOpcode::show() { cout << "Simple opcode" << endl; }

void SimpleOpcode::run() { this->opcode(); }

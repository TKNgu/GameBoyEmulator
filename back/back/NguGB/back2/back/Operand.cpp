#include "Operand.hpp"

#include <iomanip>
#include <iostream>

using namespace std;

void Operand::show() {
  cout << "Operand value" << endl;
  cout << "uint8  0x" << setfill('0') << setw(2) << hex << (unsigned)(read8())
       << endl;
  cout << "uint16 0x" << setfill('0') << setw(4) << hex << (unsigned)(read16())
       << endl;
  cout << "uint8L 0x" << setfill('0') << setw(2) << hex << (unsigned)(read8L())
       << endl;
  cout << "uint8H 0x" << setfill('0') << setw(2) << hex << (unsigned)(read8H())
       << endl;
}

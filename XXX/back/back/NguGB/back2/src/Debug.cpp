#include "Debug.hpp"

#include <bitset>
#include <iomanip>
#include <sstream>

using namespace std;

string ShowHex(uint8_t data) {
  stringstream show;
  show << "0x" << setfill('0') << setw(2) << hex << (unsigned)(data);
  return show.str();
}

string ShowHex(uint16_t data) {
  stringstream show;
  show << "0x" << setfill('0') << setw(4) << hex << (unsigned)(data);
  return show.str();
}

string ShowBin(uint8_t data) {
  stringstream show;
  show << "0b" << bitset<8>(data);
  return show.str();
}

string ShowBin(uint16_t data) {
  stringstream show;
  show << "0b" << bitset<16>(data);
  return show.str();
}

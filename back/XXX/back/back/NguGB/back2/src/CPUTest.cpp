#include <cassert>
#include <cstdlib>
#include <ctime>
#include <iostream>

#include "CPU.hpp"
#include "Debug.hpp"
#include "Operand.hpp"

using namespace std;

#define RAND_PC this->registers.PC = rand() % 0xff00
#define RAND_16 rand() % 0xffff
#define RAND_8 rand() % 0xff
#define RAM_n16(value)                                                         \
  this->ram[this->registers.PC + 1] = value & 0x00ff;                          \
  this->ram[this->registers.PC + 2] = (value >> 8) & 0x00ff
#define RAM_n8(value) this->ram[this->registers.PC + 1] = value & 0x00ff
#define RUN(OPCODE) this->opcodes[OPCODE]()

#define SHOW_RAM8(address) cout << "Ram " << ShowHex(ram[address]) << endl
#define SHOW_RAM16(address)                                                    \
  cout << "Ram " << ShowHex(ram[address]) << " " << ShowHex(ram[address + 1])  \
       << endl

void CPU::test() {
  srand(time(nullptr));

  {
    auto value = RAND_PC;
    auto operand = Operand(false, &this->registers.PC);
    assert(operand.readHalf() == (value & 0x0f));
  }

  {
    auto value = this->registers.A = RAND_8;
    auto operand = Operand(true, &this->registers.A);
    assert(operand.readHalf() == (value & 0x0f));
  }

  {
    auto PC = RAND_PC;
    RUN(0x00);
    assert(this->registers.PC == (PC + 1));
  }

  {
    RAND_PC;
    auto BC = RAND_16;
    RAM_n16(BC);
    RUN(0x01);
    assert(this->registers.BC == BC);
  }

  {
    RAND_PC;
    auto value = this->registers.A = RAND_8;
    this->registers.BC = RAND_16;
    RUN(0x02);
    assert(this->ram[this->registers.BC] == value);
  }

  {
    auto value = this->registers.BC = RAND_16;
    RUN(0x03);
    assert(this->registers.BC == ++value);
  }

  {
    this->registers.B = 0x00;
    RUN(0x04);
    assert(this->registers.B == 0x01);
    assert((ZFLAG) == 0x00);
    assert((NFLAG) == 0x00);
    assert((HFLAG) == 0x00);
  }

  {
    this->registers.B = 0x0f;
    RUN(0x04);
    assert(this->registers.B == 0x10);
    assert((ZFLAG) == 0x00);
    assert((NFLAG) == 0x00);
    assert((HFLAG) != 0x00);
  }

  {
    this->registers.B = 0xff;
    RUN(0x04);
    assert(this->registers.B == 0x00);
    assert((ZFLAG) != 0x00);
    assert((NFLAG) == 0x00);
    assert((HFLAG) != 0x00);
  }

  {
    this->registers.B = 0x02;
    RUN(0x05);
    assert(this->registers.B == 0x01);
    assert((ZFLAG) == 0x00);
    assert((NFLAG) != 0x00);
    assert((HFLAG) == 0x00);
  }

  {
    this->registers.B = 0x01;
    RUN(0x05);
    assert(this->registers.B == 0x00);
    assert((ZFLAG) != 0x00);
    assert((NFLAG) != 0x00);
    assert((HFLAG) == 0x00);
  }

  {
    this->registers.B = 0x10;
    RUN(0x05);
    assert(this->registers.B == 0x0f);
    assert((ZFLAG) == 0x00);
    assert((NFLAG) != 0x00);
    assert((HFLAG) != 0x00);
  }

  {
    this->registers.B = 0x00;
    RUN(0x05);
    assert(this->registers.B == 0xff);
    assert((ZFLAG) == 0x00);
    assert((NFLAG) != 0x00);
    assert((HFLAG) != 0x00);
  }

  {
    RAND_PC;
    auto value = RAND_8;
    RAM_n8(value);
    RUN(0x06);
    assert(this->registers.B == value);
  }

  {
    this->registers.A = 0b01001001;
    this->registers.F = 0b11110000;
    RUN(0x07);
    assert(this->registers.F == 0b00000000);
    assert(this->registers.A == 0b10010010);
  }

  {
    this->registers.A = 0b10001100;
    this->registers.F = 0b11100000;
    RUN(0x07);
    assert(this->registers.A == 0b00011001);
    assert(this->registers.F == 0b00010000);
  }

  {
    auto value = RAND_16;
    this->registers.SP = value;
    RAND_PC;
    auto address = RAND_16;
    this->ram[this->registers.PC + 1] = address & 0x00ff;
    this->ram[this->registers.PC + 2] = (address >> 8) & 0x00ff;
    RUN(0x08);
    assert(this->ram[address] == (value & 0x00ff));
    assert(this->ram[address + 1] == ((value >> 8) & 0x00ff));
  }

  {
    this->registers.HL = 0x0005;
    this->registers.BC = 0x0002;
    RUN(0x09);
    assert(this->registers.HL == 0x0007);
    assert((NFLAG) == 0x00);
    assert((CFLAG) == 0x00);
    assert((HFLAG) == 0x00);
  }

  {
    this->registers.HL = 0x0007;
    this->registers.BC = 0x000A;
    RUN(0x09);
    assert(this->registers.HL == 0x0011);
    assert((NFLAG) == 0x00);
    assert((CFLAG) == 0x00);
    assert((HFLAG) != 0x00);
  }

  {
    this->registers.HL = 0xff00;
    this->registers.BC = 0x0100;
    RUN(0x09);
    assert(this->registers.HL == 0x0000);
    assert((NFLAG) == 0x00);
    assert((CFLAG) != 0x00);
    assert((HFLAG) == 0x00);
  }

  {
    this->registers.HL = 0xff0f;
    this->registers.BC = 0x0105;
    RUN(0x09);
    assert(this->registers.HL == 0x0014);
    assert((NFLAG) == 0x00);
    assert((CFLAG) != 0x00);
    assert((HFLAG) != 0x00);
  }

  {
    this->registers.BC = RAND_16;
    auto value = RAND_8;
    this->ram[this->registers.BC] = value & 0x00ff;
    RUN(0x0a);
    assert(this->registers.A == value);
  }

  {
    auto value = this->registers.BC = RAND_16;
    RUN(0x0b);
    assert(this->registers.BC == value - 1);
  }

  {
    this->registers.C = 0x00;
    RUN(0xc);
    assert(this->registers.C == 0x01);
    assert((ZFLAG) == 0x00);
    assert((NFLAG) == 0x00);
    assert((HFLAG) == 0x00);
  }

  {
    this->registers.C = 0xff;
    RUN(0xc);
    assert(this->registers.C == 0x00);
    assert((ZFLAG) != 0x00);
    assert((NFLAG) == 0x00);
    assert((HFLAG) != 0x00);
  }

  {
    this->registers.C = 0x0f;
    RUN(0xc);
    assert(this->registers.C == 0x10);
    assert((ZFLAG) == 0x00);
    assert((NFLAG) == 0x00);
    assert((HFLAG) != 0x00);
  }

  {
    this->registers.C = 0x02;
    RUN(0xd);
    assert(this->registers.C == 0x01);
    assert((ZFLAG) == 0x00);
    assert((NFLAG) != 0x01);
    assert((HFLAG) == 0x00);
  }

  {
    this->registers.C = 0x01;
    RUN(0xd);
    assert(this->registers.C == 0x00);
    assert((ZFLAG) != 0x00);
    assert((NFLAG) != 0x01);
    assert((HFLAG) == 0x00);
  }

  {
    this->registers.C = 0x10;
    RUN(0xd);
    assert(this->registers.C == 0x0f);
    assert((ZFLAG) == 0x00);
    assert((NFLAG) != 0x01);
    assert((HFLAG) != 0x00);
  }

  {
    this->registers.C = 0x00;
    RUN(0xd);
    assert(this->registers.C == 0xff);
    assert((ZFLAG) == 0x00);
    assert((NFLAG) != 0x01);
    assert((HFLAG) != 0x00);
  }

  {
    RAND_PC;
    auto value = this->ram[this->registers.PC + 1] = RAND_8;
    RUN(0x0e);
    assert(this->registers.C == value);
  }

  {
    this->registers.A = 0b00000001;
    RUN(0x0f);
    assert(this->registers.A == 0b10000000);
    assert((CFLAG) != 0x00);
  }

  {
    this->registers.A = 0b00000010;
    RUN(0x0f);
    assert(this->registers.A == 0b00000001);
    assert((CFLAG) == 0x00);
  }

  {
    auto value = RAND_PC;
    RUN(0x10);
    assert(this->registers.PC == value + 2);
  }

  {
    RAND_PC;
    auto value = RAND_16;
    this->ram[this->registers.PC + 1] = value & 0x00ff;
    this->ram[this->registers.PC + 2] = (value >> 8) & 0x00ff;
    RUN(0x11);
    assert(this->registers.DE == value);
  }

  {
    RAND_PC;
    this->registers.DE = RAND_16;
    auto value = this->registers.A = RAND_8;
    RUN(0x12);
    assert(this->ram[this->registers.DE] == value);
  }

  {
    auto value = this->registers.DE = RAND_16;
    RUN(0x13);
    assert(this->registers.DE == value + 1);
  }

  {
    this->registers.D = 0x02;
    RUN(0x15);
    assert(this->registers.D == 0x01);
    assert((ZFLAG) == 0x00);
    assert((NFLAG) != 0x00);
    assert((HFLAG) == 0x00);
  }

  {
    this->registers.D = 0x01;
    RUN(0x15);
    assert(this->registers.D == 0x00);
    assert((ZFLAG) != 0x00);
    assert((NFLAG) != 0x00);
    assert((HFLAG) == 0x00);
  }

  {
    this->registers.D = 0x10;
    RUN(0x15);
    assert(this->registers.D == 0x0f);
    assert((ZFLAG) == 0x00);
    assert((NFLAG) != 0x00);
    assert((HFLAG) != 0x00);
  }

  {
    this->registers.D = 0x00;
    RUN(0x15);
    assert(this->registers.D == 0xff);
    assert((ZFLAG) == 0x00);
    assert((NFLAG) != 0x00);
    assert((HFLAG) != 0x00);
  }

  {
    this->registers.A = 0b00001000;
    CFLAGOF;
    RUN(0x17);
    assert((CFLAG) == 0x00);
    assert(this->registers.A == 0b00010000);
  }

  {
    this->registers.A = 0b10001000;
    CFLAGOF;
    RUN(0x17);
    assert((CFLAG) != 0x00);
    assert(this->registers.A == 0b00010000);
  }

  {
    this->registers.A = 0b10001000;
    CFLAGON;
    RUN(0x17);
    assert((CFLAG) != 0x00);
    assert(this->registers.A == 0b00010001);
  }

  {
    this->registers.A = 0b00001000;
    CFLAGON;
    RUN(0x17);
    assert((CFLAG) == 0x00);
    assert(this->registers.A == 0b00010001);
  }

  {
    auto pc = RAND_PC;
    uint8_t value = RAND_8 & 0b01111111;
    this->ram[this->registers.PC + 1] = value;
    RUN(0x18);
    assert(this->registers.PC == (pc + (int8_t)(value)));
  }

  {
    auto pc = RAND_PC;
    uint8_t value = RAND_8 | 0b10000000;
    this->ram[this->registers.PC + 1] = value;
    RUN(0x18);
    assert(this->registers.PC == (pc + (int8_t)(value)));
  }

  {
    this->registers.A = 0b00000010;
    CFLAGOF;
    RUN(0x1f);
    assert(this->registers.A == 0b00000001);
    assert((CFLAG) == 0x00);
  }

  {
    this->registers.A = 0b00000010;
    CFLAGON;
    RUN(0x1f);
    assert(this->registers.A == 0b10000001);
    assert((CFLAG) == 0x00);
  }

  {
    this->registers.A = 0b00000011;
    CFLAGON;
    RUN(0x1f);
    assert(this->registers.A == 0b10000001);
    assert((CFLAG) != 0x00);
  }

  {
    this->registers.A = 0b00000011;
    CFLAGOF;
    RUN(0x1f);
    assert(this->registers.A == 0b00000001);
    assert((CFLAG) != 0x00);
  }

  {
    this->registers.A = 0x00;
    CFLAGOF;
    HFLAGOF;
    RUN(0x27);
    assert(this->registers.A == 0x00);
    assert((ZFLAG) != 0x00);
    assert((HFLAG) == 0x00);
    assert((CFLAG) == 0x00);
  }

  {
    this->registers.A = 0x03;
    CFLAGOF;
    HFLAGOF;
    RUN(0x27);
    assert(this->registers.A == 0x03);
    assert((ZFLAG) == 0x00);
    assert((HFLAG) == 0x00);
    assert((CFLAG) == 0x00);
  }

  {
    this->registers.A = 0x0A;
    CFLAGOF;
    HFLAGOF;
    RUN(0x27);
    assert(this->registers.A == 0x10);
    assert((ZFLAG) == 0x00);
    assert((HFLAG) == 0x00);
    assert((CFLAG) == 0x00);
  }

  {
    this->registers.A = 0xAA;
    CFLAGOF;
    HFLAGOF;
    RUN(0x27);
    assert(this->registers.A == 0x10);
    assert((ZFLAG) == 0x00);
    assert((HFLAG) == 0x00);
    assert((CFLAG) != 0x00);
  }

  {
    this->registers.A = 0x00;
    CFLAGON;
    HFLAGON;
    RUN(0x27);
    assert(this->registers.A == 0x66);
    assert((ZFLAG) == 0x00);
    assert((HFLAG) == 0x00);
    assert((CFLAG) != 0x00);
  }

  {
    this->registers.A = 0x10;
    CFLAGOF;
    HFLAGON;
    RUN(0x27);
    assert(this->registers.A == 0x16);
    assert((ZFLAG) == 0x00);
    assert((HFLAG) == 0x00);
    assert((CFLAG) == 0x00);
  }

  {
    this->registers.A = 0x00;
    this->registers.B = 0x00;
    CFLAGOF;
    RUN(0x88);
    assert(this->registers.A == 0x00);
    assert((CFLAG) == 0x00);
    assert((NFLAG) == 0x00);
    assert((HFLAG) == 0x00);
    assert((ZFLAG) != 0x00);
  }

  {
    this->registers.A = 0x0f;
    this->registers.B = 0x01;
    CFLAGOF;
    RUN(0x88);
    assert(this->registers.A == 0x10);
    assert((CFLAG) == 0x00);
    assert((NFLAG) == 0x00);
    assert((HFLAG) != 0x00);
    assert((ZFLAG) == 0x00);
  }

  {
    this->registers.A = 0xff;
    this->registers.B = 0x01;
    CFLAGOF;
    RUN(0x88);
    assert(this->registers.A == 0x00);
    assert((CFLAG) != 0x00);
    assert((NFLAG) == 0x00);
    assert((HFLAG) != 0x00);
    assert((ZFLAG) != 0x00);
  }

  {
    this->registers.A = 0xff;
    this->registers.B = 0x01;
    CFLAGON;
    RUN(0x88);
    assert(this->registers.A == 0x01);
    assert((CFLAG) != 0x00);
    assert((NFLAG) == 0x00);
    assert((HFLAG) != 0x00);
    assert((ZFLAG) == 0x00);
  }

  {
    this->registers.A = 0xf9;
    this->registers.B = 0x22;
    CFLAGON;
    RUN(0x88);
    assert(this->registers.A == 0x1C);
    assert((CFLAG) != 0x00);
    assert((NFLAG) == 0x00);
    assert((HFLAG) == 0x00);
    assert((ZFLAG) == 0x00);
  }

  {
    this->registers.A = 0x00;
    this->registers.B = 0x00;
    RUN(0x90);
    assert(this->registers.A == 0x00);
    assert((ZFLAG) != 0x00);
    assert((NFLAG) != 0x00);
    assert((HFLAG) == 0x00);
    assert((CFLAG) == 0x00);
  }

  {
    this->registers.A = 0xf0;
    this->registers.B = 0x01;
    RUN(0x90);
    assert(this->registers.A == 0xef);
    assert((ZFLAG) == 0x00);
    assert((NFLAG) != 0x00);
    assert((HFLAG) != 0x00);
    assert((CFLAG) == 0x00);
  }

  {
    this->registers.A = 0x04;
    this->registers.B = 0x11;
    RUN(0x90);
    assert(this->registers.A == 0xf3);
    assert((ZFLAG) == 0x00);
    assert((NFLAG) != 0x00);
    assert((HFLAG) == 0x00);
    assert((CFLAG) != 0x00);
  }

  {
    this->registers.A = 0x01;
    this->registers.B = 0x00;
    CFLAGON;
    RUN(0x98);
    assert(this->registers.A == 0x00);
    assert((ZFLAG) != 0x00);
    assert((NFLAG) != 0x00);
    assert((HFLAG) == 0x00);
    assert((CFLAG) == 0x00);
  }

  {
    this->registers.A = 0xf0;
    this->registers.B = 0x01;
    CFLAGON;
    RUN(0x98);
    assert(this->registers.A == 0xee);
    assert((ZFLAG) == 0x00);
    assert((NFLAG) != 0x00);
    assert((HFLAG) != 0x00);
    assert((CFLAG) == 0x00);
  }

  {
    this->registers.A = 0x04;
    this->registers.B = 0x11;
    CFLAGON;
    RUN(0x98);
    assert(this->registers.A == 0xf2);
    assert((ZFLAG) == 0x00);
    assert((NFLAG) != 0x00);
    assert((HFLAG) == 0x00);
    assert((CFLAG) != 0x00);
  }

  {
    auto valueA = this->registers.A = RAND_8;
    auto valueB = this->registers.B = RAND_8;
    RUN(0xa0);
    assert(this->registers.A == (valueA & valueB));
    assert((ZFLAG) != (valueA & valueB));
    assert((NFLAG) == 0x00);
    assert((HFLAG) != 0x00);
    assert((CFLAG) == 0x00);
  }

  {
    auto valueA = this->registers.A = RAND_8;
    auto valueB = this->registers.B = RAND_8;
    RUN(0xa8);
    assert(this->registers.A == (valueA ^ valueB));
    assert((ZFLAG) != (valueA ^ valueB));
    assert((NFLAG) == 0x00);
    assert((HFLAG) == 0x00);
    assert((CFLAG) == 0x00);
  }

  {
    auto valueA = this->registers.A = RAND_8;
    auto valueB = this->registers.B = RAND_8;
    RUN(0xb0);
    assert(this->registers.A == (valueA | valueB));
    assert((ZFLAG) != (valueA | valueB));
    assert((NFLAG) == 0x00);
    assert((HFLAG) == 0x00);
    assert((CFLAG) == 0x00);
  }

  {
    auto valueA = this->registers.A = RAND_8;
    auto valueB = this->registers.B = RAND_8;
    RUN(0xb8);
    uint8_t tmp = valueA - valueB;
    assert(this->registers.A == valueA);
    assert((ZFLAG) != tmp);
    assert((NFLAG) != 0x00);
    assert((CFLAG) == ((valueA < valueB) ? 0b00010000 : 0x00000000));
    assert((HFLAG) ==
           (((valueA & 0x0f) < (valueB & 0x0f)) ? 0b00100000 : 0b00000000));
  }

  {
    this->registers.F = 0b00000000;
    auto address = this->registers.PC = RAND_16;
    RUN(0xc0);
    assert(this->registers.PC == (address + 1));
  }

  {
    this->registers.F = 0b10000000;
    auto address = this->registers.SP = RAND_16;
    auto low = this->ram[this->registers.SP] = RAND_8;
    auto high = this->ram[this->registers.SP + 1] = RAND_8;
    RUN(0xc0);
    assert(this->registers.PC == (low + (high << 8)));
    assert(this->registers.SP == (address + 2));
  }

  {
    auto address = this->registers.SP = RAND_16;
    auto low = this->ram[this->registers.SP] = RAND_8;
    auto high = this->ram[this->registers.SP + 1] = RAND_8;
    RUN(0xc1);
    assert(this->registers.BC == (low + (high << 8)));
    assert(this->registers.SP == (address + 2));
  }

  {
    this->registers.F = 0b00000000;
    auto address = RAND_PC;
    RUN(0xc2);
    assert(this->registers.PC == (address + 3));
  }

  {
    this->registers.F = 0b01000000;
    auto address = this->registers.PC = 0x04ff;
    auto low = this->ram[this->registers.PC + 1] = 0xab;
    auto high = this->ram[this->registers.PC + 2] = 0x34;
    RUN(0xc2);
    assert(this->registers.PC == (low + (high << 8)));
  }

  {
    this->registers.F = 0b00000000;
    auto address = RAND_PC;
    RUN(0xc4);
    assert(this->registers.PC == (address + 3));
  }

  {
    this->registers.F = 0b10000000;
    this->registers.SP = RAND_16;
    auto address = RAND_PC;
    auto low = this->ram[this->registers.PC + 1] = RAND_8;
    auto high = this->ram[this->registers.PC + 2] = RAND_8;
    RUN(0xc4);
    auto stack = this->registers.SP;
    assert(this->registers.PC == (low + (high << 8)));
    assert(address == (this->ram[stack] + (this->ram[stack + 1] << 8)));
  }

  {
    this->registers.SP = RAND_16;
    this->registers.BC = RAND_16;
    RUN(0xC5);
    assert(this->registers.BC == (this->ram[this->registers.SP] +
                                  (this->ram[this->registers.SP + 1] << 8)));
  }

  {
    this->registers.SP = RAND_16;
    auto address = RAND_PC;
    RUN(0xc7);
    assert(this->registers.PC == 0x00);
    assert(address == (this->ram[this->registers.SP] +
                       (this->ram[this->registers.SP + 1] << 8)));
  }

  {
    auto value = this->registers.A = RAND_8;
    RAND_PC;
    auto address = this->ram[this->registers.PC + 1] = RAND_8;
    RUN(0xe0);
    assert(value == this->ram[0xff00 + address]);
  }

  {
    RAND_PC;
    auto address = this->ram[this->registers.PC + 1] = RAND_8;
    auto value = this->ram[0xff00 + address] = RAND_8;
    RUN(0xf0);
    assert(this->registers.A == value);
  }

  {
    RAND_PC;
    this->ram[this->registers.PC] = 0xcb;
    this->ram[this->registers.PC + 1] = 0x00;
    this->registers.B = 0b00110001;
    RUN(0xcb);
    assert(this->registers.B == 0b01100010);
    assert((ZFLAG) == 0x00);
    assert((CFLAG) == 0x00);
  }

  {
    RAND_PC;
    this->ram[this->registers.PC] = 0xcb;
    this->ram[this->registers.PC + 1] = 0x00;
    this->registers.B = 0x00;
    RUN(0xcb);
    assert(this->registers.B == 0x00);
    assert((ZFLAG) != 0x00);
    assert((CFLAG) == 0x00);
  }

  {
    RAND_PC;
    this->ram[this->registers.PC] = 0xcb;
    this->ram[this->registers.PC + 1] = 0x00;
    this->registers.B = 0b10001110;
    RUN(0xcb);
    assert(this->registers.B == 0b00011101);
    assert((ZFLAG) == 0x00);
    assert((CFLAG) != 0x00);
  }

  {
    RAND_PC;
    this->ram[this->registers.PC] = 0xcb;
    this->ram[this->registers.PC + 1] = 0x08;
    this->registers.B = 0b00110001;
    RUN(0xcb);
    assert(this->registers.B == 0b10011000);
    assert((ZFLAG) == 0x00);
    assert((CFLAG) != 0x00);
  }

  {
    RAND_PC;
    this->ram[this->registers.PC] = 0xcb;
    this->ram[this->registers.PC + 1] = 0x08;
    this->registers.B = 0x00;
    RUN(0xcb);
    assert(this->registers.B == 0x00);
    assert((ZFLAG) != 0x00);
    assert((CFLAG) == 0x00);
  }

  {
    RAND_PC;
    this->ram[this->registers.PC] = 0xcb;
    this->ram[this->registers.PC + 1] = 0x08;
    this->registers.B = 0b10001110;
    RUN(0xcb);
    assert(this->registers.B == 0b01000111);
    assert((ZFLAG) == 0x00);
    assert((CFLAG) == 0x00);
  }
}

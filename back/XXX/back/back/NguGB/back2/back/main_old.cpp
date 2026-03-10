#include <SFML/Window.hpp>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include "CPU.hpp"
#include "Memory.hpp"

using namespace std;

int main_old() {
  cout << "NguGB" << endl;
  // Memory memory;
  // if (memory.readCartridge("data/cpu_instrs.gb")) {
  //   cout << "Error read rom file" << endl;
  //   return 1;
  // }
  try {
    CPU cpu("data/Opcodes.json");
  } catch (runtime_error &e) {
    cout << "Error " << e.what() << endl;
  }
  // char cmd;
  // while (true) {
  //   cpu.show();
  //   cpu.tick();
  //   cin >> cmd;
  // }
  // return 0;
  //
  // sf::Window window(sf::VideoMode(160, 144), "NguGB");
  // while (window.isOpen()) {
  //   sf::Event event;
  //   while (window.pollEvent(event)) {
  //     if (event.type == sf::Event::Closed) {
  //       window.close();
  //     }
  //   }
  // }
  // return 0;
}

#include "LCD.hpp"

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/Window/Event.hpp>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "Debug.hpp"

using namespace std;

LCD::LCD(uint8_t *ram)
    : window(sf::RenderWindow(sf::VideoMode(160, 144), "NguGB")), ram(ram) {}

void LCD::bootUp() {
  // for (auto indexTiles = 0u; indexTiles < 384; indexTiles++) {
  //   for (auto index = 0u; index < 16; index++) {
  //     cout << ShowHex(this->ram[0x8000 + index + indexTiles * 16]) << " ";
  //   }
  //   cout << endl;
  // }
}

void LCD::test() {
  sf::Texture texture;
  texture.create(10, 10);
  sf::Uint8 *pixels = new sf::Uint8[10 * 10 * 4];
  memset(pixels, 0xff, 10 * 10 * 4);
  texture.update(pixels);
  sf::Sprite sprite(texture);

  while (true) {
    sf::Event event;
    while (this->window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) {
        return;
      }
    }
    this->window.draw(sprite);
    this->window.display();
  }
}

void LCD::render() {}

void LCD::tick() {}

void LCD::input() {
  sf::Event event;
  while (this->window.pollEvent(event)) {
    if (event.type == sf::Event::Closed)
      this->window.close();
  }
  sf::sleep(sf::seconds(1));
}

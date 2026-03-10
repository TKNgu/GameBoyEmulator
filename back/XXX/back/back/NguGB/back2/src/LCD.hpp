#ifndef LCD_hpp
#define LCD_hpp

#include <SFML/Graphics/RenderWindow.hpp>

class LCD {
public:
  LCD(uint8_t *);
  void bootUp();
  void render();
  void tick();
  void input();
  void test();

private:
  sf::RenderWindow window;
  uint8_t *ram;
};

#endif

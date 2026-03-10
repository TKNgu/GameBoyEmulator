#ifndef Opcode_hpp
#define Opcode_hpp

#include <functional>

class Opcode {
public:
  virtual ~Opcode() {}
  virtual void show() = 0;
  virtual void run() = 0;
};

class SimpleOpcode : public Opcode {
public:
  SimpleOpcode(std::function<void()>);
  void show() override;
  void run() override;

private:
  std::function<void()> opcode;
};

#endif

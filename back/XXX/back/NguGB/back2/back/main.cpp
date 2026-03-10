#include <fstream>
#include <iostream>

#include "json.hpp"

using namespace std;

int main(int, char *[]) {
  cout << "Build CPU" << endl;
  ifstream fileOpcodes("data/Opcodes.json");
  auto opcodes = nlohmann::json::parse(fileOpcodes);
  return 0;
}

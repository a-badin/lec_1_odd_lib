#include <iostream>
#include "odd.hpp"

int parse_number(int argc, char** argv)
{
  if (argc < 2)
    throw std::invalid_argument("");
  return std::stoi(argv[1]);
}

int main(int argc, char** argv) {
  int i = 0;
  std::string ss;
  try
  {
    i = parse_number(argc, argv);
  } catch (std::invalid_argument& e) {
    std::cerr << "First argument must be number" << std::endl;
    return 1;
  }

  std::cout << std::boolalpha << odd::is_odd(i) << std::endl;

  return 0;
}

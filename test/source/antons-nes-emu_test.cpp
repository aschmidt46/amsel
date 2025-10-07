#include "lib.hpp"

auto main() -> int
{
  auto const lib = library {};

  return lib.name == "antons-nes-emu" ? 0 : 1;
}

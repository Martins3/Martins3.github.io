#include <bit>
#include <bitset>
#include <cstdint>
#include <iostream>

int main() {
  // @todo 需要修改下 ccls 的默认 flags
  for (const std::uint8_t i : {0, 0b11111111, 0b00011101}) {
    std::cout << "popcount( " << std::bitset<8>(i)
              << " ) = " << std::popcount(i) << '\n';
  }
}

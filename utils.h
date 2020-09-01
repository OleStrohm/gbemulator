#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace util {

std::vector<uint8_t> readFile(const std::string_view &filename) {
  std::ifstream fs(filename.data(), std::ios::binary);

  std::vector<uint8_t> res(std::istreambuf_iterator<char>(fs), {});

  return res;
}

void printfBits(std::string msg, int n, int bits, bool newline = true) {
  printf("%s", msg.c_str());
  for (int b = bits - 1; b >= 0; b--) {
    if (b != (bits - 1) && b % 4 == 3)
      printf(" ");
    printf("%d", (n & (1 << b)) ? 1 : 0);
  }
  if (newline)
    printf("\n");
}

void hexdump(std::vector<uint8_t> hex, size_t length) {
  std::string charview;
  bool wasCopy = false;

  int c = 0;
  for (; c < hex.size() && c < length;) {
    if (c % 16 == 0) {
      bool isCopy = false;
      if (c >= 16 && hex.size() - c >= 16) {
        isCopy = true;
        for (int col = 0; col < 16; col++)
          if (hex[c + col] != hex[c + col - 16])
            isCopy = false;
        if (isCopy) {
          bool isAdditionalCopy = false;
          if (c >= 32 && hex.size() - c >= 16) {
            isAdditionalCopy = true;
            for (int col = 0; col < 16; col++)
              if (hex[c + col] != hex[c + col - 32])
                isAdditionalCopy = false;
          }

          if (!isAdditionalCopy)
            printf("*\n");

		  wasCopy = true;
          c += 16;
          continue;
        }
      }
    }
	wasCopy = false;

    int b = hex[c];
    if (c % 16 == 0)
      printf("%08x: ", c);
    printf("%02x ", b);
    if (std::isprint(b))
      charview += b;
    else
      charview += '.';

    c++;
    if (c % 8 == 0)
      std::cout << " ";
    if (c % 16 == 0) {
      std::cout << "|" << charview << "|" << std::endl;
      charview = "";
    }
  }
  if (c % 16 != 0) {
    while (c % 16 != 0) {
      std::cout << "   ";
      c++;
      if (c % 8 == 0)
        std::cout << " ";
      if (c % 16 == 0)
        std::cout << "|" << charview << "|" << std::endl;
    }
  }
  if (wasCopy)
	  printf("%08x:\n", c);
}

}

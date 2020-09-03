#pragma once

#include <string>
#include <vector>

namespace util {

std::vector<uint8_t> readFile(const std::string_view &filename);

void printfBits(std::string msg, int n, int bits, bool newline = true);
void hexdump(std::vector<uint8_t> hex);
void hexdump(std::vector<uint8_t> hex, size_t length);
void hexdump(std::vector<uint8_t> hex, size_t length, size_t base);

} // namespace util

#pragma once

#include <cstdint>
#include <vector>

#include "ppu.h"

class Bus {
  std::vector<uint8_t> cartridge;
  std::vector<uint8_t> ram;

  PPU *ppu;

public:
  Bus(PPU *ppu);

  void loadCartridge(std::vector<uint8_t> boot);

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t value);
};

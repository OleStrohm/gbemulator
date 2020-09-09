#pragma once

#include <cstdint>
#include <vector>

class CPU;
class PPU;

class Bus {
  std::vector<uint8_t> cartridge;
  std::vector<uint8_t> ram;
  std::vector<uint8_t> ramBank;

  uint64_t cartridgeBankAddress = 0x4000;
  uint64_t ramBankAddress = 0x0000;

  PPU *ppu;
  CPU *cpu;

public:
  void connectCPU(CPU *cpu);
  void connectPPU(PPU *ppu);

  void loadCartridge(std::vector<uint8_t> boot);

  void raiseInterrupt(int interrupt);

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t value);
};

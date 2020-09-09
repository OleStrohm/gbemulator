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

  uint16_t DMAAddress = 0;
  bool inDMATransfer = false;

  uint8_t read_internal(uint16_t addr);
  void write_internal(uint16_t addr, uint8_t value);

public:
  void connectCPU(CPU *cpu);
  void connectPPU(PPU *ppu);

  void loadCartridge(std::vector<uint8_t> boot);

  void raiseInterrupt(int interrupt);

  void syncronize();
  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t value);
};

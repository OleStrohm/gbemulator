#pragma once

#include "register.h"

#include <cstdio>
#include <memory>
#include <vector>

class Instruction;

class CPU {
  std::vector<uint8_t> rom;
  std::vector<uint8_t> ram0;
  std::vector<uint8_t> vram;
  std::vector<uint8_t> zeropage;

  RegisterBank registers;
  std::unique_ptr<Instruction> instr;

  bool halted = false;

public:
  CPU() : rom(0x8000), ram0(0x1000), vram(0x2000), zeropage(0xFFFE - 0xFF80) {
    registers.pc = 0;
  }

  bool step();

  bool hasHalted() { return halted; }
  RegisterBank &getRegisters() { return registers; }

  uint8_t read(uint16_t addr) {
    if (addr < 0x8000)
      return rom[addr];
    if (addr >= 0x8000 && addr < 0xA000)
      return vram[addr - 0x8000];
    if (addr >= 0xC000 && addr < 0xD000)
      return ram0[addr - 0xC000];
    if (addr >= 0xFF80 && addr <= 0xFFFE)
      return zeropage[addr - 0xFF80];

    printf("ERROR: READ MEMORY OUT OF BOUNDS : %04X\n", addr);
    return 0xFF;
  }

  void write(uint16_t addr, uint8_t value) {
    if (addr < 0x8000)
      rom[addr] = value;
    else if (addr >= 0x8000 && addr < 0xA000)
      vram[addr - 0x8000] = value;
    else if (addr >= 0xC000 && addr < 0xD000)
      ram0[addr - 0xC000] = value;
    else if (addr >= 0xFF00 && addr < 0xFF4C) {
      if (addr == 0xFF26) {
        if (value >> 7)
          printf("Enabled Audio\n");
        else
          printf("Disabled Audio\n");
      }
    } else if (addr >= 0xFF80 && addr <= 0xFFFE)
      zeropage[addr - 0xFF80] = value;
    else
      printf("ERROR: WRITE MEMORY OUT OF BOUNDS\n");
  }

  void loadRom(std::vector<uint8_t> rom) {
    auto biosSize = rom.size();
    this->rom = rom;
    this->rom.resize(biosSize);
  }

  void loadBoot(std::vector<uint8_t> boot) {
    for (int i = 0; i < boot.size(); i++) {
      this->rom[i] = boot[i];
    }
  }

  void dumpRom();
  void dumpRegisters();
};

#pragma once

#include "register.h"

#include <cstdio>
#include <memory>
#include <vector>

class Instruction;

class CPU {
  std::vector<uint8_t> rom;
  std::vector<uint8_t> switchableRam;
  std::vector<uint8_t> ram;
  std::vector<uint8_t> vram;
  std::vector<uint8_t> oam;
  std::vector<uint8_t> zeropage;

  RegisterBank registers;
  std::unique_ptr<Instruction> instr;

  bool interruptsEnabled = false;
  bool halted = false;
  bool breakpoint = false;

public:
  CPU();

  bool step();

  void setInterruptEnable(bool enableInterrupts) {
    interruptsEnabled = enableInterrupts;
  }

  bool hasHalted() { return halted; }
  RegisterBank &getRegisters() { return registers; }

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t value);

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
  void dumpVRam();
  void dumpRam();
  void dumpRegisters();
};

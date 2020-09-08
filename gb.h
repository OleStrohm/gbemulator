#pragma once

#include "bus.h"
#include "register.h"

#include <cstdio>
#include <memory>
#include <vector>

class Instruction;

class CPU {
  std::vector<uint8_t> boot;
  std::vector<uint8_t> ram;
  std::vector<uint8_t> zeropage;

  Bus *bus;

  RegisterBank registers;
  std::unique_ptr<Instruction> instr;
  uint64_t clockCycle;

  uint16_t DIV;
  uint8_t TIMA;
  uint8_t TMA;
  uint8_t TAC;
  uint8_t IF;
  uint8_t IE;

  bool interruptsEnabled = false;
  bool interruptsShouldBeEnabled = false;
  int8_t interruptChangeStateDelay = -1;

  bool halted = false;
  bool hasRecoveredFromHalt = true;

  bool unlockedBootRom = false;

  bool breakpoint = false;

public:
  CPU(Bus *bus);

  bool step();

  void setInterruptEnable(bool enableInterrupts) {
    interruptsShouldBeEnabled = enableInterrupts;
    interruptChangeStateDelay = 2;
  }

  void setHalted() {
    halted = true;
    hasRecoveredFromHalt = true;
  }
  bool hasHalted() { return halted; }
  RegisterBank &getRegisters() { return registers; }

  void raiseInterrupt(int interrupt);

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t value);

  void loadBoot(std::vector<uint8_t> boot) { this->boot = boot; }

  void dumpBoot();
  void dumpRam();
  void dumpRegisters();
};

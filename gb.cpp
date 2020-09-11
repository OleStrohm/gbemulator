#include "gb.h"

#include "ppu.h"

#include "instructions.h"
#include "utils.h"
#include <chrono>
#include <iostream>
#include <ratio>
#include <thread>

constexpr uint8_t interruptVblank = 1 << 0;
constexpr uint16_t interruptVblankAddress = 0x0040;
constexpr uint8_t interruptLCDC = 1 << 1;
constexpr uint16_t interruptLCDCAddress = 0x0048;
constexpr uint8_t interruptTimer = 1 << 2;
constexpr uint16_t interruptTimerAddress = 0x0050;
constexpr uint8_t interruptSerialComplete = 1 << 3;
constexpr uint16_t interruptSerialCompleteAddress = 0x0058;
constexpr uint8_t interruptInput = 1 << 4;
constexpr uint16_t interruptInputAddress = 0x0060;

constexpr uint32_t timerLUT[] = {7, 1, 3, 5}; // In m cycles

constexpr bool logRegisters = false;
constexpr bool skipBootScreen = false;

CPU::CPU(Bus *bus)
    : boot(0x100), ram(0x2000), zeropage(0xFFFE - 0xFF80), bus(bus) {
  registers.pc = 0;
  unlockedBootRom = false;
  if (skipBootScreen) {
    registers.pc = 0x100;
    unlockedBootRom = true;
  }

  clockCycle = 0;
  TAC = 0;
  TIMA = 0;
  TMA = 0;
  IF = 0;
  if (logRegisters) {
    registers.a = 0;
    registers.f = 0;
    registers.b = 0;
    registers.c = 0;
    registers.d = 0;
    registers.e = 0;
    registers.h = 0;
    registers.l = 0;
    registers.sp = 0;
  }
}

bool CPU::step() {
  clockCycle += 1;
  bool currentANDresult =
      ((clockCycle >> timerLUT[TAC & 0x3]) & 0x1) && ((TAC >> 2) & 0x1);
  // printf("Clock&timerLUT: %i, TAC: %02X, TAC&0x4: %i, currentANDresult: %i,
  // previousANDresult: %i\n", ((clockCycle >> timerLUT[TAC & 0x3]) & 0x1), TAC,
  // ((TAC >> 2) & 0x1), currentANDresult, previousANDresult);
  if (previousANDresult && !currentANDresult) {
    TIMA++;
    // printf("Inc TIMA to %02X (TMA: %02X)\n", TIMA, TMA);
    if (TIMA == 0x0) {
      timerHasOverflowed = true;
    }
  }
  previousANDresult = currentANDresult;
  if (interruptChangeStateDelay >= 0) {
    if (interruptChangeStateDelay == 0) {
      interruptsEnabled = interruptsShouldBeEnabled;
    }
    interruptChangeStateDelay--;
  }
  if (halted) {
    if (IF == 0)
      return !breakpoint;

    halted = false;
    if (!interruptsEnabled)
      hasRecoveredFromHalt = false;
  }

  bus->syncronize();

  uint8_t opcode = read(registers.pc);
  if (!instr) {
    // if (registers.pc >= 0x100)
    //  std::this_thread::sleep_for(std::chrono::seconds(100));
    if (interruptsEnabled && IF & IE) {
      uint16_t interruptAddress = 0xFFFF;
      if (IE & interruptVblank && IF & interruptVblank) {
        interruptAddress = interruptVblankAddress;
        IF &= ~interruptVblank;
      } else if (IE & interruptLCDC && IF & interruptLCDC) {
        interruptAddress = interruptLCDCAddress;
        IF &= ~interruptLCDC;
      } else if (IE & interruptTimer && IF & interruptTimer) {
        interruptAddress = interruptTimerAddress;
        IF &= ~interruptTimer;
      } else if (IE & interruptSerialComplete && IF & interruptSerialComplete) {
        interruptAddress = interruptSerialCompleteAddress;
        IF &= ~interruptSerialComplete;
      } else if (IE & interruptInput && IF & interruptInput) {
        interruptAddress = interruptInput;
        IF &= ~interruptInput;
      }

      if (interruptAddress != 0xFFFF) {
        setInterruptEnable(false);
        registers.sp--;
        write(registers.sp, registers.pc >> 8);
        registers.sp--;
        write(registers.sp, registers.pc & 0xFF);

        registers.pc = interruptAddress;
        return !breakpoint;
      }
    }
    if (logRegisters) {
      printf("A: %02X F: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X "
             "SP: %04X PC: 00:%04X (%02X %02X %02X %02X) TIMA: %02X\n",
             registers.a, registers.f, registers.b, registers.c, registers.d,
             registers.e, registers.h, registers.l, registers.sp, registers.pc,
             read(registers.pc), read(registers.pc + 1), read(registers.pc + 2),
             read(registers.pc + 3), TIMA);
    }

    instr = instruction::decode(opcode);
    if (instr->getType() != instruction::Nop) {
      // printf("0x%04X: %02X | ", registers.pc, opcode);
      // util::printfBits("", opcode, 8);
      // printf("\tInstruction: %s\n", instr->getName().c_str());
    }

    if (instr->getType() == instruction::Unsupported) {
      printf("Unsupported instruction\n");
      breakpoint = true;
    }
  } else if (!instr->isFinished()) {
    instr->amend(opcode);
  }

  if (hasRecoveredFromHalt)
    registers.pc++;

  if (instr->isFinished()) {
    if (instr->execute(this))
      instr = nullptr;
    else if (hasRecoveredFromHalt)
      registers.pc--;
  }

  if (timerHasOverflowed) {
    IF |= interruptTimer;
    TIMA = TMA;
    timerHasOverflowed = false;
  }

  hasRecoveredFromHalt = true;

  // if (registers.pc == 0xC5FA)
  // breakpoint = true;

  return !breakpoint;
}

void CPU::raiseInterrupt(int interrupt) { IF |= interrupt; }

uint8_t CPU::read(uint16_t addr) {
  if (addr < 0x8000) {
    if (!unlockedBootRom && addr < 0x100)
      return boot[addr];
    return bus->read(addr);
  }
  if (addr >= 0x8000 && addr < 0xA000)
    return bus->read(addr);
  if (addr >= 0xA000 && addr < 0xC000)
    return bus->read(addr);
  if (addr >= 0xC000 && addr < 0xE000)
    return bus->read(addr);
  if (addr >= 0xE000 && addr < 0xFE00)
    return bus->read(addr);
  if (addr >= 0xFE00 && addr < 0xFEA0)
    return bus->read(addr);
  if (addr >= 0xFF00 && addr < 0xFF4C) {
    if (addr == 0xFF04)
      return clockCycle >> 6;
    if (addr == 0xFF05)
      return TIMA;
    if (addr == 0xFF06)
      return TMA;
    if (addr == 0xFF07)
      return TAC;
    if (addr == 0xFF0F)
      return IF;
    if (addr >= 0xFF40 && addr <= 0xFF4B) {
      return bus->read(addr);
    }

    return bus->read(addr);
  }
  if (addr >= 0xFF80 && addr <= 0xFFFE)
    return zeropage[addr - 0xFF80];
  if (addr == 0xFFFF)
    return IE;

  breakpoint = true;
  printf("ERROR: READ MEMORY OUT OF BOUNDS : %04X\n", addr);
  return 0xFF;
}

void CPU::write(uint16_t addr, uint8_t value) {
  if (addr < 0x8000) {
    bus->write(addr, value);
  } else if (addr >= 0x8000 && addr < 0xA000)
    bus->write(addr, value);
  else if (addr >= 0xA000 && addr < 0xC000)
    bus->write(addr, value);
  else if (addr >= 0xC000 && addr < 0xE000)
    bus->write(addr, value);
  else if (addr >= 0xE000 && addr < 0xFE00)
    bus->write(addr, value);
  else if (addr >= 0xFE00 && addr < 0xFEA0)
    bus->write(addr, value);
  else if (addr >= 0xFF00 && addr < 0xFF4C) {
    if (addr == 0xFF01) {
      // serial
      // fprintf(stderr, "%c", value);
    } else if (addr == 0xFF02) {
    } else if (addr == 0xFF04) {
      clockCycle = 0;
    } else if (addr == 0xFF05) {
      TIMA = value;
      timerHasOverflowed = false;
      // printf("Wrote %02X to TIMA\n", value);
    } else if (addr == 0xFF06) {
      TMA = value;
      // printf("Wrote %02X to TMA\n", TMA);
    } else if (addr == 0xFF07) {
      TAC = value;
      // printf("Wrote %02X to TAC\n", TAC);
    } else if (addr == 0xFF0F) {
      IF = value;
    } else {
      bus->write(addr, value);
    }
  } else if (addr == 0xFF50 && value == 0x01) {
    unlockedBootRom = true;
  } else if (addr >= 0xFF80 && addr <= 0xFFFE) {
    zeropage[addr - 0xFF80] = value;
  } else if (addr == 0xFFFF) {
    // printf("Wrote %02X to IE\n", value);
    // util::printfBits("IE bits: ", value, 8);
    IE = value;
  } else {
    breakpoint = true;
    printf("ERROR: WRITE MEMORY OUT OF BOUNDS at %04X\n", addr);
  }
}

void CPU::dumpBoot() { util::hexdump(boot, boot.size()); }

// void CPU::dumpRom() { util::hexdump(rom, rom.size()); }
//
// void CPU::dumpVRam() { util::hexdump(vram, vram.size(), 0x8000); }

void CPU::dumpRam() { util::hexdump(ram, ram.size(), 0xC000); }

void CPU::dumpRegisters() {
  printf("        == Registers ===\n");
  printf("================");
  printf("================\n");
  printf("===    PC    ===");
  printf("===    SP    ===\n");
  printf("===   %04X   ===", registers.pc);
  printf("===   %04X   ===\n", registers.sp);
  printf("================");
  printf("================\n");
  printf("==  A  == F   ==");
  printf("==  B  == C   ==\n");
  printf("==  %02X == %02X  ==", registers.a, registers.f);
  printf("==  %02X == %02X  ==\n", registers.b, registers.c);
  printf("================");
  printf("================\n");
  printf("==  D  == E   ==");
  printf("==  H  == L   ==\n");
  printf("==  %02X == %02X  ==", registers.d, registers.e);
  printf("==  %02X == %02X  ==\n", registers.h, registers.l);
  printf("================");
  printf("================\n");
  printf("===   ZNHC   ===\n");
  printf("===   %i%i%i%i   ===\n", registers.f >> 7, (registers.f >> 6) & 1,
         (registers.f >> 5) & 1, (registers.f >> 4) & 1);
  printf("================\n");
}

void startRenderLoop(PPU *ppu) {
  ppu->setup();
  ppu->render();
  ppu->close();
  ppu->cleanup();
}

int main(int argc, char **argv) {
  Bus bus;
  CPU cpu(&bus);
  PPU ppu(&bus);
  bus.connectCPU(&cpu);
  bus.connectPPU(&ppu);

  if (argc == 2) {
    bus.loadCartridge(util::readFile(argv[1]));
    printf("Loaded Cartride!\n");
  }
  cpu.loadBoot(util::readFile("boot.bin"));
  // cpu.dumpBoot();

  auto syncTimer = std::chrono::high_resolution_clock::now();

  auto fpsCounter = std::chrono::high_resolution_clock::now();

  int cyclesPS = 0;
  int cyclesPF = 0;
  uint64_t cumulativeFrameTime = 0;

  std::thread th(startRenderLoop, &ppu);

  while (!ppu.isClosed()) {
    auto now = std::chrono::high_resolution_clock::now();
    auto fpsElapsed =
        std::chrono::duration_cast<std::chrono::seconds>(now - fpsCounter)
            .count();

    for (int i = 0; i < 1000; i++) {
      cyclesPS++;
      cyclesPF++;
      ppu.step();
      cpu.step();

      if (cyclesPF == 17'556) {
        auto syncNow = std::chrono::high_resolution_clock::now();
        auto syncTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            syncNow - syncTimer)
                            .count();
        // printf("Frame time: %f ms, did %i cycles\n", syncTime / 1'000'000.0f,
        // cyclesPF);
        cumulativeFrameTime += syncTime;
        cyclesPF = 0;

        // TODO: fix this
        if (syncTime < 1'000'000'000 / 60) {
          uint64_t sleepTime = (1'000'000'000 / 60) - syncTime;
		  // printf("Sleeping for: %f ms\n", sleepTime / 1'000'000.0f);
		  std::this_thread::sleep_for(std::chrono::nanoseconds(sleepTime));
        }
        syncTimer = std::chrono::high_resolution_clock::now();
        break;
      }
    }

    if (fpsElapsed > 1.0) {
      fpsCounter += std::chrono::seconds(1);
      printf("FPS: %d\n", ppu.getFrame());
      printf("CYCLES: %d\n", cyclesPS);
      printf("Time per frame: %f\n",
             (cumulativeFrameTime / (float)ppu.getFrame()) / 1'000'000.0f);
      ppu.setFrame(0);
      cyclesPS = 0;
      cumulativeFrameTime = 0;
    }
  }

  th.join();

  return 0;
}

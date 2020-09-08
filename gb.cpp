#include "gb.h"

#include "ppu.h"

#include "instructions.h"
#include "utils.h"
#include <chrono>
#include <iostream>
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

constexpr uint32_t timerDividerLUT[] = {256, 4, 16, 64}; // In m cycles

constexpr bool logRegisters = false;

CPU::CPU(Bus *bus)
    : boot(0x100), ram(0x2000), zeropage(0xFFFE - 0xFF80), bus(bus) {
  registers.pc = 0; // TODO: reset to 0
  unlockedBootRom = false;

  clockCycle = 0;
  TAC = 0;
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

int count = 0;

bool CPU::step() {
  clockCycle++;
  if (clockCycle % 64) {
    DIV++;
  }
  if (clockCycle % timerDividerLUT[TAC & 0x3] == 0 && TAC & 0x4 &&
      ++TIMA == 0) {
    IF |= interruptTimer;
    TIMA = TMA;
  }
  if (interruptChangeStateDelay >= 0) {
    if (interruptChangeStateDelay == 0) {
      interruptsEnabled = interruptsShouldBeEnabled;
      if (interruptsEnabled)
        printf("Enabled interrupts!\n");
      else
        printf("Disabled interrupts!\n");
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

  uint8_t opcode = read(registers.pc);
  if (!instr) {
    // if (registers.pc >= 0x100)
    //  std::this_thread::sleep_for(std::chrono::seconds(100));
    if (interruptsEnabled && IF) {
      uint16_t interruptAddress = 0xFFFF;
      if (IE & interruptVblank && IF & interruptVblank) {
        printf("Vblank interrupt!\n");
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
		  printf("Interrupt!\n");
        setInterruptEnable(false);
        registers.sp--;
        write(registers.sp, registers.pc >> 8);
        registers.sp--;
        write(registers.sp, registers.pc & 0xFF);

        registers.pc = interruptAddress;
        return !breakpoint;
      } else {
        IF = 0;
      }
    }
    if (logRegisters) {
      printf("A: %02X F: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X "
             "SP: %04X PC: 00:%04X (%02X %02X %02X %02X)\n",
             registers.a, registers.f, registers.b, registers.c, registers.d,
             registers.e, registers.h, registers.l, registers.sp, registers.pc,
             read(registers.pc), read(registers.pc + 1), read(registers.pc + 2),
             read(registers.pc + 3));
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
    return ram[addr - 0xC000];
  if (addr >= 0xE000 && addr < 0xFE00)
    return ram[addr - 0xE000];
  if (addr >= 0xFE00 && addr < 0xFEA0)
    return bus->read(addr);
  if (addr >= 0xFF00 && addr < 0xFF4C) {
    if (addr == 0xFF00) {
      printf("Read input\n");
      return 0xFF;
    }
    if (addr == 0xFF04)
      return DIV;
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

    fprintf(stderr, "READ TO UNCONNECTED IO at %04X\n", addr);
    return 0xFF;
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
    ram[addr - 0xC000] = value;
  else if (addr >= 0xE000 && addr < 0xFE00)
    ram[addr - 0xE000] = value;
  else if (addr >= 0xFE00 && addr < 0xFEA0)
    bus->write(addr, value);
  else if (addr >= 0xFF00 && addr < 0xFF4C) {
    if (addr == 0xFF00) {
      printf("Wrote to P1\n");
    } else if (addr == 0xFF01) {
      fprintf(stderr, "%c", value);
    } else if (addr == 0xFF02) {
    } else if (addr == 0xFF04) {
      DIV = 0;
    } else if (addr == 0xFF05) {
      TIMA = value;
    } else if (addr == 0xFF06) {
      TMA = value;
    } else if (addr == 0xFF07) {
      TAC = value;
    } else if (addr == 0xFF0F) {
      IF = value;
    } else if (addr >= 0xFF40 && addr <= 0xFF4B) {
      bus->write(addr, value);
    } else {
      fprintf(stderr, "WRITE TO UNCONNECTED IO at %04X\n", addr);
    }
  } else if (addr == 0xFF50 && value == 0x01) {
    unlockedBootRom = true;
  } else if (addr >= 0xFF80 && addr <= 0xFFFE) {
    zeropage[addr - 0xFF80] = value;
  } else if (addr == 0xFFFF) {
    printf("Wrote %02X to IE\n", value);
    IE = value;
    util::printfBits("IE bits: ", IE, 8);
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
  }
  cpu.loadBoot(util::readFile("boot.bin"));
  // cpu.dumpBoot();

  using cycles = std::chrono::duration<double, std::ratio<4, 4'194'304>>;

  auto lastTime = std::chrono::high_resolution_clock::now();
  auto fpsCounter = std::chrono::high_resolution_clock::now();

  int cyclesPS = 0;

  std::thread th(startRenderLoop, &ppu);

  while (!ppu.isClosed()) {
    auto now = std::chrono::high_resolution_clock::now();
    auto cycleCount =
        std::chrono::duration_cast<cycles>(now - lastTime).count();
    auto fpsElapsed =
        std::chrono::duration_cast<std::chrono::seconds>(now - fpsCounter)
            .count();

    while (cycleCount > 1.0) {
      lastTime =
          lastTime +
          std::chrono::duration_cast<std::chrono::nanoseconds>(cycles(1.0));
      cyclesPS++;
      cycleCount -= 1.0;
      ppu.step();
      cpu.step();
    }

    if (fpsElapsed > 1.0) {
      fpsCounter += std::chrono::seconds(1);
      printf("FPS: %d\n", ppu.getFrame());
      printf("CYCLES: %d\n", cyclesPS);
      ppu.setFrame(0);
      cyclesPS = 0;
    }

    if (ppu.getLY() == 0 && ppu.getLX() == 0) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(16)); // TODO: fix this
    }
  }

  th.join();

  return 0;
}

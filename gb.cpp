#include "gb.h"

#include "instructions.h"
#include "utils.h"
#include <iostream>

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

constexpr bool logRegisters = true;

CPU::CPU()
    : boot(0x100), rom(0x8000), switchableRam(0x2000), ram(0x2000),
      vram(0x2000), oam(0xFEA0 - 0xFE00), zeropage(0xFFFE - 0xFF80) {
  registers.pc = 0;
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
    if (registers.pc == 0x86) {
		dumpVRam();
		exit(0);
    }

    if (interruptsEnabled && IF) {
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
      //printf("0x%04X: %02X | ", registers.pc, opcode);
      //util::printfBits("", opcode, 8);
      //printf("\tInstruction: %s\n", instr->getName().c_str());
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

uint8_t CPU::read(uint16_t addr) {
  if (addr < 0x8000) {
    if (!unlockedBootRom && addr < 0x100)
      return boot[addr];
    return rom[addr];
  }
  if (addr >= 0x8000 && addr < 0xA000)
    return vram[addr - 0x8000];
  if (addr >= 0xA000 && addr < 0xC000)
    return switchableRam[addr - 0xA000];
  if (addr >= 0xC000 && addr < 0xE000)
    return ram[addr - 0xC000];
  if (addr >= 0xE000 && addr < 0xFE00)
    return ram[addr - 0xE000];
  if (addr >= 0xFE00 && addr < 0xFEA0)
    return oam[addr - 0xE000];
  if (addr >= 0xFF00 && addr < 0xFF4C) {
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
	if (addr == 0xFF42) {
		fprintf(stderr, "Read SCY\n");
		return 0xFF;
	}
    if (addr == 0xFF44)
      return 0x90;

    fprintf(stderr, "UNCONNECTED IO at %04X\n", addr);
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
    breakpoint = true;
    printf("ERROR: WRITING %02X TO ROM at %04X\n", value, addr);
  } else if (addr >= 0x8000 && addr < 0xA000)
    vram[addr - 0x8000] = value;
  else if (addr >= 0xA000 && addr < 0xC000)
    switchableRam[addr - 0xA000] = value;
  else if (addr >= 0xC000 && addr < 0xE000)
    ram[addr - 0xC000] = value;
  else if (addr >= 0xE000 && addr < 0xFE00)
    ram[addr - 0xE000] = value;
  else if (addr >= 0xFE00 && addr < 0xFEA0)
    oam[addr - 0xE000] = value;
  else if (addr >= 0xFF00 && addr < 0xFF4C) {
    if (addr == 0xFF01) {
      fprintf(stderr, "%c", value);
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
    } else if (addr == 0xFF42) {
      fprintf(stderr, "Wrote %02x to SCY\n", value);
    } else if (addr == 0xFF43) {
      fprintf(stderr, "Wrote %02x to SCX\n", value);
    }
  } else if (addr >= 0xFF80 && addr <= 0xFFFE) {
    zeropage[addr - 0xFF80] = value;
  } else if (addr == 0xFFFF) {
    IE = value;
  } else {
    breakpoint = true;
    printf("ERROR: WRITE MEMORY OUT OF BOUNDS at %04X\n", addr);
  }
}

void CPU::dumpBoot() { util::hexdump(boot, boot.size()); }

void CPU::dumpRom() { util::hexdump(rom, rom.size()); }

void CPU::dumpVRam() { util::hexdump(vram, vram.size(), 0x8000); }

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

int main(int argc, char **argv) {
  CPU cpu;

  if (argc == 2) {
    auto file = util::readFile(argv[1]);
    cpu.loadRom(file);
  }
  cpu.loadBoot(util::readFile("boot.bin"));
  //cpu.dumpBoot();

  while (true) {
    if (!cpu.step()) {
      bool moveOn = false;
      while (!moveOn) {
        std::string in;
        getline(std::cin, in);
        std::cout << "\x1b[A";
        if (in == "q")
          return 0;
        else if (in == "r") {
          cpu.dumpRegisters();
        } else if (in == "dboot") {
          cpu.dumpBoot();
        } else if (in == "drom") {
          cpu.dumpRom();
        } else if (in == "dvram") {
          cpu.dumpVRam();
        } else if (in == "dram") {
          cpu.dumpRam();
        } else
          moveOn = true;
      }
    }
  }
}

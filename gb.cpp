#include "gb.h"

#include "instructions.h"
#include "utils.h"
#include <iostream>

constexpr bool logRegisters = true;

CPU::CPU()
    : rom(0x8000), switchableRam(0x2000), ram(0x2000), vram(0x2000),
      zeropage(0xFFFE - 0xFF80) {
  registers.pc = 0;
  if (logRegisters) {
    registers.pc = 0x100;
    registers.sp = 0xFFFE;
    registers.a = 0x01;
    registers.f = 0xB0;
    registers.b = 0x00;
    registers.c = 0x13;
    registers.d = 0x00;
    registers.e = 0xD8;
    registers.h = 0x01;
    registers.l = 0x4D;
  }
}

int count = 0;

bool CPU::step() {
  uint8_t opcode = read(registers.pc);
  if (!instr) {
    count++;
    if (count == 70000)
      exit(0);

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

    if (instr->getType() == instruction::Unsupported)
      breakpoint = true;
  } else if (!instr->isFinished()) {
    instr->amend(opcode);
  }

  registers.pc++;

  if (instr->isFinished()) {
    if (instr->execute(this))
      instr = nullptr;
    else
      registers.pc--;
  }

  //  if (registers.pc == 0x100)
  //    breakpoint = true;

  return !breakpoint;
}

uint8_t CPU::read(uint16_t addr) {
  if (addr < 0x8000)
    return rom[addr];
  if (addr >= 0x8000 && addr < 0xA000)
    return vram[addr - 0x8000];
  if (addr >= 0xA000 && addr < 0xC000)
    return switchableRam[addr - 0xA000];
  if (addr >= 0xC000 && addr < 0xE000)
    return ram[addr - 0xC000];
  if (addr >= 0xFF00 && addr < 0xFF4C) {

    return 0x90;
  }
  if (addr >= 0xFF80 && addr <= 0xFFFE)
    return zeropage[addr - 0xFF80];

  breakpoint = true;
  printf("ERROR: READ MEMORY OUT OF BOUNDS : %04X\n", addr);
  return 0xFF;
}

void CPU::write(uint16_t addr, uint8_t value) {
  if (addr < 0x8000)
    rom[addr] = value;
  else if (addr >= 0x8000 && addr < 0xA000)
    vram[addr - 0x8000] = value;
  else if (addr >= 0xA000 && addr < 0xC000)
    switchableRam[addr - 0xA000] = value;
  else if (addr >= 0xC000 && addr < 0xE000)
    ram[addr - 0xC000] = value;
  else if (addr >= 0xFF00 && addr < 0xFF4C) {
    return;
    // if (addr == 0xFF26) {
    //  if (value >> 7)
    //    printf("Enabled Audio\n");
    //  else
    //    printf("Disabled Audio\n");
    //} else
    //  printf("%02X: Wrote %02X to IO at %04X\n", registers.pc, value, addr);
  } else if (addr >= 0xFF80 && addr <= 0xFFFE) {
    zeropage[addr - 0xFF80] = value;
  } else if (addr == 0xFFFF) {
    // printf("Set Interrupt Register to 0x%02X\n", value);
  } else {
    breakpoint = true;
    printf("ERROR: WRITE MEMORY OUT OF BOUNDS\n");
  }
}
void CPU::dumpRom() { util::hexdump(rom, 0x1000); }

void CPU::dumpVRam() { util::hexdump(vram, vram.size(), 0x8000); }

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

  if (argc != 2)
    return 0;

  auto file = util::readFile(argv[1]);

  cpu.loadRom(file);
  //  cpu.loadBoot(util::readFile("boot.bin"));
  // cpu.dumpRom();

  while (!cpu.hasHalted()) {
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
        } else if (in == "dvr") {
          cpu.dumpVRam();
        } else
          moveOn = true;
      }
    }
  }
}

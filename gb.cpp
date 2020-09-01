#include "gb.h"

#include "instructions.h"
#include "utils.h"
#include <iostream>

bool CPU::step() {
  bool breakpoint = false;

  uint8_t opcode = read(registers.pc);

  if (!instr) {
    instr = instruction::decode(opcode);
    if (instr->getType() != instruction::Nop) {
      printf("0x%04X: %02X | ", registers.pc, opcode);
      util::printfBits("", opcode, 8);
      printf("\tInstruction: %s\n", instr->getName().c_str());
    }

    if (instr->getType() == instruction::Nop)
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

  if(registers.pc < 0xc)
	  breakpoint = true;

  return breakpoint;
}

void CPU::dumpRom() { util::hexdump(rom, 0x1000); }

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
  cpu.loadBoot(util::readFile("boot.bin"));
  cpu.dumpRom();

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
        } else
          moveOn = true;
      }
    }
  }
}

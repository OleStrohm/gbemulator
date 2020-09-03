#include "instructions.h"

#include "log.h"
#include "utils.h"
#include <memory>

#define DECODE(X)                                                              \
  if (!instr)                                                                  \
  instr = X::decode(opcode)

Ret::Ret(Condition condition, bool enableInterrupts)
    : condition(condition), enableInterrupts(enableInterrupts), currentByte(0) {
  finished = true;
}

std::unique_ptr<Instruction> Ret::decode(uint8_t opcode) {
  Condition conditions[] = {Condition::NotZero, Condition::Zero,
                            Condition::NotCarry, Condition::Carry};

  if (((opcode >> 5) & 0x7) == 0b110 && (opcode & 0x7) == 0b000)
    return std::make_unique<Ret>(conditions[(opcode >> 3) & 0x3], false);
  if (opcode == 0b11001001)
    return std::make_unique<Ret>(Condition::Unconditional, false);
  if (opcode == 0b11011001)
    return std::make_unique<Ret>(Condition::Unconditional, true);

  return nullptr;
}

bool Ret::execute(CPU *cpu) {
  uint8_t flags = cpu->getRegisters().f;
  if (condition != Condition::Unconditional) {
    if (!wastedCycle) {
      wastedCycle = true;
      return false;
    }

    if (!checkedCondition) {
      if (condition == Condition::NotZero &&
          (flags & (1 << 7)) == 0) // Not Zero
        return true;
      if (condition == Condition::Zero && (flags & (1 << 7)) == 1) // Zero
        return true;
      if (condition == Condition::NotCarry &&
          (flags & (1 << 4)) == 0) // Not Carry
        return true;
      if (condition == Condition::Carry && (flags & (1 << 4)) == 1) // Carry
        return true;

      checkedCondition = true;
      return false;
    }
  }

  if (currentByte != 2) {
    address = (address >> 8) | (cpu->read(cpu->getRegisters().sp));
    cpu->getRegisters().sp += 1;
    currentByte++;

    return false;
  }

  cpu->getRegisters().pc = address;

  return true;
}

std::string Ret::getName() {
  return enableInterrupts
             ? "Reti"
             : (condition == Condition::Unconditional ? "Ret" : "Ret F");
}

int Ret::getType() { return instruction::Ret; }

RotateA::RotateA(bool isLeft, bool throughCarry)
    : isLeft(isLeft), throughCarry(throughCarry) {
  finished = true;
}

std::unique_ptr<Instruction> RotateA::decode(uint8_t opcode) {
  if (((opcode >> 5) & 0x7) == 0b000 && (opcode & 0x7) == 0b111)
    return std::make_unique<RotateA>(((opcode >> 3) & 0x1) == 0,
                                     (opcode >> 4) & 0x1);

  return nullptr;
}

bool RotateA::execute(CPU *cpu) {
  uint8_t &result = cpu->getRegisters().a;
  uint8_t &flags = cpu->getRegisters().f;

  if (throughCarry) {
    if (isLeft) {
      LOG("Rotate A left\n");
      bool carrySet = (flags & (1 << 4)) == (1 << 4);
      flags &= 0b11101111;
      if (result & 0x80)
        flags |= 1 << 4;

      result = result << 1;
      if (carrySet)
        result |= 1;
    } else {
      LOG("Rotate A right\n");
      bool carrySet = (flags & (1 << 4)) == (1 << 4);
      flags &= 0b11101111;
      if (result & 0x1)
        flags |= 1 << 4;

      result = result >> 1;
      if (carrySet)
        result |= 0x80;
    }
  } else {
    if (isLeft) {
      LOG("Rotate A left (without carry)\n");
      flags &= 0b11101111;
      bool carrySet = (result & 0x80) == 0x80;
      if (carrySet)
        flags |= 1 << 4;

      result = result << 1;
      if (carrySet)
        result |= 1;
    } else {
      LOG("Rotate A right (without carry)\n");
      flags &= 0b11101111;
      bool carrySet = (result & 0x1) == 0x1;
      if (carrySet)
        flags |= 1 << 4;

      result = result >> 1;
      if (carrySet)
        result |= 0x80;
    }
  }

  return true;
}

std::string RotateA::getName() { return throughCarry ? "RdA" : "RdCA"; }
int RotateA::getType() { return instruction::RotateA; }

PopPush::PopPush(uint8_t reg, bool isPop)
    : reg(reg), isPop(isPop), currentByte(0), hasWastedCycle(isPop) {
  finished = true;
}

std::unique_ptr<Instruction> PopPush::decode(uint8_t opcode) {
  if (((opcode >> 6) & 0x3) == 0b11 && ((opcode >> 3) & 1) == 0b0 &&
      (opcode & 0x3) == 0b01)
    return std::make_unique<PopPush>((opcode >> 4) & 0x3,
                                     ((opcode >> 2) & 1) == 0);

  return nullptr;
}

bool PopPush::execute(CPU *cpu) {
  uint16_t *registerMapping[]{&cpu->getRegisters().bc, &cpu->getRegisters().de,
                              &cpu->getRegisters().hl, &cpu->getRegisters().sp};

  std::string registerNames[2][4] = {{"C", "E", "L", "F"},
                                     {"B", "D", "H", "A"}};

  if (!hasWastedCycle) {
    hasWastedCycle = true;
    return false;
  }

  if (isPop) {
    ((uint8_t *)registerMapping[reg])[currentByte] =
        cpu->read(cpu->getRegisters().sp);
    LOG("POP %s (%02X from %04X)\n", registerNames[currentByte][reg].c_str(),
        ((uint8_t *)registerMapping[reg])[currentByte], cpu->getRegisters().sp);
    cpu->getRegisters().sp++;
  } else {
    cpu->getRegisters().sp--;
    cpu->write(cpu->getRegisters().sp,
               ((uint8_t *)registerMapping[reg])[1 - currentByte]);
    LOG("PUSH %s (%02X to %04X)\n", registerNames[1 - currentByte][reg].c_str(),
        ((uint8_t *)registerMapping[reg])[1 - currentByte],
        cpu->getRegisters().sp);
  }

  currentByte++;
  if (currentByte != 2)
    return false;

  return true;
}

std::string PopPush::getName() { return isPop ? "Pop" : "Push"; }
int PopPush::getType() { return instruction::PopPush; }

Call::Call(Condition condition)
    : needsBytes(2), address(0), condition(condition), storedPCCount(0),
      movedSP(false) {
  finished = false;
}

std::unique_ptr<Instruction> Call::decode(uint8_t opcode) {
  Condition conditions[] = {Condition::NotZero, Condition::Zero,
                            Condition::NotCarry, Condition::Carry};

  if (((opcode >> 5) & 0x7) == 0b110 && (opcode & 0x7) == 0b010)
    return std::make_unique<Call>(conditions[(opcode >> 3) & 0x3]);
  if (opcode == 0b11001101)
    return std::make_unique<Call>(Condition::Unconditional);

  return nullptr;
}

void Call::amend(uint8_t val) {
  needsBytes--;
  address = (address >> 8) | (val << 8);
  finished = needsBytes == 0;
}

bool Call::execute(CPU *cpu) {
  uint8_t flags = cpu->getRegisters().f;
  if (condition == Condition::NotZero && (flags & (1 << 7)) == 0) // Not Zero
    return true;
  if (condition == Condition::Zero && (flags & (1 << 7)) == 1) // Zero
    return true;
  if (condition == Condition::NotCarry && (flags & (1 << 4)) == 0) // Not Carry
    return true;
  if (condition == Condition::Carry && (flags & (1 << 4)) == 1) // Carry
    return true;

  if (storedPCCount != 2) {
    cpu->write(cpu->getRegisters().sp - storedPCCount,
               (cpu->getRegisters().pc >> (8 * (1 - storedPCCount))) & 0xFF);
    LOG("WROTE %02X to %04X\n",
        (cpu->getRegisters().pc >> (8 * (1 - storedPCCount))) & 0xFF,
        cpu->getRegisters().sp - storedPCCount);
    storedPCCount++;
    if (storedPCCount != 2)
      return false;
  }

  if (!movedSP) {
    cpu->getRegisters().sp -= 2;
    movedSP = true;
    return false;
  }

  cpu->getRegisters().pc = address;
  LOG("Call jump to %04X\n", address);

  return true;
}

std::string Call::getName() { return "Call"; }
int Call::getType() { return instruction::Call; }

IncDec::IncDec(bool increment, uint8_t destination, bool isBigRegister)
    : increment(increment), destination(destination),
      isBigRegister(isBigRegister) {
  finished = true;
  address = 0;
  gotAddress = false;
  storedValue = 0;
  retrievedStoredValue = false;
}

std::unique_ptr<Instruction> IncDec::decode(uint8_t opcode) {
  if ((opcode >> 6) == 0b00 && (opcode & 0x7) == 0b011)
    return std::make_unique<IncDec>(((opcode >> 3) & 1) == 0,
                                    (opcode >> 4) & 0x3, true);
  if ((opcode >> 6) == 0b00 && ((opcode >> 1) & 0x3) == 0b10)
    return std::make_unique<IncDec>((opcode & 1) == 0, (opcode >> 3) & 0x7,
                                    false);

  return nullptr;
}

bool IncDec::execute(CPU *cpu) {
  uint8_t &flags = cpu->getRegisters().f;

  if (isBigRegister) {
    uint16_t *registerMapping[]{
        &cpu->getRegisters().bc, &cpu->getRegisters().de,
        &cpu->getRegisters().hl, &cpu->getRegisters().sp};

    uint16_t *dest = registerMapping[destination];

    if (increment)
      *dest = *dest + 1;
    else
      *dest = *dest - 1;

    flags &= 0b00010000;
    if (*dest == 0)
      flags |= 0b10000000;
  } else {
    uint8_t *registerMapping[] = {&cpu->getRegisters().b,
                                  &cpu->getRegisters().c,
                                  &cpu->getRegisters().d,
                                  &cpu->getRegisters().e,
                                  &cpu->getRegisters().h,
                                  &cpu->getRegisters().l,
                                  nullptr,
                                  &cpu->getRegisters().a};

    uint8_t *dest = registerMapping[destination];

    if (dest == nullptr) {
      if (!gotAddress) {
        address = cpu->getRegisters().hl;
        gotAddress = true;
        return false;
      }

      if (!retrievedStoredValue) {
        storedValue = cpu->read(address);
        retrievedStoredValue = true;
        return false;
      }

      if (increment)
        cpu->write(address, storedValue + 1);
      else
        cpu->write(address, storedValue - 1);

      flags &= 0b00010000;
      if (increment) {
        if (((*dest) + 1) == 0)
          flags |= 0b10000000;
        else if (((*dest) - 1) == 0)
          flags |= 0b10000000;
      }
    } else {
      if (increment)
        *dest = *dest + 1;
      else
        *dest = *dest - 1;

      flags &= 0b00010000;
      if (*dest == 0)
        flags |= 0b10000000;
    }
  }

  return true;
}

std::string IncDec::getName() { return increment ? "Inc" : "Dec"; }
int IncDec::getType() {
  return increment ? instruction::Inc : instruction::Dec;
}

ExtendedInstruction::ExtendedInstruction()
    : instruction(0), address(0), gotAddress(0), result(0), shouldWrite(false) {
  finished = false;
}

std::unique_ptr<Instruction> ExtendedInstruction::decode(uint8_t opcode) {
  if (opcode == 0xCB)
    return std::make_unique<ExtendedInstruction>();

  return nullptr;
}

void ExtendedInstruction::amend(uint8_t val) {
  instruction = val;
  finished = true;
  LOG("\tCB Instruction: %02X | ", val);
  util::printfBits("", val, 8);
}

bool ExtendedInstruction::execute(CPU *cpu) {
  uint8_t *registerMapping[] = {&cpu->getRegisters().b,
                                &cpu->getRegisters().c,
                                &cpu->getRegisters().d,
                                &cpu->getRegisters().e,
                                &cpu->getRegisters().h,
                                &cpu->getRegisters().l,
                                nullptr,
                                &cpu->getRegisters().a};

  if ((instruction & 0x7) == 0b110 && !gotAddress) {
    address = cpu->getRegisters().hl;
    gotAddress = true;
    return false;
  }

  if (shouldWrite) {
    cpu->write(address, result);
    return true;
  }

  result =
      gotAddress ? cpu->read(address) : *registerMapping[instruction & 0x7];
  uint8_t &flags = cpu->getRegisters().f;
  if (instruction >> 4 == 0b0000) { // RdC D
    bool isLeft = ((instruction >> 3) & 1) == 0;
    if (isLeft) {
      LOG("Rotate left (without carry\n");
      flags &= 0b11101111;
      bool carrySet = (result & 0x80) == 0x80;
      if (carrySet)
        flags |= 1 << 4;

      result = result << 1;
      if (carrySet)
        result |= 1;

      if (registerMapping[instruction & 0x7] != nullptr) {
        *registerMapping[instruction & 0x7] = result;
      } else {
        shouldWrite = true;
        return false;
      }
    } else {
      LOG("Rotate right (without carry\n");
      flags &= 0b11101111;
      bool carrySet = (result & 0x1) == 0x1;
      if (carrySet)
        flags |= 1 << 4;

      result = result >> 1;
      if (carrySet)
        result |= 0x80;

      if (registerMapping[instruction & 0x7] != nullptr) {
        *registerMapping[instruction & 0x7] = result;
      } else {
        shouldWrite = true;
        return false;
      }
    }
  } else if (instruction >> 4 == 0b0001) { // Rd D
    bool isLeft = ((instruction >> 3) & 1) == 0;
    if (isLeft) {
      LOG("Rotate left\n");
      bool carrySet = (flags & (1 << 4)) == (1 << 4);
      flags &= 0b11101111;
      if (result & 0x80)
        flags |= 1 << 4;

      result = result << 1;
      if (carrySet)
        result |= 1;

      if (registerMapping[instruction & 0x7] != nullptr) {
        *registerMapping[instruction & 0x7] = result;
      } else {
        shouldWrite = true;
        return false;
      }
    } else {
      LOG("Rotate right\n");
      bool carrySet = (flags & (1 << 4)) == (1 << 4);
      flags &= 0b11101111;
      if (result & 0x1)
        flags |= 1 << 4;

      result = result >> 1;
      if (carrySet)
        result |= 0x80;

      if (registerMapping[instruction & 0x7] != nullptr) {
        *registerMapping[instruction & 0x7] = result;
      } else {
        shouldWrite = true;
        return false;
      }
    }
  } else if (instruction >> 4 == 0b0010) { // sdA D
    bool isLeft = ((instruction >> 3) & 1) == 0;
    if (isLeft) {
      LOG("Shift left\n");
      flags &= 0b11101111;
      bool carrySet = (result & 0x80) == 0x80;
      if (carrySet)
        flags |= 1 << 4;

      result = result << 1;

      if (registerMapping[instruction & 0x7] != nullptr) {
        *registerMapping[instruction & 0x7] = result;
      } else {
        shouldWrite = true;
        return false;
      }
    } else {
      LOG("Arithmetic shift right\n");
      flags &= 0b11101111;
      bool carrySet = (result & 0x1) == 0x1;
      if (carrySet)
        flags |= 1 << 4;

      result = result >> 1;
      if (result & 0b01000000)
        result |= 0x80;

      if (registerMapping[instruction & 0x7] != nullptr) {
        *registerMapping[instruction & 0x7] = result;
      } else {
        shouldWrite = true;
        return false;
      }
    }
  } else if (instruction >> 3 == 0b00110) { // SWAP D
    LOG("SWAP\n");
    result = (result << 4) | (result >> 4);

    if (registerMapping[instruction & 0x7] != nullptr) {
      *registerMapping[instruction & 0x7] = result;
    } else {
      shouldWrite = true;
      return false;
    }
  } else if (instruction >> 3 == 0b00111) { // SRL D
    LOG("Logical shift right\n");
    flags &= 0b11101111;
    bool carrySet = (result & 0x1) == 0x1;
    if (carrySet)
      flags |= 1 << 4;

    result = result >> 1;

    if (registerMapping[instruction & 0x7] != nullptr) {
      *registerMapping[instruction & 0x7] = result;
    } else {
      shouldWrite = true;
      return false;
    }
  } else if (instruction >> 6 == 0b01) { // BIT N,D
    LOG("Testing BIT\n");
    uint8_t n = (instruction >> 3) & 0x7;

    flags &= 0b00010000;
    flags |= 1 << 5;
    if (((result) & (1 << n)) == 0)
      flags |= 1 << 7;
  } else if (instruction >> 6 == 0b10) { // RES N,D
    LOG("Resetting BIT\n");
    uint8_t n = (instruction >> 3) & 0x7;

    result &= ~(1 << n);

    if (registerMapping[instruction & 0x7] != nullptr) {
      *registerMapping[instruction & 0x7] = result;
    } else {
      shouldWrite = true;
      return false;
    }
  } else if (instruction >> 6 == 0b11) { // SET N,D
    LOG("Setting BIT\n");
    uint8_t n = (instruction >> 3) & 0x7;

    result |= (1 << n);

    if (registerMapping[instruction & 0x7] != nullptr) {
      *registerMapping[instruction & 0x7] = result;
    } else {
      shouldWrite = true;
      return false;
    }
  }

  return true;
}

std::string ExtendedInstruction::getName() { return "CB extended instruction"; }
int ExtendedInstruction::getType() { return instruction::CB; }

RST::RST(uint8_t n) : n(n), state(3) { finished = true; }

std::unique_ptr<Instruction> RST::decode(uint8_t opcode) {
  if ((opcode >> 6) == 0b11 && (opcode & 0x7) == 0b111)
    return std::make_unique<RST>(((opcode >> 3) & 0x7) << 3);

  return nullptr;
}

bool RST::execute(CPU *cpu) {
  switch (state) {
  case 3: {
    cpu->getRegisters().sp -= 2;
    state--;
    return false;
  }
  case 2: {
    cpu->write(cpu->getRegisters().sp, cpu->getRegisters().pc);
    state--;
    return false;
  }
  case 1: {
    state--;
    return false;
  }
  case 0: {
    printf("RST %02X", n);
    cpu->getRegisters().pc = n;
  }
  }
  return true;
}

std::string RST::getName() { return "RST"; }
int RST::getType() { return instruction::RST; }

ALU::ALU(bool takesImmediate, uint8_t operation, uint8_t source)
    : takesImmediate(takesImmediate), operation(operation), source(source),
      address(0) {
  finished = !takesImmediate;
}

std::unique_ptr<Instruction> ALU::decode(uint8_t opcode) {
  if ((opcode >> 7) == 0b1 &&
      (((opcode >> 6) & 1) == 0 || (opcode & 0x7) == 0b110))
    return std::make_unique<ALU>(((opcode >> 6) & 1) == 1, (opcode >> 3) & 0x7,
                                 opcode & 0x7);

  return nullptr;
}

void ALU::amend(uint8_t val) {
  immediate = val;
  LOG("Got immediate\n");
  finished = true;
}

bool ALU::execute(CPU *cpu) {
  uint8_t *registerMapping[] = {&cpu->getRegisters().b,
                                &cpu->getRegisters().c,
                                &cpu->getRegisters().d,
                                &cpu->getRegisters().e,
                                &cpu->getRegisters().h,
                                &cpu->getRegisters().l,
                                nullptr,
                                &cpu->getRegisters().a};

  uint8_t *dest = &cpu->getRegisters().a;

  if (source == 0b110 && !address) {
    address = cpu->getRegisters().hl;
    return false;
  }

  uint8_t &flags = cpu->getRegisters().f;
  uint8_t src = address ? cpu->read(address) : *(registerMapping[source]);
  uint8_t carry = (flags >> 4) & 1;

  flags = 0;
  // TODO: implement N flag and make sure it's right
  switch (operation) {
  case 0b000 /*ADD*/: {
    if (((uint32_t)*dest) + ((uint32_t)src) > 0xFF)
      flags |= 1 << 4;

    *dest += src;

    if (*dest == 0)
      flags |= 1 << 7;
  }
  case 0b001 /*ADC*/:
    if (((uint32_t)*dest) + ((uint32_t)src) + (uint32_t)carry > 0xFF)
      flags |= 1 << 4;

    *dest += src + carry;

    if (*dest == 0)
      flags |= 1 << 7;
  case 0b010 /*SUB*/:
    if (*dest < src)
      flags |= 1 << 4;

    *dest -= src;

    if (*dest == 0)
      flags |= 1 << 7;
    flags |= 0b01000000;
  case 0b011 /*SBC*/:
    if (((int32_t)*dest) - ((int32_t)src) + (int32_t)carry - 1 < 0)
      flags |= 1 << 4;

    *dest += -(src) + carry - 1;

    if (*dest == 0)
      flags |= 1 << 7;
    flags |= 0b01000000;
  case 0b100 /*AND*/:
    *dest &= src;

    if (*dest == 0)
      flags |= 1 << 7;
    flags |= 1 << 5;
  case 0b101 /*XOR*/:
    *dest ^= src;

    if (*dest == 0)
      flags |= 1 << 7;
  case 0b110 /*OR */:
    *dest |= src;

    if (*dest == 0)
      flags |= 1 << 7;
  case 0b111 /*CP */:
    flags |= 0b01000000;
    if (*dest < src)
      flags |= 1 << 4;
    if (*dest == src)
      flags |= 1 << 7;
  }

  return true;
}

std::string ALU::getName() {
  switch (operation) {
  case 0b000:
    return "ADD";
  case 0b001:
    return "ADC";
  case 0b010:
    return "SUB";
  case 0b011:
    return "SBC";
  case 0b100:
    return "AND";
  case 0b101:
    return "XOR";
  case 0b110:
    return "OR";
  case 0b111:
    return "CP";
  }
  return "ALU";
}
int ALU::getType() { return instruction::ALU; }

Load::Load()
    : operation(0xFF), is16Bit(true), needsBytes(2), destination(0xFF),
      source(0xFF), address(0), loadedAddress(false) {
  finished = false;
  isMemoryLoad = false;
  immediate = 0;
}

Load::Load(uint8_t operation)
    : operation(operation), needsBytes(0), source(0xFF), address(0),
      loadedAddress(false) {
  finished = true;
  isMemoryLoad = false;
  immediate = 0;
}

Load::Load(uint8_t destination, bool is16Bit, int dummy)
    : operation(0xFF), needsBytes(is16Bit ? 2 : 1), is16Bit(is16Bit),
      destination(destination), source(0xFF), address(0), loadedAddress(false) {
  finished = false;
  isMemoryLoad = false;
  immediate = 0;
}

Load::Load(uint8_t destination, uint8_t source)
    : operation(0xFF), needsBytes(0), destination(destination), source(source),
      address(0), loadedAddress(false) {
  finished = true;
  isMemoryLoad = false;
  immediate = 0;
}

Load::Load(uint8_t direction, bool isC, bool is16Bit, bool actingOnSP)
    : operation(0xFF), destination(0xFF), source(0xFF), direction(direction),
      needsBytes(isC ? 0 : (is16Bit ? 2 : 1)), is16Bit(is16Bit), isC(isC),
      actingOnSP(actingOnSP), loadedAddress(false) {
  finished = needsBytes == 0;
  isMemoryLoad = true;
  immediate = 0;
  address = 0;
}

std::unique_ptr<Instruction> Load::decode(uint8_t opcode) {
  if (opcode == 0b00001000)
    return std::make_unique<Load>();
  if ((opcode >> 6) == 0 && (opcode & 0xF) == 1)
    return std::make_unique<Load>((opcode >> 4) & 0x3, true, 0);
  if ((opcode >> 6) == 0 && (opcode & 0x7) == 0b110)
    return std::make_unique<Load>((opcode >> 3) & 0x7, false, 0);
  if ((opcode >> 6) == 0b01 && opcode != 0b01110110)
    return std::make_unique<Load>((opcode >> 3) & 0x7, opcode & 0x7);
  if ((opcode >> 6) == 0b00 && (opcode & 0x7) == 0b010)
    return std::make_unique<Load>((opcode >> 3) & 0x7);
  if (opcode == 0b11111000)
    return std::make_unique<Load>(0, false, false, true);
  if (opcode == 0b11100000)
    return std::make_unique<Load>(0, false, false, false);
  if (opcode == 0b11110000)
    return std::make_unique<Load>(1, false, false, false);
  if (opcode == 0b11100010)
    return std::make_unique<Load>(0, true, false, false);
  if (opcode == 0b11110010)
    return std::make_unique<Load>(1, true, false, false);
  if (opcode == 0b11101010)
    return std::make_unique<Load>(0, false, true, false);
  if (opcode == 0b11111010)
    return std::make_unique<Load>(1, false, true, false);
  if (opcode == 0b11111001)
    return std::make_unique<Load>(1, false, false, true);

  return nullptr;
}

void Load::amend(uint8_t val) {
  needsBytes--;
  if (is16Bit)
    immediate = (immediate >> 8) | (val << 8);
  else
    immediate = val;
  finished = needsBytes == 0;
}

bool Load::execute(CPU *cpu) {
  if (isMemoryLoad) {
    if (isC) {
      if (!loadedAddress) {
        address = 0xFF00 + cpu->getRegisters().c;
        loadedAddress = true;
        return false;
      }

      if ((direction & 1) == 0) {
        cpu->write(address, cpu->getRegisters().a);
        LOG("WROTE %02X to %04X (0xFF00 + C)\n", cpu->getRegisters().a,
            address);
      } else {
        cpu->getRegisters().a = cpu->read(address);
        LOG("READ %02X from %04X (0xFF00 + C)\n", cpu->getRegisters().a,
            address);
      }
    } else if (actingOnSP) {
      if ((direction & 1) == 0) {
        cpu->getRegisters().hl = cpu->getRegisters().sp + (int8_t)immediate;
        LOG("Moved %02X to HL (SP + %i)", cpu->getRegisters().hl,
            (int8_t)immediate);
      } else {
        cpu->getRegisters().a = cpu->read(address);
        LOG("Moved %02X to SP from HL", cpu->getRegisters().sp);
      }
    } else {
      if (!loadedAddress) {
        if (is16Bit)
          address = immediate;
        else
          address = 0xFF00 + immediate;

        loadedAddress = true;
        return false;
      }

      if ((direction & 1) == 0) {
        cpu->write(address, cpu->getRegisters().a);
        LOG("WROTE %02X to %04X\n", cpu->getRegisters().a, address);
      } else {
        cpu->getRegisters().a = cpu->read(address);
        LOG("READ %02X from %04X\n", cpu->getRegisters().a, address);
      }
    }
  } else if (destination == 0xFF) {
    if (!loadedAddress) {
      address = immediate;
      loadedAddress = true;
      return false;
    }

    if (address == immediate) {
      cpu->write(address, immediate & 0xFF);
      LOG("Wrote %02X to %04X\n", cpu->getRegisters().sp & 0xFF, address);
      address += 1;
      return false;
    }
    cpu->write(address, (immediate >> 8) & 0xFF);
    LOG("Wrote %02X to %04X\n", (cpu->getRegisters().sp >> 8) & 0xFF, address);
  } else if (operation != 0xFF) {
    if (loadedAddress) {
      if ((operation & 1) == 0b0) {
        cpu->write(address, cpu->getRegisters().a);
        LOG("Wrote %02X from A to %04X\n", cpu->getRegisters().a, address);
      } else {
        cpu->getRegisters().a = cpu->read(address);
        LOG("Read %02X into A from %04X\n", cpu->getRegisters().a, address);
      }
    } else {
      if ((operation >> 2) == 1)
        address = cpu->getRegisters().hl;
      else
        address = ((operation >> 1) & 1) ? cpu->getRegisters().de
                                         : cpu->getRegisters().bc;

      loadedAddress = true;

      if ((operation >> 1) == 0b10) {
        cpu->getRegisters().hl++;
        LOG("Incremented HL\n");
      } else if ((operation >> 1) == 0b11) {
        cpu->getRegisters().hl--;
        LOG("Decremented HL\n");
      }
      return false;
    }
  } else if (source == 0xFF) {
    if (is16Bit) {
      if (destination == 0b00) {
        cpu->getRegisters().bc = immediate;
        LOG("Loaded 0x%04X into BC\n", immediate);
      } else if (destination == 0b01) {
        cpu->getRegisters().de = immediate;
        LOG("Loaded 0x%04X into DE\n", immediate);
      } else if (destination == 0b10) {
        cpu->getRegisters().hl = immediate;
        LOG("Loaded 0x%04X into HL\n", immediate);
      } else if (destination == 0b11) {
        cpu->getRegisters().sp = immediate;
        LOG("Loaded 0x%04X into SP\n", immediate);
      } else
        LOG("WRONG REG FIELD\n");
    } else {
      uint8_t *registerMapping[] = {&cpu->getRegisters().b,
                                    &cpu->getRegisters().c,
                                    &cpu->getRegisters().d,
                                    &cpu->getRegisters().e,
                                    &cpu->getRegisters().h,
                                    &cpu->getRegisters().l,
                                    nullptr,
                                    &cpu->getRegisters().a};

      std::string registerNames[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};

      uint8_t *dest = registerMapping[destination];
      if (loadedAddress) {
        cpu->write(address, immediate);
        LOG("Wrote %02X to %04X\n", immediate, address);
      } else if (dest == nullptr) {
        address = *dest;
        loadedAddress = true;
        LOG("Got address\n");
        return false;
      } else {
        *dest = immediate;
        LOG("Moved %02X to %s\n", immediate,
            registerNames[destination].c_str());
      }
    }
  } else {
    uint8_t *registerMapping[] = {&cpu->getRegisters().b,
                                  &cpu->getRegisters().c,
                                  &cpu->getRegisters().d,
                                  &cpu->getRegisters().e,
                                  &cpu->getRegisters().h,
                                  &cpu->getRegisters().l,
                                  nullptr,
                                  &cpu->getRegisters().a};

    std::string registerNames[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};

    uint8_t *dest = registerMapping[destination];
    uint8_t *src = registerMapping[source];

    if (loadedAddress) {
      if (src == nullptr) {
        *dest = cpu->read(address);
        LOG("Read 0x%02X into %s from %04X\n", *src,
            registerNames[destination].c_str(), address);
      } else {
        cpu->write(address, *src);
        LOG("Wrote 0x%02X from %s to %04X\n", *src,
            registerNames[source].c_str(), address);
      }
    } else if (dest == nullptr || src == nullptr) {
      address = cpu->getRegisters().hl;
      loadedAddress = true;
      LOG("Got address");
      return false;
    } else {
      *dest = *src;
      LOG("Moved 0x%02X from %s to %s\n", *src, registerNames[source].c_str(),
          registerNames[destination].c_str());
    }
  }

  return true;
}

std::string Load::getName() { return "Load"; }
int Load::getType() { return instruction::Load; }

Jump::Jump(bool isDisplacement, Condition condition)
    : needsBytes(isDisplacement ? 1 : 2), isDisplacement(isDisplacement),
      condition(condition), checkedCondition(false) {
  finished = false;
  addr = 0;
}

std::unique_ptr<Instruction> Jump::decode(uint8_t opcode) {
  Condition conditions[] = {Condition::NotZero, Condition::Zero,
                            Condition::NotCarry, Condition::Carry};

  if (opcode == 0b00011000)
    return std::make_unique<Jump>(true, Condition::Unconditional);
  if ((opcode >> 5) == 0b001 && (opcode & 0x7) == 0b000)
    return std::make_unique<Jump>(true, conditions[(opcode >> 3) & 0x3]);
  if (opcode == 0b11000011)
    return std::make_unique<Jump>(false, Condition::Unconditional);
  if ((opcode >> 5) == 0b110 && (opcode & 0x7) == 0b010)
    return std::make_unique<Jump>(false, conditions[(opcode >> 3) & 0x3]);

  return nullptr;
}

void Jump::amend(uint8_t val) {
  if (isDisplacement)
    addr = val;
  else
    addr = (val << 8) | (addr >> 8);
  needsBytes--;
  finished = needsBytes == 0;
}

bool Jump::execute(CPU *cpu) {
  uint8_t flags = cpu->getRegisters().f;
  if (checkedCondition || condition == Condition::Unconditional) {
    if (isDisplacement) {
      cpu->getRegisters().pc += (int8_t)addr;
      LOG("Jumped to %04X\n", cpu->getRegisters().pc);
    } else {
      cpu->getRegisters().pc = addr;
      LOG("Jumped to %04X\n", cpu->getRegisters().pc);
    }
  } else if (!checkedCondition) {
    checkedCondition = true;
    if (condition == Condition::NotZero &&
        (flags & (1 << 7)) == 0) { // Not Zero
      return false;
    }
    if (condition == Condition::Zero && (flags & (1 << 7)) != 0) { // Zero
      return false;
    }
    if (condition == Condition::NotCarry &&
        (flags & (1 << 4)) == 0) { // Not Carry
      return false;
    }
    if (condition == Condition::Carry && (flags & (1 << 4)) != 0) { // Carry
      return false;
    }
  }

  return true;
}

std::string Jump::getName() { return "Jump"; }
int Jump::getType() { return instruction::Jump; }

Nop::Nop() { finished = true; }

std::unique_ptr<Instruction> Nop::decode(uint8_t opcode) {
  if (opcode == 0)
    return std::make_unique<Nop>();

  return nullptr;
}

bool Nop::execute(CPU *cpu) { return true; };

std::string Nop::getName() { return "Nop"; }
int Nop::getType() { return instruction::Nop; }

namespace instruction {
std::unique_ptr<Instruction> decode(uint8_t opcode) {
  std::unique_ptr<Instruction> instr;

  DECODE(Nop);
  DECODE(IncDec);
  DECODE(RotateA);
  DECODE(Load);
  DECODE(ALU);
  DECODE(PopPush);
  DECODE(RST);
  DECODE(Ret);
  DECODE(Jump);
  DECODE(Call);
  DECODE(ExtendedInstruction);
  DECODE(Unsupported);

  return instr;
}
} // namespace instruction

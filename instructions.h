#pragma once

#include "gb.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace instruction {
enum : int { NoInstruction = 0, Nop, Load, ALU, Jump, RST, Unsupported };
}

enum Condition { Unconditional, NotZero, Zero, NotCarry, Carry };

class Instruction {
protected:
  bool finished = false;

public:
  virtual void amend(uint8_t val) { printf("Added value %02X\n", val); }
  virtual bool execute(CPU *cpu) {
    printf("Executed instruction\n");
    return false;
  }

  bool isFinished() { return finished; }

  virtual std::string getName() { return "BaseInstruction"; }
  virtual int getType() { return instruction::NoInstruction; }
};

namespace instruction {
std::unique_ptr<Instruction> decode(uint8_t opcode);
}

class RST : public Instruction {
  uint8_t n;
  uint8_t state;

public:
  RST(uint8_t n);

  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

class ALU : public Instruction {
  uint8_t operation;
  uint8_t destination;
  uint8_t source;
  uint16_t address;
  uint8_t immediate;
  bool takesImmediate;

public:
  ALU(bool takesImmediate, uint8_t operation, uint8_t source);

  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual void amend(uint8_t val) override;
  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

class Load : public Instruction {
  uint16_t immediate;
  bool is16Bit;
  uint8_t needsBytes;
  uint8_t destination;
  uint8_t source;
  uint16_t address;
  bool loadedAddress;
  uint8_t operation;

public:
  Load();
  Load(uint8_t operation);
  Load(uint8_t reg, bool is16Bit, int dummy);
  Load(uint8_t destination, uint8_t source);

  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual void amend(uint8_t val) override;
  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

class Jump : public Instruction {
  uint16_t addr;
  uint8_t needsBytes;
  Condition condition;

public:
  Jump(uint8_t needsBytes, Condition condition);

  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual void amend(uint8_t val) override;
  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

class Nop : public Instruction {
public:
  Nop();

  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

class Unsupported : public Instruction {
public:
  Unsupported() {}

  static std::unique_ptr<Instruction> decode(uint8_t opcode) {
    return std::make_unique<Unsupported>();
  }

  virtual std::string getName() override { return "Unsupported"; }
  virtual int getType() override { return instruction::Unsupported; }
};

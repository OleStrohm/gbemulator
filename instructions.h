#pragma once

#include "gb.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace instruction {
enum : int {
  NoInstruction = 0,
  Nop,
  SpecialAdd,
  Inc,
  Dec,
  RotateA,
  Stop,
  DAA,
  CPL,
  SetClearCarryFlag,
  Load,
  HALT,
  ALU,
  PopPush,
  Ret,
  Jump,
  Call,
  RST,
  EnableDisableInterrupts,
  CB,
  Unsupported
};
}

enum class Condition { Unconditional, NotZero, Zero, NotCarry, Carry };

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

class Halt : public Instruction {
public:
  Halt();

  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

class DAA : public Instruction {
public:
  DAA();

  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

class Complement : public Instruction {
public:
  Complement();

  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

class SetClearCarryFlag : public Instruction {
  bool shouldSet;

public:
  SetClearCarryFlag(bool shouldSet);

  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

class SpecialAdd : public Instruction {
  uint8_t reg;
  uint8_t immediate;
  uint8_t wastedCycles;

public:
  SpecialAdd(uint8_t reg);

  virtual void amend(uint8_t val) override;
  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

class EnableDisableInterrupts : public Instruction {
  bool enable;

public:
  EnableDisableInterrupts(bool enable);

  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

class RotateA : public Instruction {
  bool isLeft;
  bool throughCarry;

public:
  RotateA(bool isLeft, bool throughCarry);

  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

class PopPush : public Instruction {
  uint8_t reg;
  bool isPop;
  uint8_t currentByte;
  bool hasWastedCycle;

public:
  PopPush(uint8_t reg, bool isPop);

  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

class Ret : public Instruction {
  uint8_t currentByte;
  uint16_t address;
  Condition condition;
  bool checkedCondition;
  bool enableInterrupts;
  bool wastedCycle;

public:
  Ret(Condition condition, bool enableInterrupts);

  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

class Call : public Instruction {
  uint8_t needsBytes;
  uint16_t address;
  Condition condition;
  uint8_t storedPCCount;
  bool movedSP;

public:
  Call(Condition condition);

  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual void amend(uint8_t val) override;
  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

class IncDec : public Instruction {
  bool increment;
  uint8_t destination;
  bool isBigRegister;
  uint16_t address;
  bool gotAddress;
  uint8_t storedValue;
  bool retrievedStoredValue;

public:
  IncDec(bool increment, uint8_t destination, bool isBigRegister);

  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

class ExtendedInstruction : public Instruction {
  uint8_t instruction;
  uint16_t valueAtHL;
  bool gotValueAtHL;
  uint8_t result;
  bool shouldWrite;

public:
  ExtendedInstruction();

  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual void amend(uint8_t val) override;
  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

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
  bool isMemoryLoad;
  uint8_t direction;
  bool isC;
  bool actingOnSP;
  bool writesSP;
  uint8_t currentByte;

public:
  Load();
  Load(uint8_t operation);
  Load(uint8_t reg, bool is16Bit, int dummy);
  Load(uint8_t destination, uint8_t source);
  Load(uint8_t direction, bool isC, bool is16Bit, bool actingOnSP);

  static std::unique_ptr<Instruction> decode(uint8_t opcode);

  virtual void amend(uint8_t val) override;
  virtual bool execute(CPU *cpu) override;

  virtual std::string getName() override;
  virtual int getType() override;
};

class Jump : public Instruction {
  uint16_t addr;
  uint8_t needsBytes;
  bool isDisplacement;
  Condition condition;
  bool checkedCondition;
  bool jumpsToHL;

public:
  Jump();
  Jump(bool isDisplacement, Condition condition);

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

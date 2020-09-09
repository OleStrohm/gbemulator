#include "bus.h"

#include "gb.h"
#include "ppu.h"

void Bus::connectCPU(CPU *cpu) { this->cpu = cpu; }
void Bus::connectPPU(PPU *ppu) { this->ppu = ppu; }

void Bus::loadCartridge(std::vector<uint8_t> cartridge) {
  this->cartridge = cartridge;
  this->ramBank.resize(0x2000);
  switch (cartridge[0x0147]) {
  case 0: {
    printf("Rom only\n");
    break;
  }
  case 0x1: {
    printf("MBC1\n");
    break;
  }
  case 0x2: {
    printf("MBC1+RAM\n");
    break;
  }
  case 0x3: {
    printf("MBC1+RAM+BATTERY\n");
    break;
  }
  case 0x5: {
    printf("MBC2\n");
    break;
  }
  case 0x6: {
    printf("MBC2+BATTERY\n");
    break;
  }
  case 0x7: {
    printf("ROM+RAM\n");
    break;
  }
  default: {
    printf("Unknown\n");
    break;
  }
  }
  switch (cartridge[0x0149]) {
  case 0: {
    this->ram.resize(0);
    break;
  }
  case 1: {
    printf("Ram size: 0x800\n");
    this->ram.resize(0x800);
    break;
  }
  case 2: {
    printf("Ram size: 0x2000\n");
    this->ram.resize(0x2000);
    break;
  }
  case 3: {
    printf("Ram size: 0x4000\n");
    this->ram.resize(0x4000);
    break;
  }
  default: {
    printf("Ram size: Unsupported\n");
    break;
  }
  }
}

void Bus::raiseInterrupt(int interrupt) { cpu->raiseInterrupt(interrupt); }

uint8_t Bus::read(uint16_t addr) {
  if (addr < 0x4000) {
    return cartridge[addr];
  } else if (addr < 0x8000) {
    return cartridge[cartridgeBankAddress + addr - 0x4000];
  } else if (addr >= 0x8000 && addr < 0xA000) {
    return ppu->read(addr);
  } else if (addr >= 0xA000 && addr < 0xC000) {
    return ram[addr - 0xA000];
  } else if (addr >= 0xC000 && addr < 0xD000) {
    return ramBank[addr - 0xC000];
  } else if (addr >= 0xD000 && addr < 0xE000) {
    return ramBank[addr - 0xC000];
  } else if (addr >= 0xE000 && addr < 0xFE00) {
    return ram[addr - 0xE000];
  } else if (addr >= 0xFE00 && addr < 0xFEA0) {
    return ppu->read(addr);
  } else if (addr >= 0xFF00 && addr < 0xFF4C) {
    if (addr >= 0xFF40 && addr <= 0xFF4B)
      return ppu->read(addr);
  }

  fprintf(stderr, "Bus read at bad address: %04X\n", addr);
  return 0xFF;
}

void Bus::write(uint16_t addr, uint8_t value) {
  if (addr < 0x8000) {
    if (cartridge[0x147] >= 1 && cartridge[0x147] <= 3) {
      if (addr >= 0x6000 && addr < 0x8000) {
        if (value & 1)
          printf("Set MBC1 to 4/32\n");
        else
          printf("Set MBC1 to 16/8\n");
      } else if (addr >= 0x2000 && addr < 0x4000) {
        cartridgeBankAddress = std::max(0x4000 * (value & 0x1F), 0x4000);
      } else
        fprintf(stderr, "Wrote %02X to cartridge rom %04X!\n", value, addr);
    } else
      fprintf(stderr, "Wrote %02X to cartridge rom %04X!\n", value, addr);
  } else if (addr >= 0x8000 && addr < 0xA000) {
    ppu->write(addr, value);
  } else if (addr >= 0xA000 && addr < 0xC000) {
    ram[addr - 0xA000] = value;
  } else if (addr >= 0xC000 && addr < 0xD000) {
    ramBank[addr - 0xC000] = value;
  } else if (addr >= 0xD000 && addr < 0xE000) {
    ramBank[addr - 0xC000] = value;
  } else if (addr >= 0xE000 && addr < 0xFE00) {
    ram[addr - 0xE000] = value;
  } else if (addr >= 0xFE00 && addr < 0xFEA0) {
    ppu->write(addr, value);
  } else if (addr >= 0xFF00 && addr < 0xFF4C) {
    if (addr >= 0xFF40 && addr <= 0xFF4B)
      ppu->write(addr, value);
    else
      fprintf(stderr, "WRITE TO UNCONNECTED IO at %04X\n", addr);
  } else
    fprintf(stderr, "Bus written to at bad address: %04X\n", addr);
}

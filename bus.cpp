#include "bus.h"

#include "gb.h"
#include "ppu.h"

void Bus::connectCPU(CPU *cpu) { this->cpu = cpu; }
void Bus::connectPPU(PPU *ppu) { this->ppu = ppu; }

void Bus::loadCartridge(std::vector<uint8_t> cartridge) {
  this->cartridge = cartridge;
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
        printf("Set MBC1 Bank address to %02lX\n", cartridgeBankAddress);
      } else
        fprintf(stderr, "Wrote to cartridge rom!");
    } else
      fprintf(stderr, "Wrote to cartridge rom!");
  } else if (addr >= 0x8000 && addr < 0xA000) {
    ppu->write(addr, value);
  } else if (addr >= 0xA000 && addr < 0xC000) {
    ram[addr - 0xA000] = value;
  } else if (addr >= 0xE000 && addr < 0xFE00) {
    ram[addr - 0xE000] = value;
  } else if (addr >= 0xFE00 && addr < 0xFEA0) {
    ppu->write(addr, value);
  } else if (addr >= 0xFF00 && addr < 0xFF4C) {
    if (addr >= 0xFF40 && addr <= 0xFF4B)
      ppu->write(addr, value);
  } else
    fprintf(stderr, "Bus written to at bad address: %04X\n", addr);
}

#include "bus.h"

Bus::Bus(PPU *ppu) : ppu(ppu) {}

void Bus::loadCartridge(std::vector<uint8_t> cartridge) {
  this->cartridge = cartridge;
}

uint8_t Bus::read(uint16_t addr) {
  if (addr < 0x8000) {
    return cartridge[addr];
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

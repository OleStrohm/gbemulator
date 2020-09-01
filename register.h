#pragma once 

#include <cstdint>

struct RegisterBank {
	union {
		uint16_t af = 0;
		struct {
			uint8_t f;
			uint8_t a;
		};
	};
	union {
		uint16_t bc = 0;
		struct {
			uint8_t c;
			uint8_t b;
		};
	};
	union {
		uint16_t de = 0;
		struct {
			uint8_t e;
			uint8_t d;
		};
	};
	union {
		uint16_t hl = 0;
		struct {
			uint8_t l;
			uint8_t h;
		};
	};
	uint16_t sp = 0;
	uint16_t pc = 0;
};

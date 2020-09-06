SOURCE = gb.cpp instructions.cpp utils.cpp

debug: $(SOURCE) gb.h instructions.h register.h utils.h bus.h
	mkdir -p build/debug
	g++ -g -std=c++17 -o ./build/debug/gameboy $(SOURCE)

release: $(SOURCE) gb.h instructions.h register.h utils.h bus.h
	mkdir -p build/release
	g++ -O3 -std=c++17 -o ./build/release/gameboy $(SOURCE)

validate01: debug
	./build/debug/gameboy "../gb-test-roms/cpu_instrs/individual/01-special.gb" > /dev/null
validate02: debug
	./build/debug/gameboy "../gb-test-roms/cpu_instrs/individual/02-interrupts.gb" > /dev/null
validate03: debug
	./build/debug/gameboy "../gb-test-roms/cpu_instrs/individual/03-op sp,hl.gb" > /dev/null
validate04: debug
	./build/debug/gameboy "../gb-test-roms/cpu_instrs/individual/04-op r,imm.gb" > /dev/null
validate05: debug
	./build/debug/gameboy "../gb-test-roms/cpu_instrs/individual/05-op rp.gb" > /dev/null
validate06: debug
	./build/debug/gameboy "../gb-test-roms/cpu_instrs/individual/06-ld r,r.gb" > /dev/null
validate07: debug
	./build/debug/gameboy "../gb-test-roms/cpu_instrs/individual/07-jr,jp,call,ret,rst.gb" > /dev/null
validate08: debug
	./build/debug/gameboy "../gb-test-roms/cpu_instrs/individual/08-misc instrs.gb" > /dev/null
validate09: debug
	./build/debug/gameboy "../gb-test-roms/cpu_instrs/individual/09-op r,r.gb" > /dev/null
validate10: debug
	./build/debug/gameboy "../gb-test-roms/cpu_instrs/individual/10-bit ops.gb" > /dev/null
validate11: debug
	./build/debug/gameboy "../gb-test-roms/cpu_instrs/individual/11-op a,(hl).gb" > /dev/null

clean:
	rm -r build

.PHONY: all test validate clean



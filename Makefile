SOURCE = gb.cpp instructions.cpp utils.cpp

debug: $(SOURCE) gb.h instructions.h register.h utils.h bus.h
	mkdir -p build/debug
	g++ -g -std=c++17 -o ./build/debug/gameboy $(SOURCE)

release: $(SOURCE) gb.h instructions.h register.h utils.h bus.h
	mkdir -p build/release
	g++ -O3 -std=c++17 -o ./build/release/gameboy $(SOURCE)

test: gb
	./build/debug/gameboy "../gb-test-roms/cpu_instrs/individual/04-op r,imm.gb"

clean:
	rm -r build

.PHONY: all test clean



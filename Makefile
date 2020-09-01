SOURCE = gb.cpp instructions.cpp

gb: $(SOURCE) gb.h instructions.h register.h utils.h bus.h
	g++ -std=c++17 -o ./build/gameboy $(SOURCE)

test: gb
	./build/gameboy ../gb-test-roms/cpu_instrs/individual/01-special.gb

clean:
	rm -r build

.PHONY: all test clean



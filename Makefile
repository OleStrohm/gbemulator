GBSOURCE = gb.cpp ppu.cpp bus.cpp instructions.cpp utils.cpp
IMGUISOURCE = deps/imgui/imgui.cpp deps/imgui/imgui_draw.cpp deps/imgui/imgui_widgets.cpp deps/imgui/imgui_demo.cpp imgui/imgui_impl_glfw.cpp imgui/imgui_impl_opengl3.cpp
CPPFLAGS = -std=c++17 -Ideps
LDFLAGS = `pkg-config --static --libs glfw3` -lGLEW -lGL

SOURCE = $(GBSOURCE) $(IMGUISOURCE)

all: gb

gb: $(SOURCE) gb.h instructions.h register.h utils.h bus.h
	mkdir -p build/debug
	g++ $(CPPFLAGS) -o ./build/debug/gameboy $(SOURCE) $(LDFLAGS)

test: gb
	# ./build/debug/gameboy ../tetris.gb
	./build/debug/gameboy ../zelda.gb
	# ./build/debug/gameboy ../gb-test-roms/mem_timing/individual/01-read_timing.gb

debug: $(SOURCE) gb.h instructions.h register.h utils.h bus.h
	mkdir -p build/debug
	g++ -g $(CPPFLAGS) -o ./build/debug/gameboy $(SOURCE) $(LDFLAGS)

release: $(SOURCE) gb.h instructions.h register.h utils.h bus.h
	mkdir -p build/release
	g++ -O3 $(CPPFLAGS) -o ./build/release/gameboy $(SOURCE) $(LDFLAGS)

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



GBSOURCE = gb.cpp ppu.cpp bus.cpp instructions.cpp utils.cpp
IMGUISOURCE = deps/imgui/imgui.cpp deps/imgui/imgui_draw.cpp deps/imgui/imgui_widgets.cpp deps/imgui/imgui_demo.cpp imgui/imgui_impl_glfw.cpp imgui/imgui_impl_opengl3.cpp
CPPFLAGS = -std=c++17 -Ideps -DIMGUI_IMPL_OPENGL_LOADER_GLEW
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

validate_cpu: gb
	./build/debug/gameboy "../gb-test-roms/cpu_instrs/cpu_instrs.gb" > /dev/null

clean:
	rm -r build

.PHONY: all test validate clean



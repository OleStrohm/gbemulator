#include "ppu.h"
#include "utils.h"
#include <GLFW/glfw3.h>
#include <chrono>
#include <iostream>
#include <iterator>
#include <thread>
#include <vector>

void error_callback(int error, const char *description) {
  fprintf(stderr, "ERROR(%i): %s\n", error, description);
}

std::string vertexShaderSource = R"EOF(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 texCoords;

out vec2 fTexCoords;

void main()
{
	gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
	fTexCoords = texCoords;
}
)EOF";

std::string fragmentShaderSource = R"EOF(
#version 330 core
out vec4 FragColor;

in vec2 fTexCoords;

uniform sampler2D uTexture;

void main()
{
	FragColor = texture(uTexture, fTexCoords);
}
)EOF";

constexpr float vertices[] = {
    -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 1.0f,
    1.0f,  1.0f,  0.0f, 1.0f, 0.0f, 1.0f,  1.0f,  0.0f, 1.0f, 0.0f,
    -1.0f, 1.0f,  0.0f, 0.0f, 0.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
};

PPU::PPU() : vram(0x2000), hasSetUp(false), frame(0) {
  uint8_t firstPart[] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xfc, 0x00, 0xfc, 0x00,
      0xfc, 0x00, 0xfc, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0x3c, 0x00, 0x3c, 0x00,
      0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00,
      0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00,
      0xf3, 0x00, 0xf3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0xcf, 0x00, 0xcf, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x0f, 0x00, 0x0f, 0x00, 0x3f, 0x00, 0x3f, 0x00, 0x0f, 0x00, 0x0f, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0xc0, 0x00,
      0x0f, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf3, 0x00, 0xf3, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0xc0, 0x00, 0xc0, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00,
      0x03, 0x00, 0x03, 0x00, 0xff, 0x00, 0xff, 0x00, 0xc0, 0x00, 0xc0, 0x00,
      0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc3, 0x00, 0xc3, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0xfc, 0x00, 0xfc, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0xf0, 0x00, 0xf0, 0x00,
      0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0x3c, 0x00, 0x3c, 0x00,
      0xfc, 0x00, 0xfc, 0x00, 0xfc, 0x00, 0xfc, 0x00, 0x3c, 0x00, 0x3c, 0x00,
      0xf3, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0xf3, 0x00,
      0xf3, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0xc3, 0x00, 0xc3, 0x00,
      0xc3, 0x00, 0xc3, 0x00, 0xc3, 0x00, 0xc3, 0x00, 0xcf, 0x00, 0xcf, 0x00,
      0xcf, 0x00, 0xcf, 0x00, 0xcf, 0x00, 0xcf, 0x00, 0xcf, 0x00, 0xcf, 0x00,
      0x3c, 0x00, 0x3c, 0x00, 0x3f, 0x00, 0x3f, 0x00, 0x3c, 0x00, 0x3c, 0x00,
      0x0f, 0x00, 0x0f, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0xfc, 0x00, 0xfc, 0x00,
      0x00, 0x00, 0x00, 0x00, 0xfc, 0x00, 0xfc, 0x00, 0xfc, 0x00, 0xfc, 0x00,
      0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00,
      0xf3, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0xf3, 0x00,
      0xf0, 0x00, 0xf0, 0x00, 0xc3, 0x00, 0xc3, 0x00, 0xc3, 0x00, 0xc3, 0x00,
      0xc3, 0x00, 0xc3, 0x00, 0xff, 0x00, 0xff, 0x00, 0xcf, 0x00, 0xcf, 0x00,
      0xcf, 0x00, 0xcf, 0x00, 0xcf, 0x00, 0xcf, 0x00, 0xc3, 0x00, 0xc3, 0x00,
      0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00,
      0xfc, 0x00, 0xfc, 0x00, 0x3c, 0x00, 0x42, 0x00, 0xb9, 0x00, 0xa5, 0x00,
      0xb9, 0x00, 0xa5, 0x00, 0x42, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  // 00001900:
  uint8_t secondPart[] = {
      0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
      0x15, 0x16, 0x17, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  for (int i = 0; i < sizeof(firstPart) / sizeof(uint8_t); i++)
    vram[i] = firstPart[i];
  for (int i = 0; i < sizeof(secondPart) / sizeof(uint8_t); i++)
    vram[i + 0x1900] = secondPart[i];

  util::hexdump(vram, vram.size(), 0x8000);
}

void PPU::invalidate() { invalidated = true; }

void PPU::close() { hasClosed = true; }

bool PPU::isClosed() { return hasClosed; }

void PPU::setup() {
  glfwSetErrorCallback(error_callback);

  if (!glfwInit()) {
    std::cout << "ERROR: Could not initialize GLFW" << std::endl;
    glfwTerminate();
    exit(1);
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  window = glfwCreateWindow(WIDTH * SCALE, HEIGHT * SCALE, "Gameboy Emulator",
                            NULL, NULL);
  if (!window) {
    std::cout << "ERROR: Could not initialize GLFW" << std::endl;
    glfwTerminate();
    exit(1);
  }

  glfwMakeContextCurrent(window);

  if (glewInit() != GLEW_OK) {
    std::cout << "ERROR: Could not initialize GLEW" << std::endl;
    glfwTerminate();
    exit(1);
  }

  unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
  const GLchar *vertexSource = vertexShaderSource.c_str();
  glShaderSource(vertexShader, 1, &vertexSource, NULL);
  glCompileShader(vertexShader);

  int success;
  char infoLog[512];
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
              << infoLog << std::endl;
    exit(1);
  }

  unsigned int fragmentShader;
  fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  const GLchar *fragmentSource = fragmentShaderSource.c_str();
  glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
  glCompileShader(fragmentShader);

  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
              << infoLog << std::endl;
    exit(1);
  }

  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
              << infoLog << std::endl;
    exit(1);
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  glGenTextures(1, &screenTexture);
  glBindTexture(GL_TEXTURE_2D, screenTexture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB,
               GL_UNSIGNED_BYTE, 0);

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  glGenBuffers(1, &pbo);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);

  glBufferData(GL_PIXEL_UNPACK_BUFFER, WIDTH * HEIGHT * BYTES_PER_PIXEL, 0,
               GL_STATIC_DRAW);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  glViewport(0, 0, WIDTH * SCALE, HEIGHT * SCALE);

  pixelData = (GLubyte *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

  hasSetUp = true;
}

void PPU::step() {
  if (!hasSetUp)
    return;
  bool isLCDOn = LCDC & (1 << 7);

  uint16_t windowTileMap = LCDC & (1 << 6) ? 0x9C00 : 0x9800;
  uint16_t tileDataBase = LCDC & (1 << 4) ? 0x9000 : 0x8000;
  uint16_t bgTileMapDisplay = LCDC & (1 << 3) ? 0x9C00 : 0x9800;
  uint8_t spriteHeight = LCDC & (1 << 2) ? 16 : 8;
  bool showSprites = LCDC & (1 << 1);
  bool showBGAndWindow = LCDC & 1;

  if (LY < 144) {
    if (LX < 20) {
    } else if (LX < 63) {
    } else {
      if (LX == 63) {
        uint8_t yy = LY + SCY;
        int yt = yy / 8;
		mtx.lock();
        for (int x = 0; x < 20 * 8; x++) {
          uint8_t xx = x + SCX;
          uint8_t xt = xx / 8;
          int tile = vram[0x1800 + yt * 32 + xt];
          int color = getColorForTile(tile, xx % 8, yy % 8) * 0xFF / 3;

          int ri = 3 * (WIDTH * LY + x);
          int gi = 3 * (WIDTH * LY + x) + 1;
          int bi = 3 * (WIDTH * LY + x) + 2;

          pixelData[ri] = color;
          pixelData[gi] = color;
          pixelData[bi] = color;
        }
		mtx.unlock();
      }
    }
  } else {
    if (LY == 144 && LX == 0) {
      invalidate();
    }
  }

  LX++;
  if (LX >= 114) {
    LX = 0;
    LY++;
    if (LY >= 154) {
      frame++;
      // printf("Drew frame: %d\n", frame);
      LY = 0;
    }
  }
}

void PPU::render() {
  bool keyDown = false;

  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);

    if (invalidated) {
      invalidated = false;

      mtx.lock();
      glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB,
                   GL_UNSIGNED_BYTE, 0);
      pixelData = (GLubyte *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	  mtx.unlock();
      if (!pixelData) {
        std::cerr << "Couldn't Map Pixel Buffer!" << std::endl;
        GLenum err = glGetError();
        std::cerr << "OpenGL Error: " << err << std::endl;
      }
    }

    glUseProgram(shaderProgram);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

int8_t PPU::getColorForTile(uint8_t index, uint8_t x, uint8_t y) {
  return ((vram[0x10 * index + 2 * y] >> (7 - x)) & 0x1) |
         (((vram[0x10 * index + 2 * y + 1] >> (7 - x)) & 0x1) << 1);
}

void PPU::cleanup() {
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteProgram(shaderProgram);

  glfwDestroyWindow(window);
  glfwTerminate();
}

void startRenderLoop(PPU *ppu) {
  ppu->setup();
  ppu->render();
  ppu->close();
  ppu->cleanup();

  std::cout << "Please close this..." << std::endl;
}

int main() {
  PPU ppu;

  ppu.setLCDC(0x91);

  using cycles = std::chrono::duration<double, std::ratio<4, 4'194'304>>;

  auto lastTime = std::chrono::high_resolution_clock::now();
  auto fpsCounter = std::chrono::high_resolution_clock::now();

  int cyclesPS = 0;

  // std::thread inputThread(startInputLoop, &ppu);
  std::thread th(startRenderLoop, &ppu);

  while (!ppu.isClosed()) {
    auto now = std::chrono::high_resolution_clock::now();
    auto cycleCount =
        std::chrono::duration_cast<cycles>(now - lastTime).count();
    auto fpsElapsed =
        std::chrono::duration_cast<std::chrono::seconds>(now - fpsCounter)
            .count();

    while (cycleCount > 1.0) {
      lastTime =
          lastTime +
          std::chrono::duration_cast<std::chrono::nanoseconds>(cycles(1.0));
      cyclesPS++;
      cycleCount -= 1.0;
      ppu.step();
    }

    if (fpsElapsed > 1.0) {
      fpsCounter += std::chrono::seconds(1);
      printf("FPS: %d\n", ppu.getFrame());
      printf("CYCLES: %d\n", cyclesPS);
      ppu.setFrame(0);
      cyclesPS = 0;
    }

    if (ppu.getLY() == 0 && ppu.getLX() == 0)
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  th.join();

  return 0;
}

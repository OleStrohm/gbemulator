#include "ppu.h"
#include "utils.h"
#include <GLFW/glfw3.h>
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

PPU::PPU() : vram(0x2000) {
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
}

void PPU::step() {

  bool isLCDOn = LCDC & (1 << 7);

  uint16_t windowTileMap = LCDC & (1 << 6) ? 0x9C00 : 0x9800;
  uint16_t tileDataBase = LCDC & (1 << 4) ? 0x9000 : 0x8000;
  uint16_t bgTileMapDisplay = LCDC & (1 << 3) ? 0x9C00 : 0x9800;
  uint8_t spriteHeight = LCDC & (1 << 2) ? 16 : 8;
  bool showSprites = LCDC & (1 << 1);
  bool showBGAndWindow = LCDC & 1;

  if (LY < 144) {
    if (LX < 20) {
      printf("OAM SEARCH STUB");
    } else if (LX < 63) {
      printf("Moving pixel data LX=%i LY=%i", LX, LY);

    } else {
      printf("HBLANK");
    }
  } else {
    printf("VBLANK");
  }

  LX++;
  if (LX >= 114) {
    LX = 0;
    LY++;
    if (LY >= 154) {
      invalidate();
      LY = 0;
    }
  }
}

void PPU::renderScreen() {
  for (int y = 18 * 8 - 1; y >= 0; y--) {
    uint8_t yy = SCY + y;
    int yt = (yy % 256) / 8;
    for (int x = 0; x < 20 * 8; x++) {
      uint8_t xx = x + SCX;
      int xt = (xx % 256) / 8;
      int tile = vram[0x1600 + (31 - yt) * 32 + xt];
      int color = getColorForTile(tile, x % 8, (18 * 8 - 1 - y) % 8) * 0xFF / 3;

      int r = 3 * (WIDTH * (18 * 8 - 1 - y) + x);
      int g = 3 * (WIDTH * (18 * 8 - 1 - y) + x) + 1;
      int b = 3 * (WIDTH * (18 * 8 - 1 - y) + x) + 2;

      pixelData[r] = color;
      pixelData[g] = color;
      pixelData[b] = color;
    }
  }
}

void PPU::render() {
  bool keyDown = false;
  bool changePixels = false;

  SCX = 0;
  SCY = 255;

  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);

    if (invalidated) {
      invalidated = false;

      glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB,
                   GL_UNSIGNED_BYTE, 0);
      pixelData = (GLubyte *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
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

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_UP) == GLFW_RELEASE &&
        glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_RELEASE)
      keyDown = false;
    if (!keyDown && glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
      SCX--;
	  renderScreen();
      invalidate();
      keyDown = true;
    }
    if (!keyDown && glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
      SCX++;
	  renderScreen();
      invalidate();
      keyDown = true;
    }
    if (!keyDown && glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
      SCY++;
	  renderScreen();
      invalidate();
      keyDown = true;
    }
    if (!keyDown && glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
      SCY--;
	  renderScreen();
      invalidate();
      keyDown = true;
    }
    if (!keyDown && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
      // int tile = 0x19;
      // drawTile(0x1, 0, 0);
      // drawTile(0x2, 8, 0);
      // drawTile(0x0d, 0, 8);
      // drawTile(0x0e, 8, 8);

      // drawTile(0x1, 144, 128);
      // drawTile(0x2, 152, 128);
      // drawTile(0x0d, 144, 136);
      // drawTile(0x0e, 152, 136);
      // for (int y = 0; y < 18; y++) {
      //  int yp = 8 * y + scy;
      //  int yt = (yp / 8) % 256;
      //  for (int x = 0; x < 20; x++) {
      //    int xp = 8 * x + scx;
      //    int xt = (xp / 8) % 256;
      //    int tile = vram[0x1600 + yt * 32 + xt];
      //    drawTile(tile, xp, yp);
      //    printf("%02X ", tile);
      //  }
      //  printf("\n");
      //}
	  renderScreen();
      invalidate();
      keyDown = true;
    }
  }
}

int8_t PPU::getColorForTile(uint8_t index, uint8_t x, uint8_t y) {
  return ((vram[0x10 * index + 2 * y] >> (7 - x)) & 0x1) |
         (((vram[0x10 * index + 2 * y + 1] >> (7 - x)) & 0x1) << 1);
}

void PPU::drawTile(uint8_t tile, uint8_t x, uint8_t y) {
  for (int ty = 0; ty < 8; ty++) {
    int yy = (y + ty) * WIDTH;
    for (int tx = 0; tx < 8; tx++) {
      int r = 3 * (yy + x + tx);
      int g = 3 * (yy + x + tx) + 1;
      int b = 3 * (yy + x + tx) + 2;
      int color = getColorForTile(tile, tx, ty);

      color = color * 0xFF / 3;

      pixelData[r] = color;
      pixelData[g] = color;
      pixelData[b] = color;
    }
  }
}

void PPU::cleanup() {
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteProgram(shaderProgram);

  glfwDestroyWindow(window);
  glfwTerminate();
}

void startInputLoop(PPU *ppu) {
  while (!ppu->isClosed()) {
    std::string input;
    getline(std::cin, input);
    if (ppu->isClosed())
      return;
    if (input == "r") {
      ppu->invalidate();
      std::cout << "Invalidated" << std::endl;
    } else {
      ppu->step();
    }
  }
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

  // std::thread inputThread(startInputLoop, &ppu);
  std::thread th(startRenderLoop, &ppu);

  startInputLoop(&ppu);

  th.join();

  return 0;
}

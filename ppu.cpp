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

void PPU::render() {
  bool keyDown = false;
  bool changePixels = false;

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

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
      keyDown = false;
    if (!keyDown && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
      invalidate();
      keyDown = true;
    }
  }
}

std::vector<uint8_t> vram(0x2000);

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
      for (int l = 0; l < 2; l++) {
        for (int t = 1; t <= 12 + l; t++) {
          for (int i = 0; i < 8; i++) {
            int yy = (i + l * 8) * WIDTH;
            for (int x = 0; x < 8; x++) {
              int r = 3 * (yy + x + 8 * t);
              int g = 3 * (yy + x + 8 * t) + 1;
              int b = 3 * (yy + x + 8 * t) + 2;
              int bit0 =
                  (vram[0x0010 * (t + 12 * l) + (2 * i)] >> (7 - x)) & 0x1;
              int bit1 =
                  (vram[0x0010 * (t + 12 * l) + 1 + (2 * i)] >> (7 - x)) & 0x1;
              int color = (bit1 << 1) | bit0;

              printf("%i%i ", bit1, bit0);

              color = color * 0xFF / 3;

              ppu->pixelData[r] = color;
              ppu->pixelData[g] = color;
              ppu->pixelData[b] = color;
            }
            printf("\n");
          }
        }
      }

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

  ppu.setLCDC(0x91);

  // std::thread inputThread(startInputLoop, &ppu);
  std::thread th(startRenderLoop, &ppu);

  startInputLoop(&ppu);

  th.join();

  return 0;
}

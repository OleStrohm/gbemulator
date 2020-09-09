#include "ppu.h"
#include "utils.h"
#include <GLFW/glfw3.h>
#include <bits/stdint-uintn.h>
#include <chrono>
#include <cstring>
#include <iostream>
#include <iterator>
#include <new>
#include <thread>
#include <vector>

constexpr uint8_t interruptVblank = 1 << 0;
constexpr uint8_t interruptLCDC = 1 << 1;
constexpr uint8_t interruptInput = 1 << 4;

struct Sprite {
  uint8_t x;
  uint8_t y;
  uint8_t tile;
  uint8_t attributes;

  Sprite(uint8_t x, uint8_t y, uint8_t tile, uint8_t attributes)
      : x(x), y(y), tile(tile), attributes(attributes) {}
};

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

PPU::PPU(Bus *bus)
    : bus(bus), vram(0x2000), oam(0xA0), hasSetUp(false), hasClosed(false),
      frame(0), LY(0), LX(0), LYC(0), LCDC(0), BGP(0), WY(0), WX(0) {
  //  uint8_t firstPart[] = {
  //      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  //      0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xfc, 0x00,
  //      0xfc, 0x00, 0xfc, 0x00, 0xfc, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0x3c,
  //      0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3c, 0x00,
  //      0x3c, 0x00, 0x3c, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0,
  //      0x00, 0x00, 0x00, 0x00, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0x00, 0x00,
  //      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xcf,
  //      0x00, 0xcf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x0f, 0x00,
  //      0x3f, 0x00, 0x3f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00,
  //      0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0x0f, 0x00,
  //      0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  //      0x00, 0x00, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00,
  //      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf3, 0x00, 0xf3,
  //      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  //      0x00, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03,
  //      0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0xff, 0x00, 0xff, 0x00,
  //      0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0, 0x00, 0xc0,
  //      0x00, 0xc3, 0x00, 0xc3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  //      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x00, 0xfc, 0x00, 0xf3,
  //      0x00, 0xf3, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00,
  //      0xf0, 0x00, 0xf0, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0xfc, 0x00, 0xfc,
  //      0x00, 0xfc, 0x00, 0xfc, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0xf3, 0x00,
  //      0xf3, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0xf3,
  //      0x00, 0xf3, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0xc3, 0x00, 0xc3, 0x00,
  //      0xc3, 0x00, 0xc3, 0x00, 0xc3, 0x00, 0xc3, 0x00, 0xcf, 0x00, 0xcf,
  //      0x00, 0xcf, 0x00, 0xcf, 0x00, 0xcf, 0x00, 0xcf, 0x00, 0xcf, 0x00,
  //      0xcf, 0x00, 0x3c, 0x00, 0x3c, 0x00, 0x3f, 0x00, 0x3f, 0x00, 0x3c,
  //      0x00, 0x3c, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x3c, 0x00, 0x3c, 0x00,
  //      0xfc, 0x00, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x00, 0xfc,
  //      0x00, 0xfc, 0x00, 0xfc, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00,
  //      0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0xf3,
  //      0x00, 0xf3, 0x00, 0xf3, 0x00, 0xf3, 0x00, 0xf0, 0x00, 0xf0, 0x00,
  //      0xc3, 0x00, 0xc3, 0x00, 0xc3, 0x00, 0xc3, 0x00, 0xc3, 0x00, 0xc3,
  //      0x00, 0xff, 0x00, 0xff, 0x00, 0xcf, 0x00, 0xcf, 0x00, 0xcf, 0x00,
  //      0xcf, 0x00, 0xcf, 0x00, 0xcf, 0x00, 0xc3, 0x00, 0xc3, 0x00, 0x0f,
  //      0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00,
  //      0xfc, 0x00, 0xfc, 0x00, 0x3c, 0x00, 0x42, 0x00, 0xb9, 0x00, 0xa5,
  //      0x00, 0xb9, 0x00, 0xa5, 0x00, 0x42, 0x00, 0x3c, 0x00, 0x00, 0x00,
  //      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  //      0x00, 0x00, 0x00};
  //
  //  // 00001900:
  //  uint8_t secondPart[] = {
  //      0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  //      0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00,
  //      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  //      0x00, 0x00, 0x00, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14,
  //      0x15, 0x16, 0x17, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  //      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  //
  //  for (int i = 0; i < sizeof(firstPart) / sizeof(uint8_t); i++)
  //    vram[i] = firstPart[i];
  //  for (int i = 0; i < sizeof(secondPart) / sizeof(uint8_t); i++)
  //    vram[i + 0x1900] = secondPart[i];
  //
  //  util::hexdump(vram, vram.size(), 0x8000);
}

uint8_t PPU::read(uint16_t addr) {
  if (addr >= 0x8000 && addr < 0xA000) {
    return vram[addr - 0x8000];
  } else if (addr >= 0xFE00 && addr < 0xFEA0) {
    return oam[addr - 0xFE00];
  } else if (addr >= 0xFF00 && addr < 0xFF4C) {
    if (addr == 0xFF40)
      return LCDC;
    if (addr == 0xFF41)
      return STAT;
    if (addr == 0xFF42)
      return SCY;
    if (addr == 0xFF43)
      return SCX;
    if (addr == 0xFF44)
      return LY;
    if (addr == 0xFF45)
      return LYC;
    if (addr == 0xFF47)
      return BGP;
  }

  fprintf(stderr, "PPU read at bad address: %04X\n", addr);
  return 0xFF;
}

void PPU::write(uint16_t addr, uint8_t value) {
  if (addr >= 0x8000 && addr < 0xA000) {
    vram[addr - 0x8000] = value;
  } else if (addr >= 0xFE00 && addr < 0xFEA0) {
    oam[addr - 0xFE00] = value;
  } else if (addr >= 0xFF00 && addr < 0xFF4C) {
    if (addr == 0xFF40)
      LCDC = value;
    if (addr == 0xFF41) {
      uint8_t writeMask = 0b11111000;
      value &= writeMask;
      STAT &= ~writeMask;
      STAT |= value;
    }
    if (addr == 0xFF42)
      SCY = value;
    if (addr == 0xFF43)
      SCX = value;
    if (addr == 0xFF44)
      LY = 0;
    if (addr == 0xFF45)
      LYC = value;
    if (addr == 0xFF47)
      BGP = value;
    if (addr == 0xFF48)
      OBP0 = value;
    if (addr == 0xFF49)
      OBP1 = value;
    if (addr == 0xFF4A)
      WY = value;
    if (addr == 0xFF4B)
      WX = value;
  } else
    fprintf(stderr, "PPU Written at bad address: %04X\n", addr);
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

  textureData = (GLubyte *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
  pixels = new uint8_t[WIDTH * HEIGHT * BYTES_PER_PIXEL];

  printf("Finished setup\n");
  hasSetUp = true;
}

void PPU::step() {
  if (!hasSetUp)
    return;
  bool isLCDOn = LCDC & (1 << 7);
  if (!isLCDOn)
    return;

  if (LX == 0) {
    if (STAT & (1 << 6) && LY == LYC) {
      bus->raiseInterrupt(interruptLCDC);
    }
    if (STAT & (1 << 4) && LY == 144) {
      bus->raiseInterrupt(interruptLCDC);
    }
    STAT &= 0xFB;
    if (LY == LYC) {
      STAT |= 0x4;
    }
  }

  uint16_t windowTileMap = LCDC & (1 << 6) ? 0x9C00 : 0x9800;
  bool showWindow = LCDC & (1 << 5);
  uint16_t tileDataBase = LCDC & (1 << 4) ? 0x8000 : 0x9000;
  bool signedTileIndex = (LCDC & (1 << 4)) == 0;
  uint16_t bgTileMapDisplay = LCDC & (1 << 3) ? 0x9C00 : 0x9800;
  uint8_t spriteHeight = LCDC & (1 << 2) ? 16 : 8;
  bool showSprites = LCDC & (1 << 1);
  bool showBGAndWindow = LCDC & 1;

  int mode = 0b01;
  if (LY < 144) {
    if (WY == LY) {
      windowEnabled = true;
    }

    if (LX < 20) {
      mode = 0b10;
    } else if (LX < 63) {
      mode = 0b11;
    } else {
      mode = 0b00;
      if (LX == 63) {
        showWindow = showWindow && windowEnabled;
        if (WX > 166 || WY > 143)
          showWindow = false;
        bool renderedWindow = false;
        uint8_t yy = LY + SCY;
        uint8_t yt = yy / 8;

        std::vector<Sprite> sprites;
        if (showSprites) {
          sprites.reserve(10);
          std::vector<Sprite> unsortedSprites;
          unsortedSprites.reserve(40);
          for (int spIndex = 0; spIndex < 40; spIndex++) {
            uint8_t y = oam[4 * spIndex];
            if (LY < y - 16 || LY >= y + spriteHeight - 16)
              continue;

            uint8_t x = oam[4 * spIndex + 1];
            uint8_t tile = oam[4 * spIndex + 2];
            uint8_t attributes = oam[4 * spIndex + 3];
            unsortedSprites.emplace_back(x, y, tile, attributes);
          }
          while (unsortedSprites.size() > 0 && sprites.size() < 10) {
            uint8_t leftmostIndex = 0;
            uint8_t leftmostX = 168;
            for (int i = 0; i < unsortedSprites.size(); i++) {
              if (unsortedSprites[i].x < leftmostX) {
                leftmostX = unsortedSprites[i].x;
                leftmostIndex = i;
              }
            }
            sprites.push_back(unsortedSprites[leftmostIndex]);
            unsortedSprites.erase(unsortedSprites.begin() + leftmostIndex);
          }
        }

        mtx.lock();
        for (int x = 0; x < 20 * 8; x++) {
          bool showWindowThisPixel = showWindow;
          if (LY < WY || x < WX - 7)
            showWindowThisPixel = false;
          uint8_t xx = x + SCX;
          uint8_t xt = xx / 8;

          int bgtile = read(bgTileMapDisplay + yt * 32 + xt);
          int color = getColorForTile(tileDataBase, signedTileIndex, bgtile,
                                      xx % 8, yy % 8);
          if (!showBGAndWindow) {
            color = 0;
          }
          if (showWindowThisPixel) {
            renderedWindow = true;
            uint8_t yw = WLY;
            uint8_t xw = x - WX + 7;
            yt = yw / 8;
            xt = xw / 8;
            int wintile = read(windowTileMap + yt * 32 + xt);
            color = getColorForTile(tileDataBase, signedTileIndex, wintile,
                                    xw % 8, yw % 8);
          }
		  uint8_t palette = BGP;
          if (showSprites) {
            uint8_t spriteColor = 0;
            bool bgPriority = false;
			uint8_t spritePalette = 0;
            for (Sprite &sprite : sprites) {
              if (x >= sprite.x - 8 && x < sprite.x) {
                uint8_t yt = (LY - (sprite.y - 16)) % 8;
                uint8_t xt = x - (sprite.x - 8);
                yt = sprite.attributes & 0x40 ? (7 - yt) : yt;
                xt = sprite.attributes & 0x20 ? (7 - xt) : xt;
                if (spriteHeight == 16) {
                  uint8_t topSprite = sprite.attributes & 0x40
                                          ? (sprite.tile | 0x1)
                                          : (sprite.tile & 0xFE);
                  if (LY >= sprite.y - 8) {
                    spriteColor =
                        getColorForTile(0x8000, false, topSprite ^ 0x1, xt, yt);
                  } else {
                    spriteColor =
                        getColorForTile(0x8000, false, topSprite, xt, yt);
                  }
                } else {
                  spriteColor =
                      getColorForTile(0x8000, false, sprite.tile, xt, yt);
                }

				spritePalette = sprite.attributes & 0x10 ? OBP1 : OBP0;
                bgPriority = sprite.attributes & 0x80;
                break;
              }
            }
            if (spriteColor != 0 && (!bgPriority || color == 0)) {
              color = spriteColor;
			  palette = spritePalette;
            }
          }
          uint32_t paletteColors[] = {0xf7bef7, 0xe78686, 0x7733e7, 0x2c2c96};
          uint8_t paletteIndex = (palette >> (2 * color)) & 0x3;

          color = paletteColors[paletteIndex];

          int ri = 3 * (WIDTH * LY + x);
          int gi = 3 * (WIDTH * LY + x) + 1;
          int bi = 3 * (WIDTH * LY + x) + 2;

          pixels[ri] = (color >> 16) & 0xFF;
          pixels[gi] = (color >> 8) & 0xFF;
          pixels[bi] = color & 0xFF;
        }
        mtx.unlock();

        if (renderedWindow)
          WLY++;
      }
    }
  } else {
    mode = 0b01;
    if (LY == 144 && LX == 0) {
      WLY = 0;
      bus->raiseInterrupt(interruptVblank);
      frame++;
      invalidate();
    }
    windowEnabled = false;
  }

  STAT &= 0xFC;
  STAT |= mode & 0x3;

  LX++;
  if (LX >= 114) {
    LX = 0;
    LY++;
    if (LY >= 154) {
      LY = 0;
    }
  }
}

void PPU::render() {
  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);

    if (invalidated) {
      invalidated = false;

      mtx.lock(); // TODO: go back to editing textureData directly
      memcpy(textureData, pixels, HEIGHT * WIDTH * BYTES_PER_PIXEL);
      mtx.unlock();
      glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB,
                   GL_UNSIGNED_BYTE, 0);
      textureData =
          (GLubyte *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
      if (!textureData) {
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

int8_t PPU::getColorForTile(uint16_t baseAddr, bool signedTileIndex,
                            uint8_t index, uint8_t x, uint8_t y) {
  int16_t offset = (signedTileIndex ? (int8_t)index : (uint8_t)index);
  uint16_t addr = baseAddr + 0x10 * offset + 2 * y;
  return ((read(addr) >> (7 - x)) & 0x1) |
         (((read(addr + 1) >> (7 - x)) & 0x1) << 1);
}

void PPU::cleanup() {
  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteProgram(shaderProgram);

  glfwDestroyWindow(window);
  glfwTerminate();
}

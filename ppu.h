#include <GL/glew.h>
#include <GLFW/glfw3.h>

constexpr int SCALE = 3;
constexpr int WIDTH = 160;
constexpr int HEIGHT = 144;
constexpr int BYTES_PER_PIXEL = 3;

class PPU {
  uint8_t LCDC;

  uint8_t SCY = 0;
  uint8_t SCX = 0;
  uint8_t LY = 0;
  uint8_t LX = 0;
  bool invalidated = false;

  GLubyte *pixelData = 0;

  GLFWwindow *window;
  unsigned int screenTexture;
  unsigned int vao, vbo, pbo;
  unsigned int shaderProgram;

  bool hasClosed = false;

public:
  void step();

  void setup();
  void render();
  void cleanup();

  void invalidate();

  void setLCDC(uint8_t LCDC) { this->LCDC = LCDC; }

  bool isClosed();
  void close();
};

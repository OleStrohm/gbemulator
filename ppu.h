#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <mutex>
#include <vector>

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

  std::vector<uint8_t> vram;

public:
  GLubyte *pixelData = 0;

private:
  GLFWwindow *window;
  unsigned int screenTexture;
  unsigned int vao, vbo, pbo;
  unsigned int shaderProgram;

  bool hasClosed = false;
  int frame;

  bool hasSetUp;
  std::mutex mtx;

public:
  PPU();

  void step();
  int8_t getColorForTile(uint8_t index, uint8_t x, uint8_t y);

  void setup();
  void render();
  void cleanup();

  void invalidate();

  void setLCDC(uint8_t LCDC) { this->LCDC = LCDC; }

  bool isClosed();
  void close();

  void setFrame(int frame) { this->frame = frame; }
  int getFrame() { return frame; }
  void setLX(uint8_t LX) { this->LX = LX; }
  uint8_t getLX() { return LX; }
  void setLY(uint8_t LY) { this->LY = LY; }
  uint8_t getLY() { return LY; }
};


#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

void error_callback(int error, const char *description) {
  fprintf(stderr, "ERROR(%i): %s\n", error, description);
}

#define SCALE 3
#define WIDTH 160
#define HEIGHT 144

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

int main() {
  glfwSetErrorCallback(error_callback);

  if (!glfwInit()) {
    std::cout << "ERROR: Could not initialize GLFW" << std::endl;
    glfwTerminate();
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  GLFWwindow *window = glfwCreateWindow(WIDTH * SCALE, HEIGHT * SCALE,
                                        "Gameboy Emulator", NULL, NULL);
  if (!window) {
    std::cout << "ERROR: Could not initialize GLFW" << std::endl;
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);

  if (glewInit() != GLEW_OK) {
    std::cout << "ERROR: Could not initialize GLFW" << std::endl;
    glfwTerminate();
    return 1;
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
    return 1;
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
    return 1;
  }

  unsigned int shaderProgram;
  shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);

  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
              << infoLog << std::endl;
    return 1;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  float vertices[] = {
      -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.5f,  -0.5f, 0.0f, 0.0f, 0.0f,
      0.5f,  0.5f,  0.0f, 1.0f, 1.0f, 0.5f,  0.5f,  0.0f, 1.0f, 1.0f,
      -0.5f, 0.5f,  0.0f, 0.0f, 1.0f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
  };

  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  int width = WIDTH;
  int height = HEIGHT;
  int nrChannels = 3;
  unsigned char textureData[3 * 160 * 144];
  for (int i = 0; i < width * height * nrChannels; i += 3) {
    int r = i;
    int g = i + 1;
    int b = i + 2;
    textureData[r] = 0x33;
    textureData[g] = 0x99;
    textureData[b] = 0x99;
  }

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
               GL_UNSIGNED_BYTE, textureData);

  unsigned int vao, vbo;
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

  unsigned int pbo;
  glGenBuffers(1, &pbo);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);

  glBufferData(GL_PIXEL_UNPACK_BUFFER, WIDTH * HEIGHT * nrChannels, 0,
               GL_STATIC_DRAW);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

  glViewport(0, 0, WIDTH * SCALE, HEIGHT * SCALE);

  bool updateTexture = false;
  bool keyDown = false;

  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);

    if (updateTexture) {
      printf("Changing Texture\n");
      for (int i = 0; i < width * height * nrChannels; i += 3) {
        int r = i;
        int g = i + 1;
        int b = i + 2;
        textureData[r] = rand();
        textureData[g] = rand();
        textureData[b] = rand();
      }

      glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeof(textureData), textureData,
                   GL_STREAM_DRAW);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                   GL_UNSIGNED_BYTE, 0);

      updateTexture = false;
    }

    glUseProgram(shaderProgram);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glfwSwapBuffers(window);
    glfwPollEvents();

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
      keyDown = false;
    }
    if (!keyDown && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
      updateTexture = true;
      keyDown = true;
    }
  }

  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteProgram(shaderProgram);

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}

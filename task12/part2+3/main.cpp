#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <cstring>
#include <cmath>

float colorInfluence = 0.5f;
float textureBlendRatio = 0.5f;
bool useTexture1 = true;
bool useTexture2 = true;

bool key1Pressed = false;
bool key2Pressed = false;

const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 600;

void processInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float speed = 1.0f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        colorInfluence -= speed;
        if (colorInfluence < 0.0f) colorInfluence = 0.0f;
        std::cout << "Color Influence: " << colorInfluence << "    \r";
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        colorInfluence += speed;
        if (colorInfluence > 1.0f) colorInfluence = 1.0f;
        std::cout << "Color Influence: " << colorInfluence << "    \r";
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        textureBlendRatio -= speed;
        if (textureBlendRatio < 0.0f) textureBlendRatio = 0.0f;
        std::cout << "Blend Ratio: " << textureBlendRatio << "      \r";
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        textureBlendRatio += speed;
        if (textureBlendRatio > 1.0f) textureBlendRatio = 1.0f;
        std::cout << "Blend Ratio: " << textureBlendRatio << "      \r";
    }

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        if (!key1Pressed) {
            useTexture1 = !useTexture1;
            key1Pressed = true;
            std::cout << "\nКуб 1: текстура " << (useTexture1 ? "ВКЛ" : "ВЫКЛ") << std::endl;
        }
    }
    else {
        key1Pressed = false;
    }

    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        if (!key2Pressed) {
            useTexture2 = !useTexture2;
            key2Pressed = true;
            std::cout << "\nКуб 2: текстуры " << (useTexture2 ? "ВКЛ" : "ВЫКЛ") << std::endl;
        }
    }
    else {
        key2Pressed = false;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Ошибка компиляции шейдера:\n" << infoLog << std::endl;
    }
    return shader;
}

GLuint createShaderProgram(const char* vertexSource, const char* fragmentSource) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Ошибка линковки программы:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

GLuint loadTextureFromFile(const char* path) {
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;
        else format = GL_RGB;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        std::cout << "[OK] Текстура загружена: " << path << std::endl;
    }
    else {
        std::cerr << "[ОШИБКА] Не удалось загрузить текстуру: " << path << std::endl;
        glDeleteTextures(1, &textureID);
        return 0;
    }

    stbi_image_free(data);
    return textureID;
}

GLuint generateCheckerTexture(int size, int cellSize, unsigned char r1, unsigned char g1, unsigned char b1, unsigned char r2, unsigned char g2, unsigned char b2) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    unsigned char* data = new unsigned char[size * size * 3];

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int idx = (y * size + x) * 3;
            bool isWhite = ((x / cellSize) + (y / cellSize)) % 2 == 0;
            if (isWhite) { data[idx] = r1; data[idx + 1] = g1; data[idx + 2] = b1; }
            else { data[idx] = r2; data[idx + 1] = g2; data[idx + 2] = b2; }
        }
    }
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    delete[] data;
    return textureID;
}

GLuint generateGradientTexture(int size, unsigned char r1, unsigned char g1, unsigned char b1, unsigned char r2, unsigned char g2, unsigned char b2) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    unsigned char* data = new unsigned char[size * size * 3];
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int idx = (y * size + x) * 3;
            float t = (float)x / (float)(size - 1);
            data[idx] = (unsigned char)(r1 + t * (r2 - r1));
            data[idx + 1] = (unsigned char)(g1 + t * (g2 - g1));
            data[idx + 2] = (unsigned char)(b1 + t * (b2 - b1));
        }
    }
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    delete[] data;
    return textureID;
}

GLuint generateCircleTexture(int size, int numCircles, unsigned char bgR, unsigned char bgG, unsigned char bgB, unsigned char fgR, unsigned char fgG, unsigned char fgB) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    unsigned char* data = new unsigned char[size * size * 3];
    float cx = size / 2.0f, cy = size / 2.0f, maxRadius = size / 2.0f;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int idx = (y * size + x) * 3;
            float dx = x - cx, dy = y - cy;
            float dist = sqrt(dx * dx + dy * dy);
            int ring = (int)(dist / (maxRadius / numCircles));
            if (ring % 2 == 0) { data[idx] = fgR; data[idx + 1] = fgG; data[idx + 2] = fgB; }
            else { data[idx] = bgR; data[idx + 1] = bgG; data[idx + 2] = bgB; }
        }
    }
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    delete[] data;
    return textureID;
}

GLuint loadTexture(const char* path, GLuint(*fallbackGenerator)()) {
    GLuint texture = loadTextureFromFile(path);
    if (texture == 0 && fallbackGenerator != nullptr) {
        std::cout << "         Использую сгенерированную текстуру вместо файла" << std::endl;
        return fallbackGenerator();
    }
    return texture;
}

GLuint fallbackTexture1() { return generateCheckerTexture(256, 32, 255, 255, 255, 50, 50, 50); }
GLuint fallbackTexture2() { return generateGradientTexture(256, 255, 100, 50, 50, 100, 255); }
GLuint fallbackTexture3() { return generateCircleTexture(256, 5, 200, 150, 50, 50, 100, 200); }

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec3 ourColor;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    ourColor = aColor;
    TexCoord = aTexCoord;
}
)";

const char* fragmentShaderSource1 = R"(
#version 330 core
in vec3 ourColor;
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D ourTexture;
uniform float colorInfluence;
uniform bool useTexture;
void main() {
    vec4 vertexColor = vec4(ourColor, 1.0);
    if (useTexture) {
        vec4 texColor = texture(ourTexture, TexCoord);
        FragColor = mix(texColor, texColor * vertexColor, colorInfluence);
    } else {
        FragColor = vertexColor;
    }
}
)";

const char* fragmentShaderSource2 = R"(
#version 330 core
in vec3 ourColor;
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D texture1;
uniform sampler2D texture2;
uniform float blendRatio;
uniform bool useTexture;
void main() {
    if (useTexture) {
        vec4 tex1 = texture(texture1, TexCoord);
        vec4 tex2 = texture(texture2, TexCoord);
        FragColor = mix(tex1, tex2, blendRatio);
    } else {
        FragColor = vec4(ourColor, 1.0);
    }
}
)";

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "  OpenGL Кубики с текстурами" << std::endl;
    std::cout << "========================================" << std::endl;

    const char* texturePath1 = (argc > 1) ? argv[1] : "textures/texture1.bmp";
    const char* texturePath2 = (argc > 2) ? argv[2] : "textures/texture2.bmp";
    const char* texturePath3 = (argc > 3) ? argv[3] : "textures/texture3.bmp";

    if (!glfwInit()) {
        std::cerr << "Ошибка инициализации GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL", NULL, NULL);
    if (!window) {
        std::cerr << "Ошибка создания окна GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Ошибка инициализации GLEW" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    float vertices[] = {
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,  1.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,  0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  1.0f, 1.0f,

         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,  1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f,

         -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,  0.0f, 1.0f,
          0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 1.0f,  1.0f, 1.0f,
          0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
          0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.0f, 0.0f,
         -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 1.0f,  0.0f, 1.0f,

         -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,  0.0f, 1.0f,
          0.5f,  0.5f, -0.5f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,
          0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
          0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f,
         -0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         -0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f,  0.0f, 1.0f
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    GLuint shaderProgram1 = createShaderProgram(vertexShaderSource, fragmentShaderSource1);
    GLuint shaderProgram2 = createShaderProgram(vertexShaderSource, fragmentShaderSource2);

    GLuint texture1 = loadTexture(texturePath1, fallbackTexture1);
    GLuint texture2 = loadTexture(texturePath2, fallbackTexture2);
    GLuint texture3 = loadTexture(texturePath3, fallbackTexture3);

    glUseProgram(shaderProgram2);
    glUniform1i(glGetUniformLocation(shaderProgram2, "texture1"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram2, "texture2"), 1);

    std::cout << "\n=== Управление ===" << std::endl;
    std::cout << "Q/E  - уменьшить/увеличить влияние цвета (смотрите консоль)" << std::endl;
    std::cout << "A/D  - изменить пропорцию смешивания текстур (смотрите консоль)" << std::endl;
    std::cout << "1    - вкл/выкл текстуру на левом кубе" << std::endl;
    std::cout << "2    - вкл/выкл текстуры на правом кубе" << std::endl;
    std::cout << "ESC  - выход" << std::endl;

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, deltaTime);

        glClearColor(0.2f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        float time = (float)glfwGetTime();

        glUseProgram(shaderProgram1);
        glm::mat4 model1 = glm::translate(glm::mat4(1.0f), glm::vec3(-1.2f, 0.0f, 0.0f));
        model1 = glm::rotate(model1, time * 0.5f, glm::vec3(0.5f, 1.0f, 0.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram1, "model"), 1, GL_FALSE, glm::value_ptr(model1));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram1, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram1, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(shaderProgram1, "colorInfluence"), colorInfluence);
        glUniform1i(glGetUniformLocation(shaderProgram1, "useTexture"), useTexture1);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glUseProgram(shaderProgram2);
        glm::mat4 model2 = glm::translate(glm::mat4(1.0f), glm::vec3(1.2f, 0.0f, 0.0f));
        model2 = glm::rotate(model2, time * 0.5f, glm::vec3(0.0f, 1.0f, 0.5f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram2, "model"), 1, GL_FALSE, glm::value_ptr(model2));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram2, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram2, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(shaderProgram2, "blendRatio"), textureBlendRatio);
        glUniform1i(glGetUniformLocation(shaderProgram2, "useTexture"), useTexture2);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture2);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture3);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        char title[256];
        snprintf(title, sizeof(title), "Куб1: %.2f | Куб2: %.2f | (См. консоль для точных значений)", colorInfluence, textureBlendRatio);
        glfwSetWindowTitle(window, title);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram1);
    glDeleteProgram(shaderProgram2);
    glDeleteTextures(1, &texture1);
    glDeleteTextures(1, &texture2);
    glDeleteTextures(1, &texture3);
    glfwTerminate();
    return 0;
}
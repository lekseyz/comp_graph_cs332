#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

std::string readShaderSource(const char* filePath) {
    std::ifstream shaderFile(filePath);
    if (!shaderFile.is_open()) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        return "";
    }
    std::stringstream shaderStream;
    shaderStream << shaderFile.rdbuf();
    shaderFile.close();
    return shaderStream.str();
}

GLuint compileShader(const char* source, GLenum shaderType) {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}

GLuint createShaderProgram(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode = readShaderSource(vertexPath);
    std::string fragmentCode = readShaderSource(fragmentPath);

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    GLuint vertexShader = compileShader(vShaderCode, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fShaderCode, GL_FRAGMENT_SHADER);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

struct RGB {
    float r, g, b;
};

// Function to convert HSV to RGB
RGB hsvToRgb(float h, float s, float v) {
    RGB rgb;
    int i = floor(h * 6);
    float f = h * 6 - i;
    float p = v * (1 - s);
    float q = v * (1 - f * s);
    float t = v * (1 - (1 - f) * s);

    switch (i % 6) {
        case 0: rgb.r = v, rgb.g = t, rgb.b = p; break;
        case 1: rgb.r = q, rgb.g = v, rgb.b = p; break;
        case 2: rgb.r = p, rgb.g = v, rgb.b = t; break;
        case 3: rgb.r = p, rgb.g = q, rgb.b = v; break;
        case 4: rgb.r = t, rgb.g = p, rgb.b = v; break;
        case 5: rgb.r = v, rgb.g = p, rgb.b = q; break;
    }

    return rgb;
}

int main() {
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.antialiasingLevel = 4;
    settings.majorVersion = 3;
    settings.minorVersion = 3;

    sf::RenderWindow window(sf::VideoMode({800, 600}), "Gradient Circle");
    window.setVerticalSyncEnabled(true);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    GLuint shaderProgram = createShaderProgram("shader.vs", "shader.fs");

    std::vector<float> vertices;
    const int numSegments = 360;
    const float radius = 0.5f;

    vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f); // Position
    vertices.push_back(1.0f); vertices.push_back(1.0f); vertices.push_back(1.0f); // Color

    for (int i = 0; i <= numSegments; ++i) {
        float angle = i * (2.0f * M_PI / numSegments);
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        
        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(0.0f);

        RGB color = hsvToRgb(static_cast<float>(i) / numSegments, 1.0f, 1.0f);
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);
    }

    // Create VBO and VAO
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    float scaleX = 1.0f;
    float scaleY = 1.0f;

    while (window.isOpen()) {
        sf::Event event;
        if (window.pollEvent(event)) {
            if (event.Closed)
                window.close();
        }


        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            scaleX += 0.01f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            scaleX -= 0.01f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            scaleY += 0.01f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            scaleY -= 0.01f;
        
        if (scaleX < 0.1f) scaleX = 0.1f;
        if (scaleY < 0.1f) scaleY = 0.1f;


        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glUniform2f(glGetUniformLocation(shaderProgram, "scale"), scaleX, scaleY);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, numSegments + 2);

        window.display();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    return 0;
}

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW error " << error << ": " << description << "\n";
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, '\0');
        glGetShaderInfoLog(shader, logLen, nullptr, log.data());
        std::cerr << "Shader compile error: " << log << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint createProgram(const char* vsSource, const char* fsSource) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSource);
    if (!vs) return 0;
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSource);
    if (!fs) {
        glDeleteShader(vs);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLen = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLen);
        std::string log(logLen, '\0');
        glGetProgramInfoLog(program, logLen, nullptr, log.data());
        std::cerr << "Program link error: " << log << "\n";
        glDeleteProgram(program);
        return 0;
    }

    return program;
}

const char* vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vColor;

uniform mat4 uMVP;

void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core

in vec3 vColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

int main() {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(700, 700, "Gradient Tetrahedron", nullptr, nullptr);
    if (!window) {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        std::cerr << "GLEW init failed: " << glewGetErrorString(glewErr) << "\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    float vertices[] = {
         1.0f,  1.0f,  1.0f,    1.0f, 0.0f, 0.0f,
        -1.0f, -1.0f,  1.0f,    0.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, -1.0f,    0.0f, 0.0f, 1.0f, 
         1.0f, -1.0f, -1.0f,    1.0f, 1.0f, 0.0f 
    };

    unsigned int indices[] = {
        0, 1, 2,  
        0, 3, 1,  
        0, 2, 3, 
        1, 3, 2  
    };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(
        0,          
        3,            
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(float),   
        (void*)0
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,              
        3,
        GL_FLOAT,
        GL_FALSE,
        6 * sizeof(float),
        (void*)(3 * sizeof(float)) 
    );
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    GLuint program = createProgram(vertexShaderSource, fragmentShaderSource);
    if (!program) {
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteVertexArrays(1, &VAO);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glm::vec3 position(0.0f, 0.0f, 0.0f);

    glm::mat4 baseRotation = glm::mat4(1.0f);
    baseRotation = glm::rotate(baseRotation, glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    baseRotation = glm::rotate(baseRotation, glm::radians(40.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    float aspect = (float)width / (float)height;

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glm::mat4 view       = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 5.0f),
        glm::vec3(0.0f, 0.0f, 0.0f), 
        glm::vec3(0.0f, 1.0f, 0.0f)   
    );

    glUseProgram(program);
    GLint mvpLoc = glGetUniformLocation(program, "uMVP");

    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);

    const float moveStep = 0.02f;
    const float rotateStep = 0.5f;

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) position.x -= moveStep;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) position.x += moveStep;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) position.y += moveStep;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) position.y -= moveStep;

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) position.z += moveStep;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) position.z -= moveStep;
        
        if (glfwGetKey(window, GLFW_KEY_COMMA) == GLFW_PRESS) baseRotation = glm::rotate(baseRotation, glm::radians(rotateStep), glm::vec3(1.0f, 0.0f, 0.0f));

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glfwGetFramebufferSize(window, &width, &height);
        if (height == 0) height = 1;
        aspect = (float)width / (float)height;
        projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), position) * baseRotation;

        glm::mat4 mvp = projection * view * model;

        glUseProgram(program);
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(program);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

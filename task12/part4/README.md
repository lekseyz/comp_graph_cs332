# Gradient Circle OpenGL Application

This project implements an OpenGL application in C++ that displays a gradient circle. The circle's color gradient is based on the HSV color model, transitioning through hues along its circumference with a white center. The application allows independent scaling of the circle along the X and Y axes via keyboard input. Vertex Buffer Objects (VBOs) are utilized for efficient storage and transfer of vertex data, and uniform variables are used to pass scaling factors to the shaders. SFML is used for window management and event handling.

## Features

*   **Radial Hue Gradient:** The circle displays a gradient where the hue changes around the circumference, and the center is white.
*   **Scalable Axes:** The circle can be independently scaled along the X and Y axes using keyboard controls.
*   **OpenGL Core Profile:** Uses modern OpenGL (version 3.3).
*   **VBOs for Vertex Data:** Vertex positions and colors are managed using Vertex Buffer Objects.
*   **Uniform Variables for Scaling:** Scaling factors are passed to the shaders via uniform variables for dynamic manipulation.
*   **SFML Integration:** Utilizes SFML for window creation, OpenGL context management, and event handling.

## Project Structure

The project consists of the following files:

*   `Makefile`: Manages the compilation process, linking C++ source files with necessary libraries.
*   `main.cpp`: The primary C++ source file containing application logic, window management, event handling, OpenGL setup, and rendering loop.
*   `shader.vs`: The vertex shader, responsible for transforming vertex positions and passing interpolated data to the fragment shader.
*   `shader.fs`: The fragment shader, responsible for calculating the final color of each pixel based on interpolated data.

## Building the Project

The project requires a C++17 compatible compiler (e.g., g++), SFML, and GLEW libraries.

### Dependencies Installation (Arch Linux)

For Arch Linux users, the required libraries can be installed using `pacman`:

```bash
sudo pacman -S sfml glew
```

### Compilation

Navigate to the project's root directory in your terminal and execute the `make` command:

```bash
make
```

This command will compile `main.cpp` along with the shaders and link them against SFML and GLEW, producing an executable named `gradient_circle`.

## Running the Application

After successful compilation, run the executable from the terminal:

```bash
./gradient_circle
```

### Controls

*   `W`: Increase Y-axis scale
*   `S`: Decrease Y-axis scale
*   `A`: Decrease X-axis scale
*   `D`: Increase X-axis scale

## Code Explanation

### Shaders: GPU-Side Logic

The graphics pipeline in OpenGL involves several stages, two of which are programmable through shaders: the Vertex Shader and the Fragment Shader.

#### Vertex Shader (`shader.vs`)

The vertex shader processes each vertex of the geometry. Its primary role is to determine the final position of the vertex on the screen.

```glsl
#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 ourColor;

uniform vec2 scale;

void main()
{
    gl_Position = vec4(aPos.x * scale.x, aPos.y * scale.y, aPos.z, 1.0);
    ourColor = aColor;
}
```

*   **`in vec3 aPos`, `in vec3 aColor`**: These are input attributes for each vertex, representing its position and color, respectively. Their `location` specifies how they map to the data provided from the C++ application.
*   **`uniform vec2 scale`**: This is a global variable, meaning its value is consistent across all vertices within a single draw call. It is used to pass the scaling factors (X and Y) from the C++ application to the shader.
*   **`gl_Position = ...`**: This line calculates the final clip-space position of the vertex. The `x` and `y` components of `aPos` are multiplied by the `scale.x` and `scale.y` factors, effectively scaling the circle.
*   **`ourColor = aColor`**: The input color of the vertex (`aColor`) is passed to the `ourColor` output variable, which will be interpolated across the primitive and provided as input to the fragment shader.

#### Fragment Shader (`shader.fs`)

The fragment shader executes for each pixel (fragment) that belongs to the rasterized primitive. Its main task is to determine the final color of that pixel.

```glsl
#version 330 core

in vec3 ourColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(ourColor, 1.0);
}
```

*   **`in vec3 ourColor`**: This input receives the interpolated color from the vertex shader. OpenGL automatically interpolates the `ourColor` values across the triangle, providing a smoothly blended color for each fragment.
*   **`out vec4 FragColor`**: This is the output variable that holds the final color of the fragment. The `1.0` indicates full opacity.

### Main Application (`main.cpp`): CPU-Side Logic

The `main.cpp` file orchestrates the entire application, handling window management, input, data preparation, and rendering commands.

#### Data Preparation

1.  **Shader Loading and Compilation**:
    *   Functions like `readShaderSource`, `compileShader`, and `createShaderProgram` are responsible for reading the shader code from files, compiling them at runtime, and linking them into a single shader program executable on the GPU.

2.  **HSV to RGB Conversion (`hsvToRgb`)**:
    *   This utility function is crucial for generating the vibrant color gradient of the circle. It converts Hue (H), Saturation (S), and Value (V) components into Red (R), Green (G), and Blue (B) components.
    *   For the circle, the `hue` component is varied based on the angular position of each vertex around the circumference, while `saturation` and `value` are kept at maximum (1.0) to produce bright, pure colors.

3.  **Circle Geometry Generation**:
    *   The circle is approximated using a `GL_TRIANGLE_FAN` primitive type. This means a central vertex is connected to a series of vertices around the circumference, forming triangles that fan out from the center.
    *   A central vertex at `(0.0, 0.0, 0.0)` with a white color `(1.0, 1.0, 1.0)` is defined.
    *   Subsequent vertices are generated along the circumference. For each vertex, its `x` and `y` coordinates are calculated using `cos()` and `sin()` functions based on its angle. The `hsvToRgb` function then assigns a unique hue-based color to each circumferential vertex, creating the gradient.

4.  **Vertex Buffer Object (VBO) and Vertex Array Object (VAO)**:
    *   **VBO**: A VBO (`glGenBuffers`, `glBindBuffer`, `glBufferData`) is created to store the vertex position and color data in the GPU's memory. This minimizes data transfer between the CPU and GPU, leading to better performance.
    *   **VAO**: A VAO (`glGenVertexArrays`, `glBindVertexArray`) encapsulates the configuration of the VBO and its vertex attributes. This includes how the data in the VBO is structured (e.g., which parts represent position, and which represent color) using `glVertexAttribPointer` and `glEnableVertexAttribArray`. The VAO acts as a blueprint, allowing the application to quickly switch between different vertex data layouts.

#### Main Loop: Runtime Execution

The `while (window.isOpen())` loop continuously executes until the application window is closed.

1.  **Event Handling**:
    *   `window.pollEvent()` checks for system events such as window closure. The event processing loop handles `sf::Event::Closed` to terminate the application.

2.  **Keyboard Input for Scaling**:
    *   `sf::Keyboard::isKeyPressed()` is used to detect if specific keys (W, S, A, D) are currently pressed. This allows for continuous adjustment of the `scaleX` and `scaleY` variables, providing smooth scaling.
    *   The scaling factors are clamped to a minimum value (0.1f) to prevent the circle from disappearing.

3.  **Rendering Commands**:
    *   **`glClearColor` and `glClear`**: Sets the background color and clears the color and depth buffers in preparation for drawing the new frame.
    *   **`glUseProgram(shaderProgram)`**: Activates the compiled shader program, instructing the GPU to use our custom vertex and fragment shaders for subsequent draw calls.
    *   **`glUniform2f(glGetUniformLocation(shaderProgram, "scale"), scaleX, scaleY)`**: This crucial line updates the `scale` uniform variable in the vertex shader with the current `scaleX` and `scaleY` values from the C++ application. This dynamic update in each frame allows the circle to be interactively scaled.
    *   **`glBindVertexArray(VAO)`**: Binds the VAO, effectively restoring all the vertex attribute configurations for our circle data.
    *   **`glDrawArrays(GL_TRIANGLE_FAN, 0, numSegments + 2)`**: This command instructs OpenGL to render the geometry. `GL_TRIANGLE_FAN` is chosen for drawing the circle, and `numSegments + 2` represents the total number of vertices (1 center + `numSegments` circumference vertices + 1 duplicate for closing the fan).
    *   **`window.display()`**: Swaps the front and back buffers, presenting the newly rendered frame to the user and preventing visual tearing or flickering.

#### Cleanup

Upon exiting the main loop, OpenGL resources such as VAOs, VBOs, and shader programs are properly deleted (`glDeleteVertexArrays`, `glDeleteBuffers`, `glDeleteProgram`) to free up GPU memory.

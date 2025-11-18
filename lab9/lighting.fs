#version 330

// Input from vertex shader
in vec3 fragPosition;
in vec3 fragNormal;

// Input uniform values
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec4 colDiffuse;

// Output color
out vec4 finalColor;

void main()
{
    finalColor = colDiffuse;
}

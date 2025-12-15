#version 330

// Входные атрибуты вершин (парсятся из файла модели)
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

// Матрицы (передаются Raylib)
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

// Выходные данные во фрагментный шейдер
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec3 fragNormal;

void main()
{
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
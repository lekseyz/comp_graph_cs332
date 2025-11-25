#include "raylib.h"
#include "rlgl.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "raymath.h"

static const char* vsGradient =
"#version 330\n"
"layout(location = 0) in vec3 vertexPosition;\n"
"layout(location = 3) in vec4 vertexColor;\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
"    gl_Position = vec4(vertexPosition, 1.0);\n"
"    fragColor = vertexColor;\n"
"}\n";

static const char* fsGradient =
"#version 330\n"
"in vec4 fragColor;\n"
"out vec4 finalColor;\n"
"void main()\n"
"{\n"
"    finalColor = fragColor;\n"
"}\n";

static const char* vsFlat =
"#version 330\n"
"layout(location = 0) in vec3 vertexPosition;\n"
"void main()\n"
"{\n"
"    gl_Position = vec4(vertexPosition, 1.0);\n"
"}\n";

static const char* fsFlat =
"#version 330\n"
"uniform vec4 solidColor;\n"
"out vec4 finalColor;\n"
"void main()\n"
"{\n"
"    finalColor = solidColor;\n"
"}\n";

int main(void)
{
    const int screenWidth = 600;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "GRADIENT SHAPES");
    SetTargetFPS(60);

    rlDisableBackfaceCulling();
    rlDisableDepthTest();

    RenderTexture2D target = LoadRenderTexture(600, 600);

    Shader shGradient = LoadShaderFromMemory(vsGradient, fsGradient);
    Shader shFlat = LoadShaderFromMemory(vsFlat, fsFlat);

    int solidColorLoc = GetShaderLocation(shFlat, "solidColor");
    Color quadColor = RED;
    Color fanColor = GREEN;
    Color pentColor = BLUE;

    Mesh quadMesh = { 0 };
    quadMesh.triangleCount = 2;
    quadMesh.vertexCount = quadMesh.triangleCount * 3;
    quadMesh.vertices = (float*)MemAlloc(quadMesh.vertexCount * 3 * sizeof(float));
    quadMesh.colors = (unsigned char*)MemAlloc(quadMesh.vertexCount * 4 * sizeof(unsigned char));
    {
        float s = 0.7f;
        int i = 0;
        quadMesh.vertices[i++] = -s; quadMesh.vertices[i++] = -s; quadMesh.vertices[i++] = 0.0f;
        quadMesh.vertices[i++] = s;  quadMesh.vertices[i++] = -s; quadMesh.vertices[i++] = 0.0f;
        quadMesh.vertices[i++] = s;  quadMesh.vertices[i++] = s;  quadMesh.vertices[i++] = 0.0f;
        quadMesh.vertices[i++] = -s; quadMesh.vertices[i++] = -s; quadMesh.vertices[i++] = 0.0f;
        quadMesh.vertices[i++] = s;  quadMesh.vertices[i++] = s;  quadMesh.vertices[i++] = 0.0f;
        quadMesh.vertices[i++] = -s; quadMesh.vertices[i++] = s;  quadMesh.vertices[i++] = 0.0f;

        int c = 0;
        quadMesh.colors[c++] = 255; quadMesh.colors[c++] = 0;   quadMesh.colors[c++] = 0;   quadMesh.colors[c++] = 255;
        quadMesh.colors[c++] = 0;   quadMesh.colors[c++] = 255; quadMesh.colors[c++] = 0;   quadMesh.colors[c++] = 255;
        quadMesh.colors[c++] = 0;   quadMesh.colors[c++] = 0;   quadMesh.colors[c++] = 255; quadMesh.colors[c++] = 255;
        quadMesh.colors[c++] = 255; quadMesh.colors[c++] = 0;   quadMesh.colors[c++] = 0;   quadMesh.colors[c++] = 255;
        quadMesh.colors[c++] = 0;   quadMesh.colors[c++] = 0;   quadMesh.colors[c++] = 255; quadMesh.colors[c++] = 255;
        quadMesh.colors[c++] = 255; quadMesh.colors[c++] = 255; quadMesh.colors[c++] = 0;   quadMesh.colors[c++] = 255;
    }
    UploadMesh(&quadMesh, false);

    Mesh fanMesh = { 0 };
    const int fanSegments = 10;
    const int fanTris = fanSegments - 1;
    fanMesh.triangleCount = fanTris;
    fanMesh.vertexCount = fanTris * 3;
    fanMesh.vertices = (float*)MemAlloc(fanMesh.vertexCount * 3 * sizeof(float));
    fanMesh.colors = (unsigned char*)MemAlloc(fanMesh.vertexCount * 4 * sizeof(unsigned char));
    {
        float r = 0.9f;
        float a0 = -PI * 0.7f;
        float a1 = PI * 0.7f;
        float step = (a1 - a0) / (float)(fanSegments - 1);
        int i = 0;
        int c = 0;

        for (int k = 0; k < fanTris; k++)
        {
            float aA = a0 + step * (float)k;
            float aB = a0 + step * (float)(k + 1);

            fanMesh.vertices[i++] = 0.0f; fanMesh.vertices[i++] = 0.0f; fanMesh.vertices[i++] = 0.0f;
            fanMesh.colors[c++] = 255; fanMesh.colors[c++] = 255; fanMesh.colors[c++] = 255; fanMesh.colors[c++] = 255;

            fanMesh.vertices[i++] = cosf(aA) * r; fanMesh.vertices[i++] = sinf(aA) * r; fanMesh.vertices[i++] = 0.0f;
            float t1 = (float)k / (float)(fanTris - 1);
            fanMesh.colors[c++] = (unsigned char)(255 * (1.0f - t1));
            fanMesh.colors[c++] = 0;
            fanMesh.colors[c++] = (unsigned char)(255 * t1);
            fanMesh.colors[c++] = 255;

            fanMesh.vertices[i++] = cosf(aB) * r; fanMesh.vertices[i++] = sinf(aB) * r; fanMesh.vertices[i++] = 0.0f;
            float t2 = (float)(k + 1) / (float)(fanTris - 1);
            fanMesh.colors[c++] = (unsigned char)(255 * (1.0f - t2));
            fanMesh.colors[c++] = 0;
            fanMesh.colors[c++] = (unsigned char)(255 * t2);
            fanMesh.colors[c++] = 255;
        }
    }
    UploadMesh(&fanMesh, false);

    Mesh pentMesh = { 0 };
    const int pentSides = 5;
    const int pentTris = pentSides;
    pentMesh.triangleCount = pentTris;
    pentMesh.vertexCount = pentTris * 3;
    pentMesh.vertices = (float*)MemAlloc(pentMesh.vertexCount * 3 * sizeof(float));
    pentMesh.colors = (unsigned char*)MemAlloc(pentMesh.vertexCount * 4 * sizeof(unsigned char));
    {
        float r = 0.8f;
        int i = 0;
        int c = 0;

        for (int k = 0; k < pentSides; k++)
        {
            float aA = 2.0f * PI * (float)k / (float)pentSides;
            float aB = 2.0f * PI * (float)(k + 1) / (float)pentSides;

            pentMesh.vertices[i++] = 0.0f; pentMesh.vertices[i++] = 0.0f; pentMesh.vertices[i++] = 0.0f;
            pentMesh.colors[c++] = 255; pentMesh.colors[c++] = 255; pentMesh.colors[c++] = 0; pentMesh.colors[c++] = 255;

            pentMesh.vertices[i++] = cosf(aA) * r; pentMesh.vertices[i++] = sinf(aA) * r; pentMesh.vertices[i++] = 0.0f;
            float hue = (float)k / (float)pentSides;
            Color col1 = ColorFromHSV(hue * 360.0f, 1.0f, 1.0f);
            pentMesh.colors[c++] = col1.r; pentMesh.colors[c++] = col1.g; pentMesh.colors[c++] = col1.b; pentMesh.colors[c++] = 255;

            pentMesh.vertices[i++] = cosf(aB) * r; pentMesh.vertices[i++] = sinf(aB) * r; pentMesh.vertices[i++] = 0.0f;
            float hue2 = (float)(k + 1) / (float)pentSides;
            Color col2 = ColorFromHSV(hue2 * 360.0f, 1.0f, 1.0f);
            pentMesh.colors[c++] = col2.r; pentMesh.colors[c++] = col2.g; pentMesh.colors[c++] = col2.b; pentMesh.colors[c++] = 255;
        }
    }
    UploadMesh(&pentMesh, false);

    Material matQuadGrad = LoadMaterialDefault();
    matQuadGrad.shader = shGradient;
    Material matFanGrad = LoadMaterialDefault();
    matFanGrad.shader = shGradient;
    Material matPentGrad = LoadMaterialDefault();
    matPentGrad.shader = shGradient;

    Material matQuadFlat = LoadMaterialDefault();
    matQuadFlat.shader = shFlat;
    Material matFanFlat = LoadMaterialDefault();
    matFanFlat.shader = shFlat;
    Material matPentFlat = LoadMaterialDefault();
    matPentFlat.shader = shFlat;

    Rectangle canvasSource = (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height };
    Rectangle canvasDest = (Rectangle){ 0, 0, 600, 600 };

    int currentShape = 0;
    int coloringMode = 0;

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_ONE)) currentShape = 0;
        if (IsKeyPressed(KEY_TWO)) currentShape = 1;
        if (IsKeyPressed(KEY_THREE)) currentShape = 2;
        if (IsKeyPressed(KEY_C)) coloringMode = (coloringMode + 1) % 2;

        BeginTextureMode(target);
        ClearBackground((Color) { 30, 30, 30, 255 });

        if (coloringMode == 1) // Flat coloring
        {
            if (currentShape == 0) {
                SetShaderValue(shFlat, solidColorLoc, (float[4]){ quadColor.r/255.0f, quadColor.g/255.0f, quadColor.b/255.0f, quadColor.a/255.0f }, SHADER_UNIFORM_VEC4);
                DrawMesh(quadMesh, matQuadFlat, MatrixIdentity());
            }
            else if (currentShape == 1) {
                SetShaderValue(shFlat, solidColorLoc, (float[4]){ fanColor.r/255.0f, fanColor.g/255.0f, fanColor.b/255.0f, fanColor.a/255.0f }, SHADER_UNIFORM_VEC4);
                DrawMesh(fanMesh, matFanFlat, MatrixIdentity());
            }
            else {
                SetShaderValue(shFlat, solidColorLoc, (float[4]){ pentColor.r/255.0f, pentColor.g/255.0f, pentColor.b/255.0f, pentColor.a/255.0f }, SHADER_UNIFORM_VEC4);
                DrawMesh(pentMesh, matPentFlat, MatrixIdentity());
            }
        }
        else // Gradient coloring
        {
            if (currentShape == 0) DrawMesh(quadMesh, matQuadGrad, MatrixIdentity());
            else if (currentShape == 1) DrawMesh(fanMesh, matFanGrad, MatrixIdentity());
            else DrawMesh(pentMesh, matPentGrad, MatrixIdentity());
        }

        EndTextureMode();

        BeginDrawing();
        ClearBackground(RAYWHITE);
        //if (GuiButton((Rectangle){20, 80, 160, 30}, "QUAD")) currentShape = 0;
        //if (GuiButton((Rectangle){20, 120, 160, 30}, "FAN")) currentShape = 1;
        //if (GuiButton((Rectangle){20, 160, 160, 30}, "PENTAGON")) currentShape = 2;
        DrawTexturePro(target.texture, canvasSource, canvasDest, (Vector2) { 0, 0 }, 0.0f, WHITE);

        DrawText("1 - Quad | 2 - Fan | 3 - Pentagon", 10, 10, 20, BLACK);
        const char* coloringText = (coloringMode == 0) ? "GRADIENT" : "FLAT";
        DrawText(TextFormat("C - Coloring: %s", coloringText), 10, 40, 20, BLACK);
        DrawText("Press 1, 2, or 3 to change shape", 10, 570, 20, DARKGREEN);

        EndDrawing();
    }

    UnloadRenderTexture(target);

    UnloadMesh(quadMesh);
    UnloadMesh(fanMesh);
    UnloadMesh(pentMesh);

    MemFree(quadMesh.vertices);
    MemFree(quadMesh.colors);
    MemFree(fanMesh.vertices);
    MemFree(fanMesh.colors);
    MemFree(pentMesh.vertices);
    MemFree(pentMesh.colors);

    UnloadMaterial(matQuadGrad);
    UnloadMaterial(matFanGrad);
    UnloadMaterial(matPentGrad);
    UnloadMaterial(matQuadFlat);
    UnloadMaterial(matFanFlat);
    UnloadMaterial(matPentFlat);

    UnloadShader(shGradient);
    UnloadShader(shFlat);

    CloseWindow();

    return 0;
}
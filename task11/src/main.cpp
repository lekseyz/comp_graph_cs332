#include "raylib.h"
#include "rlgl.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "raymath.h"

static const char *vsCode =
"#version 330\n"
"in vec3 vertexPosition;\n"
"void main()\n"
"{\n"
"    gl_Position = vec4(vertexPosition, 1.0);\n"
"}\n";

static const char *fsQuad =
"#version 330\n"
"out vec4 finalColor;\n"
"void main()\n"
"{\n"
"    finalColor = vec4(1.0, 0.3, 0.3, 1.0);\n"
"}\n";

static const char *fsFan =
"#version 330\n"
"out vec4 finalColor;\n"
"void main()\n"
"{\n"
"    finalColor = vec4(0.3, 1.0, 0.3, 1.0);\n"
"}\n";

static const char *fsPentagon =
"#version 330\n"
"out vec4 finalColor;\n"
"void main()\n"
"{\n"
"    finalColor = vec4(0.3, 0.3, 1.0, 1.0);\n"
"}\n";

int main(void)
{
    const int screenWidth = 600;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "RAYLIB FLAT SHAPES");
    SetTargetFPS(60);

    rlDisableBackfaceCulling();
    rlDisableDepthTest();

    RenderTexture2D target = LoadRenderTexture(600, 600);

    Shader shQuad = LoadShaderFromMemory(vsCode, fsQuad);
    Shader shFan = LoadShaderFromMemory(vsCode, fsFan);
    Shader shPent = LoadShaderFromMemory(vsCode, fsPentagon);

    Mesh quadMesh = {0};
    quadMesh.triangleCount = 2;
    quadMesh.vertexCount = quadMesh.triangleCount*3;
    quadMesh.vertices = (float *)MemAlloc(quadMesh.vertexCount*3*sizeof(float));
    {
        float s = 0.7f;
        int i = 0;
        quadMesh.vertices[i++] = -s; quadMesh.vertices[i++] = -s; quadMesh.vertices[i++] = 0.0f;
        quadMesh.vertices[i++] =  s; quadMesh.vertices[i++] = -s; quadMesh.vertices[i++] = 0.0f;
        quadMesh.vertices[i++] =  s; quadMesh.vertices[i++] =  s; quadMesh.vertices[i++] = 0.0f;
        quadMesh.vertices[i++] = -s; quadMesh.vertices[i++] = -s; quadMesh.vertices[i++] = 0.0f;
        quadMesh.vertices[i++] =  s; quadMesh.vertices[i++] =  s; quadMesh.vertices[i++] = 0.0f;
        quadMesh.vertices[i++] = -s; quadMesh.vertices[i++] =  s; quadMesh.vertices[i++] = 0.0f;
    }
    UploadMesh(&quadMesh, false);

    Mesh fanMesh = {0};
    const int fanSegments = 10;
    const int fanTris = fanSegments - 1;
    fanMesh.triangleCount = fanTris;
    fanMesh.vertexCount = fanTris*3;
    fanMesh.vertices = (float *)MemAlloc(fanMesh.vertexCount*3*sizeof(float));
    {
        float r = 0.9f;
        float a0 = -PI*0.7f;
        float a1 =  PI*0.7f;
        float step = (a1 - a0)/(float)(fanSegments - 1);
        int i = 0;
        for (int k = 0; k < fanTris; k++)
        {
            float aA = a0 + step*(float)k;
            float aB = a0 + step*(float)(k + 1);
            fanMesh.vertices[i++] = 0.0f; fanMesh.vertices[i++] = 0.0f; fanMesh.vertices[i++] = 0.0f;
            fanMesh.vertices[i++] = cosf(aA)*r; fanMesh.vertices[i++] = sinf(aA)*r; fanMesh.vertices[i++] = 0.0f;
            fanMesh.vertices[i++] = cosf(aB)*r; fanMesh.vertices[i++] = sinf(aB)*r; fanMesh.vertices[i++] = 0.0f;
        }
    }
    UploadMesh(&fanMesh, false);

    Mesh pentMesh = {0};
    const int pentSides = 5;
    const int pentTris = pentSides;
    pentMesh.triangleCount = pentTris;
    pentMesh.vertexCount = pentTris*3;
    pentMesh.vertices = (float *)MemAlloc(pentMesh.vertexCount*3*sizeof(float));
    {
        float r = 0.8f;
        int i = 0;
        for (int k = 0; k < pentSides; k++)
        {
            float aA = 2.0f*PI*(float)k/(float)pentSides;
            float aB = 2.0f*PI*(float)(k + 1)/(float)pentSides;
            pentMesh.vertices[i++] = 0.0f; pentMesh.vertices[i++] = 0.0f; pentMesh.vertices[i++] = 0.0f;
            pentMesh.vertices[i++] = cosf(aA)*r; pentMesh.vertices[i++] = sinf(aA)*r; pentMesh.vertices[i++] = 0.0f;
            pentMesh.vertices[i++] = cosf(aB)*r; pentMesh.vertices[i++] = sinf(aB)*r; pentMesh.vertices[i++] = 0.0f;
        }
    }
    UploadMesh(&pentMesh, false);

    Material matQuad = LoadMaterialDefault();
    matQuad.shader = shQuad;
    Material matFan = LoadMaterialDefault();
    matFan.shader = shFan;
    Material matPent = LoadMaterialDefault();
    matPent.shader = shPent;

    //Rectangle panelBounds = (Rectangle){0, 0, 200, 600};
    Rectangle canvasSource = (Rectangle){0, 0, (float)target.texture.width, -(float)target.texture.height};
    Rectangle canvasDest = (Rectangle){0, 0, 600, 600};

    int currentShape = 0;

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_ONE)) currentShape = 0;
        if (IsKeyPressed(KEY_TWO)) currentShape = 1;
        if (IsKeyPressed(KEY_THREE)) currentShape = 2;

        BeginTextureMode(target);
        ClearBackground((Color){30, 30, 30, 255});

        if (currentShape == 0) DrawMesh(quadMesh, matQuad, MatrixIdentity());
        else if (currentShape == 1) DrawMesh(fanMesh, matFan, MatrixIdentity());
        else DrawMesh(pentMesh, matPent, MatrixIdentity());

        EndTextureMode();

        BeginDrawing();
        ClearBackground(RAYWHITE);

        //DrawRectangleRec(panelBounds, (Color){230, 230, 230, 255});
        //DrawText("SHAPES", 20, 20, 20, BLACK);

        //if (GuiButton((Rectangle){20, 80, 160, 30}, "QUAD")) currentShape = 0;
        //if (GuiButton((Rectangle){20, 120, 160, 30}, "FAN")) currentShape = 1;
        //if (GuiButton((Rectangle){20, 160, 160, 30}, "PENTAGON")) currentShape = 2;

        DrawTexturePro(target.texture, canvasSource, canvasDest, (Vector2){0, 0}, 0.0f, WHITE);

        EndDrawing();
    }

    UnloadRenderTexture(target);

    UnloadMesh(quadMesh);
    UnloadMesh(fanMesh);
    UnloadMesh(pentMesh);

    MemFree(quadMesh.vertices);
    MemFree(fanMesh.vertices);
    MemFree(pentMesh.vertices);

    UnloadMaterial(matQuad);
    UnloadMaterial(matFan);
    UnloadMaterial(matPent);

    UnloadShader(shQuad);
    UnloadShader(shFan);
    UnloadShader(shPent);

    CloseWindow();

    return 0;
}

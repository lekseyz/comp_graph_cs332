#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>

struct SceneObject {
    Model model;
    Texture2D texture;
    Vector3 position;
    float scale;
    Color color;
};

int main()
{
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "C++ Raylib: Scene with Shaders");

    Camera camera = { 0 };
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    Shader shader = LoadShader("shaders/shader.vs", "shaders/shader.fs");

    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    int lightPosLoc = GetShaderLocation(shader, "lightPos");
    int ambientLoc = GetShaderLocation(shader, "ambientColor");

    Vector3 lightPos = { 5.0f, 10.0f, 5.0f };
    float ambientColor[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
    SetShaderValue(shader, lightPosLoc, &lightPos, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, ambientLoc, ambientColor, SHADER_UNIFORM_VEC4);

    std::vector<SceneObject> objects;

    std::string modelNames[] = { "cottage", "bus", "cat", "bird", "toilet" };
    
    Vector3 positions[] = {
        { 0.0f, 0.0f, 0.0f },
        { -8.0f, 0.5f, 5.0f },
        { 3.0f, 0.0f, 4.0f },
        { -2.0f, 0.0f, -5.0f },
        { 2.5f, 1.0f, -8.5f }
    };

    float rotates[] = {
        .0f,
        .0f,
        -90.0f,
        -90.0f,
        -90.0f
    };

    float scales[] = {
        .3f,
        .5f,
        .02f,
        .02f,
        .06f
    };

    for (int i = 0; i < 5; i++) {
        SceneObject obj;
        std::string objPath = "models/" + modelNames[i] + ".obj";
        std::string texPath = "models/" + modelNames[i] + ".png";

        obj.model = LoadModel(objPath.c_str()); 
        obj.texture = LoadTexture(texPath.c_str());
    
        obj.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = obj.texture;
        
        obj.model.materials[0].shader = shader;

        obj.position = positions[i];
        obj.scale = scales[i];
        obj.color = WHITE;
        obj.model.transform = MatrixRotateX(rotates[i] * DEG2RAD);
        objects.push_back(obj);
    }

    Model floor = LoadModelFromMesh(GenMeshPlane(20.0f, 20.0f, 10, 10));
    floor.materials[0].shader = shader; // Полу тоже нужен шейдер


    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode3D(camera);
                
                DrawModel(floor, (Vector3){0, -0.01f, 0}, 1.0f, WHITE);

                for (const auto& obj : objects) {
                    DrawModel(obj.model, obj.position, obj.scale, obj.color);
                }

                DrawSphere(lightPos, 0.2f, YELLOW);
                DrawGrid(20, 1.0f);

            EndMode3D();

            DrawText("Scene with 5 Objects & Custom Shaders", 10, 10, 20, BLACK);
            DrawText("Use Mouse to Rotate Camera", 10, 40, 20, DARKGRAY);
            DrawFPS(10, 70);

        EndDrawing();
    }

    for (auto& obj : objects) {
        UnloadTexture(obj.texture);
        UnloadModel(obj.model);
    }
    UnloadModel(floor);
    UnloadShader(shader);

    CloseWindow();

    return 0;
}
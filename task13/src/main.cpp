#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <filesystem>
#include <map>
#include <cmath>

namespace fs = std::filesystem;

const char* instancingVs = R"(
#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in mat4 instanceTransform;

uniform mat4 mvp;

out vec2 fragTexCoord;
out vec3 fragNormal;

void main() {
    fragTexCoord = vertexTexCoord;
    fragNormal = normalize(vec3(instanceTransform * vec4(vertexNormal, 0.0)));
    gl_Position = mvp * instanceTransform * vec4(vertexPosition, 1.0);
}
)";

const char* instancingFs = R"(
#version 330
in vec2 fragTexCoord;
in vec3 fragNormal;

uniform sampler2D texture0;

out vec4 finalColor;

void main() {
    vec3 lightDir = normalize(vec3(1.0, 2.0, 3.0));
    float diff = max(dot(fragNormal, lightDir), 0.2);
    vec4 texColor = texture(texture0, fragTexCoord);
    finalColor = vec4(texColor.rgb * diff, texColor.a);
}
)";

struct Planet {
    int modelIndex;
    float distance;
    float orbitSpeed;
    float rotationSpeed;
    float scale;
    Vector3 axis;
};

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "Solar System - Shader Instancing");

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 30.0f, 30.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Shader shader = LoadShaderFromMemory(instancingVs, instancingFs);
    int loc = GetShaderLocation(shader, "instanceTransform");
    
    std::vector<Model> models;
    std::string modelsDir = "models";
    
    Image checkedImg = GenImageChecked(512, 512, 32, 32, PURPLE, BLACK);
    Texture2D defaultTexture = LoadTextureFromImage(checkedImg);
    UnloadImage(checkedImg);

    if (fs::exists(modelsDir) && fs::is_directory(modelsDir)) {
        for (const auto& entry : fs::directory_iterator(modelsDir)) {
            if (entry.path().extension() == ".obj") {
                Model model = LoadModel(entry.path().string().c_str());
                model.materials[0].shader = shader;
                model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = defaultTexture;
                models.push_back(model);
            }
        }
    }

    if (models.empty()) {
        Model fallback = LoadModelFromMesh(GenMeshKnot(1.0f, 3.0f, 64, 128));
        fallback.materials[0].shader = shader;
        fallback.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = defaultTexture;
        models.push_back(fallback);
    }

    std::vector<Planet> solarSystem;
    int systemSize = 50;

    for (int i = 0; i < systemSize; i++) {
        Planet p;
        p.modelIndex = GetRandomValue(0, (int)models.size() - 1);
        p.distance = (i == 0) ? 0.0f : (float)(i * 2 + GetRandomValue(2, 5));
        p.orbitSpeed = (i == 0) ? 0.0f : (1.0f / p.distance) * 5.0f;
        p.rotationSpeed = (float)GetRandomValue(1, 10) / 10.0f;
        p.scale = (i == 0) ? 4.0f : (float)GetRandomValue(5, 12) / 10.0f;
        p.axis = Vector3Normalize((Vector3){(float)GetRandomValue(-10, 10), (float)GetRandomValue(-10, 10), (float)GetRandomValue(-10, 10)});
        solarSystem.push_back(p);
    }

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        float time = (float)GetTime();

        std::map<int, std::vector<Matrix>> instances;

        for (const auto& planet : solarSystem) {
            float x = cosf(time * planet.orbitSpeed) * planet.distance;
            float z = sinf(time * planet.orbitSpeed) * planet.distance;
            
            Vector3 position = { x, 0.0f, z };
            if (planet.distance == 0.0f) position = Vector3Zero();

            Matrix matScale = MatrixScale(planet.scale, planet.scale, planet.scale);
            Matrix matRot = MatrixRotate(planet.axis, time * planet.rotationSpeed);
            Matrix matTrans = MatrixTranslate(position.x, position.y, position.z);
            
            Matrix transform = MatrixMultiply(MatrixMultiply(matScale, matRot), matTrans);
            
            instances[planet.modelIndex].push_back(transform);
        }

        BeginDrawing();
        ClearBackground(DARKBLUE);

        BeginMode3D(camera);

        for (auto const& [modelIdx, transforms] : instances) {
            if (!transforms.empty()) {
                DrawMeshInstanced(models[modelIdx].meshes[0], models[modelIdx].materials[0], transforms.data(), (int)transforms.size());
            }
        }

        DrawGrid(50, 2.0f);

        EndMode3D();

        DrawFPS(10, 10);
        EndDrawing();
    }

    for (auto& model : models) UnloadModel(model);
    UnloadTexture(defaultTexture);
    UnloadShader(shader);
    CloseWindow();

    return 0;
}
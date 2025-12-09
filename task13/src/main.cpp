#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <filesystem>
#include <map>
#include <cmath>
#include <iostream>

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

Texture2D LoadTextureForModel(const fs::path& modelPath, const Texture2D& defaultTexture) {
    fs::path basePath = modelPath;
    basePath.replace_extension();

    std::vector<std::string> textureExtensions = { ".png", ".jpg", ".jpeg", ".bmp", ".tga" };

    for (const auto& ext : textureExtensions) {
        fs::path texturePath = basePath;
        texturePath.replace_extension(ext);

        if (fs::exists(texturePath)) {
            std::cout << "[INFO] Texture found: " << texturePath.string() << std::endl;
            return LoadTexture(texturePath.string().c_str());
        }
    }
    return defaultTexture;
}

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Solar System - Auto Loading");

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 60.0f, 60.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Shader shader = LoadShaderFromMemory(instancingVs, instancingFs);
    shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(shader, "instanceTransform");

    std::vector<Model> models;
    std::vector<Texture2D> textures;

    Image checkedImg = GenImageChecked(512, 512, 32, 32, PURPLE, BLACK);
    Texture2D defaultTexture = LoadTextureFromImage(checkedImg);
    UnloadImage(checkedImg);

    std::string modelsDirName = "models";
    std::cout << "SCANNING FOLDER: " << modelsDirName << std::endl;

    if (fs::exists(modelsDirName) && fs::is_directory(modelsDirName)) {
        for (const auto& entry : fs::directory_iterator(modelsDirName)) {
            fs::path filePath = entry.path();

            std::string ext = filePath.extension().string();
            if (ext == ".obj" || ext == ".OBJ") {

                std::cout << "[LOADING] Found: " << filePath.filename().string() << " ... ";

                Model model = LoadModel(filePath.string().c_str());

                if (model.meshCount > 0) {
                    model.materials[0].shader = shader;

                    Texture2D tex = LoadTextureForModel(filePath, defaultTexture);
                    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = tex;

                    models.push_back(model);

                    if (tex.id != defaultTexture.id) textures.push_back(tex);

                    std::cout << "SUCCESS!" << std::endl;
                }
                else {
                    std::cout << "FAILED (Empty mesh)" << std::endl;
                }
            }
        }
    }
    else {
        std::cout << "[ERROR] Folder 'models' not found!" << std::endl;
        std::cout << "Make sure you created 'models' folder next to the executable." << std::endl;
    }

    if (models.empty()) {
        std::cout << "\n[WARN] No models found! Using fallback Cube." << std::endl;
        Model fallback = LoadModelFromMesh(GenMeshCube(2.0f, 2.0f, 2.0f));
        fallback.materials[0].shader = shader;
        fallback.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = defaultTexture;
        models.push_back(fallback);
    }

    std::vector<Planet> solarSystem;
    int systemSize = 100;

    for (int i = 0; i < systemSize; i++) {
        Planet p;
        p.modelIndex = GetRandomValue(0, (int)models.size() - 1);

        p.distance = (i == 0) ? 0.0f : (float)(i * 1.5f + 5.0f);
        p.orbitSpeed = (i == 0) ? 0.0f : (1.0f / (p.distance + 1.0f)) * 15.0f;
        p.rotationSpeed = (float)GetRandomValue(1, 30) / 10.0f;
        p.scale = (i == 0) ? 3.0f : (float)GetRandomValue(5, 15) / 10.0f;

        p.axis = Vector3Normalize((Vector3) {
            (float)GetRandomValue(-10, 10),
                (float)GetRandomValue(-10, 10),
                (float)GetRandomValue(-10, 10)
        });

        solarSystem.push_back(p);
    }

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_ORBITAL);

        float time = (float)GetTime();

        std::map<int, std::vector<Matrix>> instances;

        for (const auto& planet : solarSystem) {
            float angle = time * planet.orbitSpeed;
            float x = cosf(angle) * planet.distance;
            float z = sinf(angle) * planet.distance;

            Matrix matScale = MatrixScale(planet.scale, planet.scale, planet.scale);
            Matrix matRot = MatrixRotate(planet.axis, time * planet.rotationSpeed);
            Matrix matTrans = MatrixTranslate(x, 0.0f, z);

            Matrix transform = MatrixMultiply(MatrixMultiply(matScale, matRot), matTrans);

            instances[planet.modelIndex].push_back(transform);
        }

        BeginDrawing();
        ClearBackground(DARKBLUE);

        BeginMode3D(camera);
        DrawGrid(100, 5.0f);

        for (auto const& [modelIdx, transforms] : instances) {
            if (!transforms.empty()) {
                DrawMeshInstanced(
                    models[modelIdx].meshes[0],
                    models[modelIdx].materials[0],
                    transforms.data(),
                    (int)transforms.size()
                );
            }
        }
        EndMode3D();

        DrawFPS(10, 10);
        DrawText(TextFormat("Total Objects: %d", systemSize), 10, 40, 20, GREEN);
        DrawText(TextFormat("Unique Models: %d", (int)models.size()), 10, 65, 20, GREEN);

        EndDrawing();
    }

    for (auto& model : models) UnloadModel(model);
    for (auto& tex : textures) UnloadTexture(tex);
    UnloadTexture(defaultTexture);
    UnloadShader(shader);

    CloseWindow();

    return 0;
}

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>

#define MAX_LIGHTS 4

enum LightType {
    LIGHT_POINT = 0,
    LIGHT_DIRECTIONAL = 1,
    LIGHT_SPOT = 2
};

struct LightSource {
    LightType type;
    Vector3 position;
    Vector3 direction;
    Color color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
    bool enabled;
};

struct SceneObject {
    Model model;
    Texture2D texture;
    Vector3 position;
    float scale;
    Color color;
};

void SetShaderLight(Shader shader, int index, LightSource light)
{
    char locName[128];
    sprintf(locName, "lights[%d].type", index);
    SetShaderValue(shader, GetShaderLocation(shader, locName), &light.type, SHADER_UNIFORM_INT);

    sprintf(locName, "lights[%d].position", index);
    float pos[3] = { light.position.x, light.position.y, light.position.z };
    SetShaderValue(shader, GetShaderLocation(shader, locName), pos, SHADER_UNIFORM_VEC3);

    sprintf(locName, "lights[%d].direction", index);
    float dir[3] = { light.direction.x, light.direction.y, light.direction.z };
    SetShaderValue(shader, GetShaderLocation(shader, locName), dir, SHADER_UNIFORM_VEC3);

    sprintf(locName, "lights[%d].color", index);
    float col[3] = { light.color.r / 255.0f, light.color.g / 255.0f, light.color.b / 255.0f };
    SetShaderValue(shader, GetShaderLocation(shader, locName), col, SHADER_UNIFORM_VEC3);

    sprintf(locName, "lights[%d].intensity", index);
    SetShaderValue(shader, GetShaderLocation(shader, locName), &light.intensity, SHADER_UNIFORM_FLOAT);

    sprintf(locName, "lights[%d].constant", index);
    SetShaderValue(shader, GetShaderLocation(shader, locName), &light.constant, SHADER_UNIFORM_FLOAT);

    sprintf(locName, "lights[%d].linear", index);
    SetShaderValue(shader, GetShaderLocation(shader, locName), &light.linear, SHADER_UNIFORM_FLOAT);

    sprintf(locName, "lights[%d].quadratic", index);
    SetShaderValue(shader, GetShaderLocation(shader, locName), &light.quadratic, SHADER_UNIFORM_FLOAT);

    sprintf(locName, "lights[%d].cutOff", index);
    SetShaderValue(shader, GetShaderLocation(shader, locName), &light.cutOff, SHADER_UNIFORM_FLOAT);

    sprintf(locName, "lights[%d].outerCutOff", index);
    SetShaderValue(shader, GetShaderLocation(shader, locName), &light.outerCutOff, SHADER_UNIFORM_FLOAT);
}

int main()
{
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Raylib: Camera");

    Camera camera = { 0 };
    camera.position = (Vector3){ 0.0f, 2.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    float cameraYaw = 0.0f;
    float cameraPitch = 0.0f;
    float mouseSensitivity = 0.003f;
    float moveSpeed = 0.15f;

    DisableCursor();

    Shader shader = LoadShader("shaders/shader.vs", "shaders/shader.fs");
    shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(shader, "mvp");
    shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader, "matModel");
    shader.locs[SHADER_LOC_MATRIX_NORMAL] = GetShaderLocation(shader, "matNormal");
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

    int ambientLoc = GetShaderLocation(shader, "ambient");
    int shininessLoc = GetShaderLocation(shader, "shininess");
    int specularStrengthLoc = GetShaderLocation(shader, "specularStrength");
    int lightsCountLoc = GetShaderLocation(shader, "lightsCount");

    float ambient[3] = { 0.2f, 0.2f, 0.2f };
    float shininess = 32.0f;
    float specularStrength = 0.5f;

    SetShaderValue(shader, ambientLoc, ambient, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, shininessLoc, &shininess, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, specularStrengthLoc, &specularStrength, SHADER_UNIFORM_FLOAT);

    std::vector<LightSource> lights;

    LightSource pointLight;
    pointLight.type = LIGHT_POINT;
    pointLight.position = (Vector3){ 5.0f, 8.0f, 5.0f };
    pointLight.direction = (Vector3){ 0.0f, 0.0f, 0.0f };
    pointLight.color = YELLOW;
    pointLight.intensity = 1.0f;
    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;
    pointLight.cutOff = 0.0f;
    pointLight.outerCutOff = 0.0f;
    pointLight.enabled = true;
    lights.push_back(pointLight);

    LightSource dirLight;
    dirLight.type = LIGHT_DIRECTIONAL;
    dirLight.position = (Vector3){ 0.0f, 0.0f, 0.0f };
    dirLight.direction = Vector3Normalize((Vector3) { -0.5f, -1.0f, -0.3f });
    dirLight.color = (Color){ 255, 240, 220, 255 };
    dirLight.intensity = 0.3f;
    dirLight.constant = 1.0f;
    dirLight.linear = 0.0f;
    dirLight.quadratic = 0.0f;
    dirLight.cutOff = 0.0f;
    dirLight.outerCutOff = 0.0f;
    dirLight.enabled = true;
    lights.push_back(dirLight);

    LightSource spotLight;
    spotLight.type = LIGHT_SPOT;
    spotLight.position = (Vector3){ 0.0f, 10.0f, 0.0f };
    spotLight.direction = Vector3Normalize((Vector3) { 0.0f, -1.0f, 0.0f });
    spotLight.color = (Color){ 100, 150, 255, 255 };
    spotLight.intensity = 2.0f;
    spotLight.constant = 1.0f;
    spotLight.linear = 0.09f;
    spotLight.quadratic = 0.032f;
    spotLight.cutOff = cosf(12.5f * DEG2RAD);
    spotLight.outerCutOff = cosf(17.5f * DEG2RAD);
    spotLight.enabled = true;
    lights.push_back(spotLight);

    std::vector<SceneObject> objects;
    std::string modelNames[] = { "cottage", "bus", "cat", "bird", "toilet" };
    Vector3 positions[] = { {0,0,0}, {-8,0.5f,5}, {3,0,4}, {-2,0,-5}, {2.5f,1,-8.5f} };
    float rotates[] = { 0, 0, -90, -90, -90 };
    float scales[] = { 0.3f, 0.5f, 0.02f, 0.02f, 0.06f };

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
    floor.materials[0].shader = shader;

    SetTargetFPS(60);

    int currentLight = 0;
    bool showHelp = true;

    while (!WindowShouldClose())
    {

        Vector2 mouseDelta = GetMouseDelta();
        cameraYaw += mouseDelta.x * -mouseSensitivity;
        cameraPitch += mouseDelta.y * -mouseSensitivity;

        if (cameraPitch > 89.0f * DEG2RAD) cameraPitch = 89.0f * DEG2RAD;
        if (cameraPitch < -89.0f * DEG2RAD) cameraPitch = -89.0f * DEG2RAD;

        Vector3 front;
        front.x = sinf(cameraYaw) * cosf(cameraPitch);
        front.y = sinf(cameraPitch);
        front.z = cosf(cameraYaw) * cosf(cameraPitch);
        front = Vector3Normalize(front);

        Vector3 right = Vector3CrossProduct(front, (Vector3) { 0, 1, 0 });
        right = Vector3Normalize(right);

        if (IsKeyDown(KEY_W)) camera.position = Vector3Add(camera.position, Vector3Scale(front, moveSpeed));
        if (IsKeyDown(KEY_S)) camera.position = Vector3Subtract(camera.position, Vector3Scale(front, moveSpeed));
        if (IsKeyDown(KEY_D)) camera.position = Vector3Add(camera.position, Vector3Scale(right, moveSpeed));
        if (IsKeyDown(KEY_A)) camera.position = Vector3Subtract(camera.position, Vector3Scale(right, moveSpeed));

        if (IsKeyDown(KEY_E)) camera.position.y += moveSpeed;
        if (IsKeyDown(KEY_Q)) camera.position.y -= moveSpeed;

        camera.target = Vector3Add(camera.position, front);

        if (IsKeyPressed(KEY_Z)) {
            if (IsCursorHidden()) EnableCursor();
            else DisableCursor();
        }

        if (IsKeyPressed(KEY_ONE)) currentLight = 0;
        if (IsKeyPressed(KEY_TWO)) currentLight = 1;
        if (IsKeyPressed(KEY_THREE)) currentLight = 2;
        if (IsKeyPressed(KEY_SPACE)) if (currentLight < (int)lights.size()) lights[currentLight].enabled = !lights[currentLight].enabled;

        if (IsKeyDown(KEY_UP) && currentLight < (int)lights.size()) lights[currentLight].intensity += 0.01f;
        if (IsKeyDown(KEY_DOWN) && currentLight < (int)lights.size()) lights[currentLight].intensity = fmaxf(0.0f, lights[currentLight].intensity - 0.01f);

        if (currentLight < (int)lights.size() && lights[currentLight].type == LIGHT_POINT) {
            float lightSpeed = 0.1f;
            if (IsKeyDown(KEY_I)) lights[currentLight].position.z -= lightSpeed;
            if (IsKeyDown(KEY_K)) lights[currentLight].position.z += lightSpeed;
            if (IsKeyDown(KEY_J)) lights[currentLight].position.x -= lightSpeed;
            if (IsKeyDown(KEY_L)) lights[currentLight].position.x += lightSpeed;
            if (IsKeyDown(KEY_U)) lights[currentLight].position.y += lightSpeed;
            if (IsKeyDown(KEY_O)) lights[currentLight].position.y -= lightSpeed;
        }

        if (currentLight < (int)lights.size() && lights[currentLight].type == LIGHT_SPOT) {
            if (IsKeyDown(KEY_LEFT)) {
                float angle = acosf(lights[currentLight].cutOff) + 0.5f * DEG2RAD;
                lights[currentLight].cutOff = cosf(fminf(angle, 45.0f * DEG2RAD));
                lights[currentLight].outerCutOff = cosf(fminf(angle + 5.0f * DEG2RAD, 50.0f * DEG2RAD));
            }
            if (IsKeyDown(KEY_RIGHT)) {
                float angle = acosf(lights[currentLight].cutOff) - 0.5f * DEG2RAD;
                lights[currentLight].cutOff = cosf(fmaxf(angle, 5.0f * DEG2RAD));
                lights[currentLight].outerCutOff = cosf(fmaxf(angle + 5.0f * DEG2RAD, 10.0f * DEG2RAD));
            }
        }

        if (IsKeyPressed(KEY_H)) showHelp = !showHelp;

        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        int activeCount = 0;
        for (int i = 0; i < (int)lights.size() && i < MAX_LIGHTS; i++) {
            if (lights[i].enabled) {
                SetShaderLight(shader, activeCount, lights[i]);
                activeCount++;
            }
        }
        SetShaderValue(shader, lightsCountLoc, &activeCount, SHADER_UNIFORM_INT);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        DrawModel(floor, (Vector3) { 0, -0.01f, 0 }, 1.0f, WHITE);
        for (const auto& obj : objects) DrawModel(obj.model, obj.position, obj.scale, obj.color);

        for (int i = 0; i < (int)lights.size(); i++) {
            if (!lights[i].enabled) continue;
            if (lights[i].type == LIGHT_POINT) DrawSphere(lights[i].position, 0.3f, lights[i].color);
            else if (lights[i].type == LIGHT_SPOT) {
                DrawSphere(lights[i].position, 0.3f, lights[i].color);
                Vector3 endPos = Vector3Add(lights[i].position, Vector3Scale(lights[i].direction, 2.0f));
                DrawLine3D(lights[i].position, endPos, lights[i].color);
            }
        }
        DrawGrid(20, 1.0f);
        EndMode3D();

        DrawText(" Camera Control", 10, 10, 20, BLACK);
        DrawFPS(10, 40);

        if (showHelp) {
            DrawRectangle(10, 70, 420, 320, Fade(SKYBLUE, 0.8f));
            DrawText("Controls (H - help, Z - unlock mouse):", 20, 80, 16, BLACK);

            DrawText("CAMERA:", 20, 110, 14, DARKBLUE);
            DrawText("MOUSE   - Look Around", 20, 130, 14, BLACK);
            DrawText("W / S   - Fly Forward / Backward", 20, 150, 14, BLACK);
            DrawText("A / D   - Fly Left / Right", 20, 170, 14, BLACK);
            DrawText("E / Q   - Fly Up / Down", 20, 190, 14, BLACK);

            DrawText("LIGHT CONTROL:", 20, 220, 14, DARKBLUE);
            DrawText("I/K/J/L - Move Light (XZ plane)", 20, 240, 14, DARKGRAY);
            DrawText("U / O   - Move Light Up/Down", 20, 260, 14, DARKGRAY);
            DrawText("1,2,3   - Select Light | SPACE - Toggle", 20, 280, 14, DARKGRAY);
            DrawText("Arrows  - Intensity & Spot Angle", 20, 300, 14, DARKGRAY);

            if (currentLight < (int)lights.size()) {
                const char* lightTypes[] = { "Point", "Directional", "Spot" };
                DrawText(TextFormat("Selected: %s | Int: %.2f", lightTypes[currentLight], lights[currentLight].intensity),
                    20, 340, 14, RED);
            }
        }
        EndDrawing();
    }

    for (auto& obj : objects) { UnloadTexture(obj.texture); UnloadModel(obj.model); }
    UnloadModel(floor);
    UnloadShader(shader);
    CloseWindow();
    return 0;
}
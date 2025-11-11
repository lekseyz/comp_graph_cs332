#define RAYGUI_IMPLEMENTATION
#include "raylib.h"
#include "raygui.h"
#include "raymath.h"
#include "rlgl.h"
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <cmath>

// ================================
// ===  Структуры и функции     ===
// ================================

struct Obj3D {
    Model model;
    Vector3 pos{0,0,0};
    Vector3 rot{0,0,0};
    float scale{1.0f};
    Color color{200,200,220,255};
    bool valid{false};
};

struct CameraObj {
    Vector3 position{0,0,0};
    Vector3 target{0,0,0};
    Vector3 direction{0,0,1};
    Matrix view;
    Matrix proj;
    float fovy = 60.0f;
    float nearZ = 0.1f;
    float farZ = 100.0f;

    void UpdateFromCamera3D(const Camera3D& c, int screenW, int screenH) {
        position = c.position;
        target = c.target;
        direction = Vector3Normalize(Vector3Subtract(target, position));
        view = GetCameraMatrix(c);
        float aspect = (float)screenW / (float)screenH;
        proj = MatrixPerspective(DEG2RAD * fovy, aspect, nearZ, farZ);
    }
};

static Vector3 GetVertex(const Mesh& m, int idx) {
    int i = idx * 3;
    return {m.vertices[i], m.vertices[i+1], m.vertices[i+2]};
}

static Vector3 TransformVec(Vector3 v, Matrix M) {
    return Vector3Transform(v, M);
}

static void DrawCulledMesh(const Mesh& mesh, Matrix objM, const Camera3D& cam, Color col, bool showBack) {
    rlDisableBackfaceCulling();
    rlBegin(RL_TRIANGLES);
    int tcount = mesh.triangleCount;
    bool hasIdx = mesh.indices != nullptr;
    for (int t = 0; t < tcount; t++) {
        int i1,i2,i3;
        if (hasIdx) {
            i1 = mesh.indices[t*3+0];
            i2 = mesh.indices[t*3+1];
            i3 = mesh.indices[t*3+2];
        } else {
            i1 = t*3+0; i2 = t*3+1; i3 = t*3+2;
        }

        Vector3 v1 = TransformVec(GetVertex(mesh,i1), objM);
        Vector3 v2 = TransformVec(GetVertex(mesh,i2), objM);
        Vector3 v3 = TransformVec(GetVertex(mesh,i3), objM);

        Vector3 e1 = Vector3Subtract(v2, v1);
        Vector3 e2 = Vector3Subtract(v3, v1);
        Vector3 n = Vector3Normalize(Vector3CrossProduct(e1, e2));

        Vector3 lookVec = Vector3Subtract(v1, cam.position);
        if (Vector3DotProduct(n, lookVec) < 0.0f) {
            rlColor4ub(col.r, col.g, col.b, col.a);
            rlVertex3f(v1.x, v1.y, v1.z);
            rlVertex3f(v2.x, v2.y, v2.z);
            rlVertex3f(v3.x, v3.y, v3.z);
        } else if (showBack) {
            rlColor4ub(60, 60, 60, 80);
            rlVertex3f(v1.x, v1.y, v1.z);
            rlVertex3f(v2.x, v2.y, v2.z);
            rlVertex3f(v3.x, v3.y, v3.z);
        }
    }
    rlEnd();
}

static void DrawObjCulled(const Obj3D& obj, const Camera3D& cam, bool showBack) {
    Matrix S = MatrixScale(obj.scale, obj.scale, obj.scale);
    Matrix R = MatrixRotateXYZ({obj.rot.x, obj.rot.y, obj.rot.z});
    Matrix T = MatrixTranslate(obj.pos.x, obj.pos.y, obj.pos.z);
    Matrix M = MatrixMultiply(MatrixMultiply(S, R), T);
    for (int i = 0; i < obj.model.meshCount; i++)
        DrawCulledMesh(obj.model.meshes[i], M, cam, obj.color, showBack);
}

// ================================
// ===           MAIN           ===
// ================================

int main() {
    InitWindow(1280, 800, "Camera Mode: Static / Orbit");
    SetTargetFPS(60);

    // Raylib камера
    Camera3D cam = {0};
    cam.position = {6.0f, 3.0f, 6.0f};
    cam.target = {0.0f, 0.0f, 0.0f};
    cam.up = {0.0f, 1.0f, 0.0f};
    cam.fovy = 60.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    // Объект-камера
    CameraObj objCam;
    objCam.fovy = cam.fovy;

    // Загружаем файлы .obj
    std::vector<std::string> objFiles;
    for (auto &p : std::filesystem::directory_iterator("figures")) {
        if (p.path().extension() == ".obj") objFiles.push_back(p.path().string());
    }

    std::vector<Obj3D> scene;
    int selected = -1;
    bool moveMode = false;
    bool showBack = false;
    bool orbitMode = false;
    bool camControlEnabled = true;  // <-- добавлено
    float camMoveSpeed = 5.0f;      // <-- добавлено

    float camOrbitRadius = 6.0f;
    float camAngle = 0.0f;
    float camSpeed = 0.6f;

    RenderTexture2D rtex = LoadRenderTexture(800, 600);

    int panelW = 280;

    auto addObj = [&](const std::string& path){
        Obj3D o;
        o.model = LoadModel(path.c_str());
        o.valid = true;
        o.scale = 1.0f;
        o.color = SKYBLUE;
        o.pos = {(float)scene.size()*2.0f, 0.0f, 0.0f};
        scene.push_back(o);
        selected = (int)scene.size()-1;
    };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        int sw = GetScreenWidth(), sh = GetScreenHeight();
        Rectangle panel = {(float)(sw - panelW), 0, (float)panelW, (float)sh};

        // ОБНОВЛЕНИЕ КАМЕРЫ И УПРАВЛЕНИЕ
        if (orbitMode) {
            // Вращающаяся камера вокруг статичного объекта
            camAngle += camSpeed * dt;
            Vector3 center = {0,0,0};
            if (selected >= 0 && selected < (int)scene.size()) center = scene[selected].pos;

            cam.position = {
                center.x + camOrbitRadius * sinf(camAngle),
                center.y + 2.0f,
                center.z + camOrbitRadius * cosf(camAngle)
            };
            cam.target = center;
            cam.up = {0,1,0};

            // Вручную задаем матрицу проекции (только для вращающейся)
            objCam.UpdateFromCamera3D(cam, rtex.texture.width, rtex.texture.height);
        } else {
            // Статичная камера, вращается объект
            for (auto &o : scene)
                o.rot.y += 1.0f * dt;
            
            // WASD УПРАВЛЕНИЕ ДЛЯ СТАТИЧЕСКОЙ КАМЕРЫ
            if (camControlEnabled) {
                // Вычисляем направление камеры правильно
                Vector3 camDirection = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
                Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camDirection, cam.up));
                Vector3 camUp = cam.up;
                
                if (IsKeyDown(KEY_W)) {
                    cam.position = Vector3Add(cam.position, Vector3Scale(camDirection, camMoveSpeed * dt));
                    cam.target = Vector3Add(cam.target, Vector3Scale(camDirection, camMoveSpeed * dt));
                }
                if (IsKeyDown(KEY_S)) {
                    cam.position = Vector3Subtract(cam.position, Vector3Scale(camDirection, camMoveSpeed * dt));
                    cam.target = Vector3Subtract(cam.target, Vector3Scale(camDirection, camMoveSpeed * dt));
                }
                if (IsKeyDown(KEY_A)) {
                    cam.position = Vector3Subtract(cam.position, Vector3Scale(camRight, camMoveSpeed * dt));
                    cam.target = Vector3Subtract(cam.target, Vector3Scale(camRight, camMoveSpeed * dt));
                }
                if (IsKeyDown(KEY_D)) {
                    cam.position = Vector3Add(cam.position, Vector3Scale(camRight, camMoveSpeed * dt));
                    cam.target = Vector3Add(cam.target, Vector3Scale(camRight, camMoveSpeed * dt));
                }
                if (IsKeyDown(KEY_Q)) { // Вверх
                    cam.position = Vector3Add(cam.position, Vector3Scale(camUp, camMoveSpeed * dt));
                    cam.target = Vector3Add(cam.target, Vector3Scale(camUp, camMoveSpeed * dt));
                }
                if (IsKeyDown(KEY_E)) { // Вниз
                    cam.position = Vector3Subtract(cam.position, Vector3Scale(camUp, camMoveSpeed * dt));
                    cam.target = Vector3Subtract(cam.target, Vector3Scale(camUp, camMoveSpeed * dt));
                }
            }
        }

        // --- Рендер сцены в текстуру ---
        BeginTextureMode(rtex);
            ClearBackground({18,18,24,255});
            BeginMode3D(cam);
                DrawGrid(20, 1.0f);
                for (int i = 0; i < (int)scene.size(); i++)
                    DrawObjCulled(scene[i], cam, showBack);
                if (selected >= 0 && selected < (int)scene.size())
                    DrawSphereWires(scene[selected].pos, 0.15f, 10, 10, GOLD);
            EndMode3D();
        EndTextureMode();

        // --- Отрисовка интерфейса ---
        BeginDrawing();
        ClearBackground({18,18,24,255});

        DrawTexturePro(rtex.texture,
            {0,0,(float)rtex.texture.width,(float)-rtex.texture.height},
            {16,16,640,480},{0,0},0,WHITE);

        // информация о камере
        DrawText(TextFormat("Mode: %s", orbitMode ? "ORBIT CAMERA" : "STATIC CAMERA"), 16, 500, 18, YELLOW);
        DrawText(TextFormat("Camera pos: [%.2f, %.2f, %.2f]", cam.position.x, cam.position.y, cam.position.z), 16, 522, 14, RAYWHITE);
        if (!orbitMode) {
            DrawText("WASD/QE - Move camera", 16, 544, 14, GREEN);
        }
        if (orbitMode) {
            DrawText("Projection matrix (manual):", 16, 540, 14, LIGHTGRAY);
            DrawText(TextFormat("[%.3f %.3f %.3f %.3f]", objCam.proj.m0, objCam.proj.m1, objCam.proj.m2, objCam.proj.m3), 16, 558, 12, LIGHTGRAY);
        }

        // ---- GUI панель ----
        GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
        GuiPanel(panel, nullptr);

        float x = panel.x + 16;
        float y = 16;
        float w = panel.width - 32;
        float h = 44;

        // Кнопки
        if (GuiButton({x,y,w,h}, "ADD OBJ")) {
            if (!objFiles.empty()) addObj(objFiles[scene.size() % objFiles.size()]);
        }
        y += h + 12;

        GuiToggle({x,y,w,h}, "MOVE WASD", &camControlEnabled); y += h + 12; // <-- изменено на camControlEnabled
        GuiToggle({x,y,w,h}, "TEST BACKFACE", &showBack); y += h + 12;
        GuiToggle({x,y,w,h}, "CAMERA ORBIT", &orbitMode); y += h + 12;

        if (GuiButton({x,y,w,h}, "CLEAR")) {
            for (auto &o : scene) if (o.valid) UnloadModel(o.model);
            scene.clear();
            selected = -1;
        }
        y += h + 18;

        GuiLabel({x,y,w,28}, TextFormat("OBJs: %d", (int)scene.size())); y += 30;
        GuiLabel({x,y,w,28}, TextFormat("CHOSEN: %d", selected)); y += 30;

        // Добавьте слайдеры для настройки
        GuiSlider({x,y,w,h}, "MOVE SPEED", TextFormat("%.1f", camMoveSpeed), &camMoveSpeed, 1.0f, 20.0f); y += h + 12;
        if (orbitMode) {
            GuiSlider({x,y,w,h}, "ORBIT RADIUS", TextFormat("%.1f", camOrbitRadius), &camOrbitRadius, 2.0f, 20.0f); y += h + 12;
            GuiSlider({x,y,w,h}, "ORBIT SPEED", TextFormat("%.1f", camSpeed), &camSpeed, 0.1f, 3.0f); y += h + 12;
        }

        DrawFPS(10,10);
        EndDrawing();
    }

    for (auto &o : scene) if (o.valid) UnloadModel(o.model);
    UnloadRenderTexture(rtex);
    CloseWindow();
    return 0;
}

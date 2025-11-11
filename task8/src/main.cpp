// main.cpp
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

// ------------------------
// Структуры и вспомогательные функции
// ------------------------

struct Obj3D {
    Model model;
    Vector3 pos{0,0,0};
    Vector3 rot{0,0,0};
    float scale{1.0f};
    Color color{200,200,220,255};
    bool valid{false};
};

// Камера-объект с матрицами
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
        view = GetCameraMatrix(c); // матрица вида
        float aspect = (float)screenW / (float)screenH;
        proj = MatrixPerspective(DEG2RAD * fovy, aspect, nearZ, farZ); // матрица проекции
    }
};

static Vector3 GetVertex(const Mesh& m, int idx) {
    int i = idx*3;
    return { m.vertices[i+0], m.vertices[i+1], m.vertices[i+2] };
}

static Vector3 TransformVec(Vector3 v, Matrix M) {
    return Vector3Transform(v, M);
}

static void DrawCulledMesh(const Mesh& mesh, Matrix objM, const Camera3D& cam, Color col, bool showBackface) {
    // рисуем треугольники с простым backface culling
    // (используем rlVertex напрямую для обучения — в production лучше шейдер/вбуферы)
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

        Vector3 center = {(v1.x+v2.x+v3.x)/3.0f, (v1.y+v2.y+v3.y)/3.0f, (v1.z+v2.z+v3.z)/3.0f};
        Vector3 lookVec = Vector3Subtract(center, cam.position);

        // если нормаль направлена от камеры — рисуем
        if (Vector3DotProduct(n, lookVec) < 0.0f) {
            rlColor4ub(col.r, col.g, col.b, col.a);
            rlVertex3f(v1.x, v1.y, v1.z);
            rlVertex3f(v2.x, v2.y, v2.z);
            rlVertex3f(v3.x, v3.y, v3.z);
        } else if (showBackface) {
            // дополнительная визуализация обратных граней (полупрозрачные)
            rlColor4ub(60, 60, 60, 80);
            rlVertex3f(v1.x, v1.y, v1.z);
            rlVertex3f(v2.x, v2.y, v2.z);
            rlVertex3f(v3.x, v3.y, v3.z);
        }
    }
    rlEnd();
}

static void DrawObjCulled(const Obj3D& obj, const Camera3D& cam, bool showBackface) {
    Matrix S = MatrixScale(obj.scale, obj.scale, obj.scale);
    Matrix R = MatrixRotateXYZ({obj.rot.x, obj.rot.y, obj.rot.z});
    Matrix T = MatrixTranslate(obj.pos.x, obj.pos.y, obj.pos.z);
    Matrix M = MatrixMultiply(MatrixMultiply(S, R), T); // мировая матрица объекта
    for (int i = 0; i < obj.model.meshCount; i++) {
        DrawCulledMesh(obj.model.meshes[i], M, cam, obj.color, showBackface);
    }
}

// ------------------------
// Основная программа
// ------------------------

static std::string toLowerExt(const std::string& s) {
    std::string ext;
    auto pos = s.find_last_of('.');
    if (pos == std::string::npos) return "";
    ext = s.substr(pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });
    return ext;
}

int main() {
    InitWindow(1280, 800, "Camera Object + ADD OBJ fixed (preserve old UI)");
    SetTargetFPS(60);

    // raylib-камера (используется BeginMode3D)
    Camera3D cam = {0};
    cam.position = {6.0f, 3.0f, 6.0f};
    cam.target = {0.0f, 0.0f, 0.0f};
    cam.up = {0.0f, 1.0f, 0.0f};
    cam.fovy = 60.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    // наш CameraObj для хранения view/proj
    CameraObj objCam;
    objCam.fovy = cam.fovy;
    objCam.nearZ = 0.1f;
    objCam.farZ = 100.0f;

    // собираем список .obj (регистр расширения игнорируем)
    std::vector<std::string> objFiles;
    std::string figuresDir = "figures";
    if (std::filesystem::exists(figuresDir) && std::filesystem::is_directory(figuresDir)) {
        for (auto &p : std::filesystem::directory_iterator(figuresDir)) {
            if (!p.is_regular_file()) continue;
            std::string ext = toLowerExt(p.path().string());
            if (ext == ".obj") objFiles.push_back(p.path().string());
        }
    }

    std::vector<Obj3D> scene;
    int selected = -1;
    int nextFile = 0;
    bool moveMode = false;
    bool testBackface = false;

    float camOrbitRadius = 6.0f;
    float camAngle = 0.0f;
    float camSpeed = 0.6f;

    RenderTexture2D rtex = LoadRenderTexture(800, 600);

    int panelW = 280;

    // Список цветов для объектов
    Color cols[6] = { RED, ORANGE, YELLOW, GREEN, SKYBLUE, VIOLET };

    // addObj: возвращает true если успешно
    auto addObj = [&](const std::string& path)->bool {
        // проверим файл
        if (!std::filesystem::exists(path)) return false;
        Model m = LoadModel(path.c_str());

        // простая проверка успешности — если meshCount == 0, модель скорее всего не загрузилась
        if (m.meshCount <= 0) {
            // если каким-то образом что-то внутри есть, очищаем
            UnloadModel(m);
            return false;
        }

        Obj3D o;
        o.model = m;
        o.valid = true;
        o.scale = 1.0f;

        // позиционируем объект по X так, чтобы новые объекты не накладывались
        float spacing = 2.0f;
        o.pos = { (float)scene.size() * spacing, 0.0f, 0.0f };

        // цвет — по индексу (используем число объектов до добавления)
        o.color = cols[scene.size() % 6];

        scene.push_back(o);
        selected = (int)scene.size() - 1;
        return true;
    };

    // Restore nextFile to valid range if objFiles size changed
    auto clampNextFile = [&](){
        if (objFiles.empty()) nextFile = 0;
        else nextFile = nextFile % (int)objFiles.size();
    };
    clampNextFile();

    // Основной цикл
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        int sw = GetScreenWidth(), sh = GetScreenHeight();
        Rectangle panel = {(float)(sw - panelW), 0, (float)panelW, (float)sh};

        // клавиши управления
        if (IsKeyPressed(KEY_P)) {
            cam.projection = (cam.projection == CAMERA_PERSPECTIVE) ? CAMERA_ORTHOGRAPHIC : CAMERA_PERSPECTIVE;
            objCam.fovy = cam.fovy;
        }

        // WASD для перемещения выбранного объекта (если включен режим MOVE)
        if (moveMode && selected >= 0 && selected < (int)scene.size()) {
            float sp = 3.0f * dt;
            if (IsKeyDown(KEY_W)) scene[selected].pos.z -= sp;
            if (IsKeyDown(KEY_S)) scene[selected].pos.z += sp;
            if (IsKeyDown(KEY_A)) scene[selected].pos.x -= sp;
            if (IsKeyDown(KEY_D)) scene[selected].pos.x += sp;
            if (IsKeyDown(KEY_Q)) scene[selected].pos.y -= sp;
            if (IsKeyDown(KEY_E)) scene[selected].pos.y += sp;
            if (IsKeyPressed(KEY_LEFT)) {
                if (!scene.empty()) selected = (selected - 1 + (int)scene.size()) % (int)scene.size();
            }
            if (IsKeyPressed(KEY_RIGHT)) {
                if (!scene.empty()) selected = (selected + 1) % (int)scene.size();
            }
        }

        // вращение камеры вокруг выбранного объекта (или вокруг 0,0,0 если ничего не выбрано)
        camAngle += camSpeed * dt;
        Vector3 center = {0.0f, 0.0f, 0.0f};
        if (selected >= 0 && selected < (int)scene.size()) center = scene[selected].pos;

        cam.position = {
            center.x + camOrbitRadius * sinf(camAngle),
            center.y + 2.0f,
            center.z + camOrbitRadius * cosf(camAngle)
        };
        cam.target = center;
        cam.up = {0.0f, 1.0f, 0.0f};

        // Обновляем объект-камеру (view и proj)
        objCam.fovy = cam.fovy;
        objCam.UpdateFromCamera3D(cam, rtex.texture.width, rtex.texture.height);

        // --- Рендер сцены в текстуру ---
        BeginTextureMode(rtex);
            ClearBackground({18,18,24,255});
            BeginMode3D(cam); // здесь raylib применяет матрицы view/proj внутренне
                DrawGrid(20, 1.0f);
                for (int i = 0; i < (int)scene.size(); i++) {
                    DrawObjCulled(scene[i], cam, testBackface);
                }
                if (selected >= 0 && selected < (int)scene.size())
                    DrawSphereWires(scene[selected].pos, 0.15f, 10, 10, GOLD);
            EndMode3D();
        EndTextureMode();

        // --- Отрисовка UI и текстур на экране ---
        BeginDrawing();
            ClearBackground({18,18,24,255});

            // показать рендер-таргет (изображение с камеры)
            DrawTexturePro(rtex.texture,
                Rectangle{0.0f, 0.0f, (float)rtex.texture.width, (float)-rtex.texture.height},
                Rectangle{16.0f, 16.0f, 640.0f, 480.0f},
                Vector2{0,0}, 0.0f, WHITE);

            // показать инфу о камере и проекции
            DrawText(TextFormat("Camera pos: [%.2f, %.2f, %.2f]", objCam.position.x, objCam.position.y, objCam.position.z), 16, 510, 14, RAYWHITE);
            DrawText("Projection matrix (proj):", 16, 532, 14, YELLOW);
            DrawText(TextFormat("[ %.3f %.3f %.3f %.3f ]", objCam.proj.m0, objCam.proj.m1, objCam.proj.m2, objCam.proj.m3), 16, 552, 12, LIGHTGRAY);
            DrawText(TextFormat("[ %.3f %.3f %.3f %.3f ]", objCam.proj.m4, objCam.proj.m5, objCam.proj.m6, objCam.proj.m7), 16, 568, 12, LIGHTGRAY);

            // Старая правая панель GUI (как в оригинале)
            GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
            GuiPanel(panel, nullptr);

            float x = panel.x + 16;
            float y = 16;
            float w = panel.width - 32;
            float h = 44;

            if (GuiButton({x,y,w,h}, "ADD OBJ")) {
                // если нет файлов — ничего не делать
                if (!objFiles.empty()) {
                    clampNextFile();
                    std::string path = objFiles[nextFile];
                    bool ok = addObj(path);
                    if (ok) {
                        // Advance nextFile only if adding succeeded
                        nextFile = (nextFile + 1) % (int)objFiles.size();
                    } else {
                        // модель не загрузилась — удаляем из списка, чтобы не пытаться снова
                        objFiles.erase(objFiles.begin() + nextFile);
                        clampNextFile();
                    }
                }
            }
            y += h + 12;

            GuiToggle({x,y,w,h}, "MOVE WASD", &moveMode); y += h + 12;
            GuiToggle({x,y,w,h}, "TEST BACKFACE", &testBackface); y += h + 12;

            if (GuiButton({x,y,w,h}, "CLEAR")) {
                for (auto &o : scene) if (o.valid) UnloadModel(o.model);
                scene.clear();
                selected = -1;
            }
            y += h + 18;

            GuiLabel({x,y,w,28}, TextFormat("OBJs: %d", (int)scene.size())); y += 30;
            GuiLabel({x,y,w,28}, TextFormat("CHOSEN: %d", selected)); y += 30;

            if (objFiles.empty()) GuiLabel({x, (float)sh-42, w, 28}, "NO OBJ FILES");
            else GuiLabel({x, (float)sh-42, w, 28}, TextFormat("NEXT: %s", std::filesystem::path(objFiles[nextFile]).filename().string().c_str()));

            DrawFPS(10,10);
        EndDrawing();
    }

    // Очистка ресурсов
    for (auto &o : scene) if (o.valid) UnloadModel(o.model);
    UnloadRenderTexture(rtex);
    CloseWindow();
    return 0;
}

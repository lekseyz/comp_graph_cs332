#define RAYGUI_IMPLEMENTATION
#include "raylib.h"
#include "raygui.h"
#include "raymath.h"
#include "rlgl.h"
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <limits>
#include <cmath>

// ================================
// ===  Структуры и функции     ===
// ================================

struct Obj3D {
    Model model;
    Vector3 pos{ 0,0,0 };
    Vector3 rot{ 0,0,0 };
    float scale{ 1.0f };
    Color color{ 200,200,220,255 };
    bool valid{ false };
    std::string name;
};

struct CameraObj {
    Vector3 position{ 0,0,0 };
    Vector3 target{ 0,0,0 };
    Vector3 direction{ 0,0,1 };
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

// Z-буфер
struct ZBuffer {
    int width;
    int height;
    std::vector<float> zbuffer;
    std::vector<Color> colorbuffer;

    ZBuffer(int w, int h) : width(w), height(h) {
        zbuffer.resize(w * h);
        colorbuffer.resize(w * h);
        Clear();
    }

    void Clear() {
        std::fill(zbuffer.begin(), zbuffer.end(), std::numeric_limits<float>::max());
        std::fill(colorbuffer.begin(), colorbuffer.end(), Color{ 18, 18, 24, 255 });
    }

    void SetPixel(int x, int y, float z, Color color) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        int idx = y * width + x;
        if (z < zbuffer[idx]) {
            zbuffer[idx] = z;
            colorbuffer[idx] = color;
        }
    }

    Color GetPixel(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height)
            return { 18, 18, 24, 255 };
        return colorbuffer[y * width + x];
    }
};

static Vector3 GetVertex(const Mesh& m, int idx) {
    int i = idx * 3;
    return { m.vertices[i], m.vertices[i + 1], m.vertices[i + 2] };
}

static Vector3 TransformVec(Vector3 v, Matrix M) {
    return Vector3Transform(v, M);
}

static Vector4 MyVector4Transform(Vector4 v, Matrix mat) {
    Vector4 result;
    result.x = mat.m0 * v.x + mat.m4 * v.y + mat.m8 * v.z + mat.m12 * v.w;
    result.y = mat.m1 * v.x + mat.m5 * v.y + mat.m9 * v.z + mat.m13 * v.w;
    result.z = mat.m2 * v.x + mat.m6 * v.y + mat.m10 * v.z + mat.m14 * v.w;
    result.w = mat.m3 * v.x + mat.m7 * v.y + mat.m11 * v.z + mat.m15 * v.w;
    return result;
}

Vector2 ProjectToScreen(Vector3 worldPos, Matrix viewProj, int screenW, int screenH) {
    Vector4 clipPos = MyVector4Transform({ worldPos.x, worldPos.y, worldPos.z, 1.0f }, viewProj);

    if (clipPos.w != 0.0f) {
        clipPos.x /= clipPos.w;
        clipPos.y /= clipPos.w;
        clipPos.z /= clipPos.w;
    }

    Vector2 screenPos;
    screenPos.x = (clipPos.x + 1.0f) * 0.5f * screenW;
    screenPos.y = (1.0f - clipPos.y) * 0.5f * screenH;

    return screenPos;
}

float GetDepth(Vector3 worldPos, Matrix viewProj) {
    Vector4 clipPos = MyVector4Transform({ worldPos.x, worldPos.y, worldPos.z, 1.0f }, viewProj);
    if (clipPos.w != 0.0f) {
        return clipPos.z / clipPos.w;
    }
    return 1.0f;
}

Color GetFaceColor(int faceIndex, bool isFront, Color baseColor) {
    float hue = fmodf(faceIndex * 137.508f, 360.0f);

    if (isFront) {
        return ColorFromHSV(hue, 0.7f, 0.9f);
    }
    else {
        return ColorFromHSV(hue, 0.3f, 0.4f);
    }
}

void DrawTriangleZBuffer(ZBuffer& zbuf, Vector3 v1, Vector3 v2, Vector3 v3,
    Matrix viewProj, Color col, int screenW, int screenH) {

    Vector2 p1 = ProjectToScreen(v1, viewProj, screenW, screenH);
    Vector2 p2 = ProjectToScreen(v2, viewProj, screenW, screenH);
    Vector2 p3 = ProjectToScreen(v3, viewProj, screenW, screenH);

    float d1 = GetDepth(v1, viewProj);
    float d2 = GetDepth(v2, viewProj);
    float d3 = GetDepth(v3, viewProj);

    if (d1 < -1.0f || d1 > 1.0f || d2 < -1.0f || d2 > 1.0f || d3 < -1.0f || d3 > 1.0f) {
        return;
    }

    int minX = (int)fmaxf(0, fminf(p1.x, fminf(p2.x, p3.x)));
    int maxX = (int)fminf(screenW - 1, fmaxf(p1.x, fmaxf(p2.x, p3.x)));
    int minY = (int)fmaxf(0, fminf(p1.y, fminf(p2.y, p3.y)));
    int maxY = (int)fminf(screenH - 1, fmaxf(p1.y, fmaxf(p2.y, p3.y)));

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            Vector2 p = { (float)x, (float)y };

            float denom = ((p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y));
            if (fabs(denom) < 0.001f) continue;

            float w1 = ((p2.y - p3.y) * (p.x - p3.x) + (p3.x - p2.x) * (p.y - p3.y)) / denom;
            float w2 = ((p3.y - p1.y) * (p.x - p3.x) + (p1.x - p3.x) * (p.y - p3.y)) / denom;
            float w3 = 1.0f - w1 - w2;

            if (w1 >= 0 && w2 >= 0 && w3 >= 0) {
                float depth = w1 * d1 + w2 * d2 + w3 * d3;
                zbuf.SetPixel(x, y, depth, col);
            }
        }
    }
}

void DrawLineZBuffer(ZBuffer& zbuf, Vector3 v1, Vector3 v2, Matrix viewProj,
    Color col, int screenW, int screenH) {

    Vector2 p1 = ProjectToScreen(v1, viewProj, screenW, screenH);
    Vector2 p2 = ProjectToScreen(v2, viewProj, screenW, screenH);

    float d1 = GetDepth(v1, viewProj);
    float d2 = GetDepth(v2, viewProj);

    int x0 = (int)p1.x, y0 = (int)p1.y;
    int x1 = (int)p2.x, y1 = (int)p2.y;

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    while (true) {
        float t = 0.5f;
        if (dx + dy > 0) {
            float dist = sqrtf((x0 - p1.x) * (x0 - p1.x) + (y0 - p1.y) * (y0 - p1.y));
            float totalDist = sqrtf((p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y));
            if (totalDist > 0) t = dist / totalDist;
        }
        float depth = d1 * (1 - t) + d2 * t;

        zbuf.SetPixel(x0, y0, depth, col);

        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void DrawMeshZBuffer(ZBuffer& zbuf, const Mesh& mesh, Matrix objM, Matrix viewProj,
    Color col, int screenW, int screenH, bool useBackfaceCulling,
    const Camera3D& cam, bool showBackfaces, bool coloredFaces, bool wireframeMode) {

    int tcount = mesh.triangleCount;
    bool hasIdx = mesh.indices != nullptr;

    for (int t = 0; t < tcount; t++) {
        int i1, i2, i3;
        if (hasIdx) {
            i1 = mesh.indices[t * 3 + 0];
            i2 = mesh.indices[t * 3 + 1];
            i3 = mesh.indices[t * 3 + 2];
        }
        else {
            i1 = t * 3 + 0;
            i2 = t * 3 + 1;
            i3 = t * 3 + 2;
        }

        Vector3 v1 = TransformVec(GetVertex(mesh, i1), objM);
        Vector3 v2 = TransformVec(GetVertex(mesh, i2), objM);
        Vector3 v3 = TransformVec(GetVertex(mesh, i3), objM);

        Vector3 e1 = Vector3Subtract(v2, v1);
        Vector3 e2 = Vector3Subtract(v3, v1);
        Vector3 n = Vector3Normalize(Vector3CrossProduct(e1, e2));
        Vector3 lookVec = Vector3Subtract(v1, cam.position);
        float dotProduct = Vector3DotProduct(n, lookVec);

        bool isFront = dotProduct < 0.0f;

        Color faceColor = col;
        if (coloredFaces) {
            faceColor = GetFaceColor(t, isFront, col);
        }

        if (useBackfaceCulling && !isFront) {
            continue;
        }

        if (!coloredFaces && showBackfaces && !isFront) {
            faceColor = Color{
                (unsigned char)(col.r * 0.3f),
                (unsigned char)(col.g * 0.3f),
                (unsigned char)(col.b * 0.3f),
                (unsigned char)(col.a * 0.7f)
            };
        }

        if (!wireframeMode) {
            DrawTriangleZBuffer(zbuf, v1, v2, v3, viewProj, faceColor, screenW, screenH);
        }

        if (wireframeMode || coloredFaces) {
            Color edgeColor = wireframeMode ? WHITE : Color{ 0, 0, 0, 255 };
            DrawLineZBuffer(zbuf, v1, v2, viewProj, edgeColor, screenW, screenH);
            DrawLineZBuffer(zbuf, v2, v3, viewProj, edgeColor, screenW, screenH);
            DrawLineZBuffer(zbuf, v3, v1, viewProj, edgeColor, screenW, screenH);
        }
    }
}

void DrawObjZBuffer(ZBuffer& zbuf, const Obj3D& obj, Matrix viewProj,
    const Camera3D& cam, int screenW, int screenH, bool useBackfaceCulling,
    bool showBackfaces, bool coloredFaces, bool wireframeMode) {
    Matrix S = MatrixScale(obj.scale, obj.scale, obj.scale);
    Matrix Rx = MatrixRotateX(obj.rot.x * DEG2RAD);
    Matrix Ry = MatrixRotateY(obj.rot.y * DEG2RAD);
    Matrix Rz = MatrixRotateZ(obj.rot.z * DEG2RAD);
    Matrix R = MatrixMultiply(MatrixMultiply(Rx, Ry), Rz);
    Matrix T = MatrixTranslate(obj.pos.x, obj.pos.y, obj.pos.z);
    Matrix M = MatrixMultiply(MatrixMultiply(S, R), T);

    for (int i = 0; i < obj.model.meshCount; i++) {
        DrawMeshZBuffer(zbuf, obj.model.meshes[i], M, viewProj,
            obj.color, screenW, screenH, useBackfaceCulling, cam,
            showBackfaces, coloredFaces, wireframeMode);
    }
}

static void DrawCulledMesh(const Mesh& mesh, Matrix objM, const Camera3D& cam, Color col, bool showBack) {
    rlDisableBackfaceCulling();
    rlBegin(RL_TRIANGLES);
    int tcount = mesh.triangleCount;
    bool hasIdx = mesh.indices != nullptr;
    for (int t = 0; t < tcount; t++) {
        int i1, i2, i3;
        if (hasIdx) {
            i1 = mesh.indices[t * 3 + 0];
            i2 = mesh.indices[t * 3 + 1];
            i3 = mesh.indices[t * 3 + 2];
        }
        else {
            i1 = t * 3 + 0; i2 = t * 3 + 1; i3 = t * 3 + 2;
        }

        Vector3 v1 = TransformVec(GetVertex(mesh, i1), objM);
        Vector3 v2 = TransformVec(GetVertex(mesh, i2), objM);
        Vector3 v3 = TransformVec(GetVertex(mesh, i3), objM);

        Vector3 e1 = Vector3Subtract(v2, v1);
        Vector3 e2 = Vector3Subtract(v3, v1);
        Vector3 n = Vector3Normalize(Vector3CrossProduct(e1, e2));

        Vector3 lookVec = Vector3Subtract(v1, cam.position);
        if (Vector3DotProduct(n, lookVec) < 0.0f) {
            rlColor4ub(col.r, col.g, col.b, col.a);
            rlVertex3f(v1.x, v1.y, v1.z);
            rlVertex3f(v2.x, v2.y, v2.z);
            rlVertex3f(v3.x, v3.y, v3.z);
        }
        else if (showBack) {
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
    Matrix R = MatrixRotateXYZ({ obj.rot.x * DEG2RAD, obj.rot.y * DEG2RAD, obj.rot.z * DEG2RAD });
    Matrix T = MatrixTranslate(obj.pos.x, obj.pos.y, obj.pos.z);
    Matrix M = MatrixMultiply(MatrixMultiply(S, R), T);
    for (int i = 0; i < obj.model.meshCount; i++)
        DrawCulledMesh(obj.model.meshes[i], M, cam, obj.color, showBack);
}

void DrawZBufferToImage(Image& img, const ZBuffer& zbuf) {
    for (int y = 0; y < zbuf.height; y++) {
        for (int x = 0; x < zbuf.width; x++) {
            Color col = zbuf.GetPixel(x, y);
            ImageDrawPixel(&img, x, y, col);
        }
    }
}

// ================================
// ===           MAIN           ===
// ================================

int main() {
    InitWindow(1800, 1000, "Z-Buffer Algorithm | Backface Culling Demo");
    SetTargetFPS(60);

    const int RENDER_WIDTH = 1000;
    const int RENDER_HEIGHT = 800;

    Camera3D cam = { 0 };
    cam.position = { 6.0f, 3.0f, 6.0f };
    cam.target = { 0.0f, 0.0f, 0.0f };
    cam.up = { 0.0f, 1.0f, 0.0f };
    cam.fovy = 60.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    CameraObj objCam;
    objCam.fovy = cam.fovy;

    // Загружаем .obj файлы из папки figures
    std::vector<std::string> objFiles;
    std::string figuresPath = "figures";

    if (std::filesystem::exists(figuresPath)) {
        for (auto& p : std::filesystem::directory_iterator(figuresPath)) {
            if (p.path().extension() == ".obj") {
                objFiles.push_back(p.path().string());
            }
        }
    }

    if (objFiles.empty()) {
        for (auto& p : std::filesystem::directory_iterator(".")) {
            if (p.path().extension() == ".obj") {
                objFiles.push_back(p.path().string());
            }
        }
    }

    if (objFiles.empty()) {
        TraceLog(LOG_WARNING, "No .obj files found in 'figures' folder or current directory");
        objFiles.push_back("");
    }
    else {
        TraceLog(LOG_INFO, TextFormat("Found %d .obj files", objFiles.size()));
    }

    std::vector<Obj3D> scene;
    int selected = -1;
    int currentObjIndex = 0;

    // Настройки
    bool useZBuffer = true;
    bool autoRotate = false;
    bool showBackfaces = false;
    bool orbitMode = false;
    bool camControlEnabled = true;
    bool usePerspective = true;
    bool useBackfaceCulling = true;
    bool coloredFaces = true;
    bool wireframeMode = false;

    float camMoveSpeed = 5.0f;
    float camOrbitRadius = 6.0f;
    float camAngle = 0.0f;
    float camSpeed = 0.6f;

    ZBuffer zbuf(RENDER_WIDTH, RENDER_HEIGHT);
    Image img = GenImageColor(RENDER_WIDTH, RENDER_HEIGHT, { 18, 18, 24, 255 });
    Texture2D tex = LoadTextureFromImage(img);

    RenderTexture2D rtex = LoadRenderTexture(RENDER_WIDTH, RENDER_HEIGHT);

    int panelW = 550;
    Vector2 scrollOffset = { 0, 0 };

    auto addObj = [&](const std::string& path, const std::string& name = "") {
        Obj3D o;
        if (!path.empty()) {
            o.model = LoadModel(path.c_str());
            o.name = name.empty() ? path : name;
        }
        else {
            o.model = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
            o.name = "Cube";
        }
        o.valid = true;
        o.scale = 1.0f;
        o.color = Color{
            (unsigned char)(rand() % 155 + 100),
            (unsigned char)(rand() % 155 + 100),
            (unsigned char)(rand() % 155 + 100),
            255
        };
        o.pos = { (float)(scene.size() % 3) * 3.0f - 3.0f, 0.0f, (float)(scene.size() / 3) * 3.0f };
        scene.push_back(o);
        selected = (int)scene.size() - 1;
        };

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        int sw = GetScreenWidth(), sh = GetScreenHeight();
        Rectangle panel = { (float)(sw - panelW), 0, (float)panelW, (float)sh };

        // ОБНОВЛЕНИЕ КАМЕРЫ
        if (orbitMode) {
            camAngle += camSpeed * dt;
            Vector3 center = { 0,0,0 };
            if (selected >= 0 && selected < (int)scene.size()) center = scene[selected].pos;

            cam.position = {
                center.x + camOrbitRadius * sinf(camAngle),
                center.y + 2.0f,
                center.z + camOrbitRadius * cosf(camAngle)
            };
            cam.target = center;
            cam.up = { 0,1,0 };

            objCam.UpdateFromCamera3D(cam, RENDER_WIDTH, RENDER_HEIGHT);
        }
        else {
            if (autoRotate) {
                for (auto& o : scene)
                    o.rot.y += 30.0f * dt;
            }

            if (camControlEnabled) {
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
                if (IsKeyDown(KEY_Q)) {
                    cam.position = Vector3Add(cam.position, Vector3Scale(camUp, camMoveSpeed * dt));
                    cam.target = Vector3Add(cam.target, Vector3Scale(camUp, camMoveSpeed * dt));
                }
                if (IsKeyDown(KEY_E)) {
                    cam.position = Vector3Subtract(cam.position, Vector3Scale(camUp, camMoveSpeed * dt));
                    cam.target = Vector3Subtract(cam.target, Vector3Scale(camUp, camMoveSpeed * dt));
                }
            }
        }

        // --- РЕНДЕРИНГ СЦЕНЫ ---
        if (useZBuffer) {
            zbuf.Clear();

            Matrix view = GetCameraMatrix(cam);
            Matrix proj;

            if (usePerspective) {
                float aspect = (float)RENDER_WIDTH / (float)RENDER_HEIGHT;
                proj = MatrixPerspective(cam.fovy * DEG2RAD, aspect, 0.1f, 100.0f);
            }
            else {
                float size = 5.0f;
                float aspect = (float)RENDER_WIDTH / (float)RENDER_HEIGHT;
                proj = MatrixOrtho(-size * aspect, size * aspect, -size, size, 0.1f, 100.0f);
            }

            Matrix viewProj = MatrixMultiply(view, proj);

            for (const auto& obj : scene) {
                if (obj.valid) {
                    DrawObjZBuffer(zbuf, obj, viewProj, cam, RENDER_WIDTH, RENDER_HEIGHT,
                        useBackfaceCulling, showBackfaces, coloredFaces, wireframeMode);
                }
            }

            DrawZBufferToImage(img, zbuf);
            UpdateTexture(tex, img.data);
        }
        else {
            BeginTextureMode(rtex);
            ClearBackground({ 18,18,24,255 });
            BeginMode3D(cam);
            DrawGrid(20, 1.0f);
            for (int i = 0; i < (int)scene.size(); i++)
                DrawObjCulled(scene[i], cam, showBackfaces);
            if (selected >= 0 && selected < (int)scene.size())
                DrawSphereWires(scene[selected].pos, 0.15f, 10, 10, GOLD);
            EndMode3D();
            EndTextureMode();
        }

        // --- ОТРИСОВКА ИНТЕРФЕЙСА ---
        BeginDrawing();
        ClearBackground({ 18,18,24,255 });

        // Рисуем viewport
        float viewportX = 20;
        float viewportY = 20;
        float viewportW = sw - panelW - 40;
        float viewportH = sh - 40;

        if (useZBuffer) {
            DrawTexturePro(tex,
                { 0, 0, (float)RENDER_WIDTH, (float)RENDER_HEIGHT },
                { viewportX, viewportY, viewportW, viewportH },
                { 0, 0 }, 0, WHITE);
        }
        else {
            DrawTexturePro(rtex.texture,
                { 0, 0, (float)rtex.texture.width, (float)-rtex.texture.height },
                { viewportX, viewportY, viewportW, viewportH },
                { 0, 0 }, 0, WHITE);
        }

        DrawRectangleLines(viewportX, viewportY, viewportW, viewportH, RAYWHITE);

        // ---- GUI ПАНЕЛЬ СО СКРОЛЛИНГОМ ----
        GuiPanel(panel, "CONTROL PANEL");

        Rectangle scrollView = { panel.x, panel.y + 30, panel.width, panel.height - 30 };
        Rectangle scrollContent = { 0, 0, scrollView.width - 20, 1400 };

        GuiScrollPanel(scrollView, nullptr, scrollContent, &scrollOffset, nullptr);

        BeginScissorMode(scrollView.x, scrollView.y, scrollView.width, scrollView.height);

        float x = panel.x + 30 + scrollOffset.x;
        float y = panel.y + 50 + scrollOffset.y;
        float w = panel.width - 60;
        float xSlider = panel.x + 145 + scrollOffset.x;  // Сдвинули слайдер правее для места под текст слева
        float wSlider = panel.width - 320;  // Уменьшили ширину слайдера для места под текст справа (230 пикселей)
        float h = 45;
        float spacing = 20;

        // === КНОПКИ ===
        GuiSetStyle(DEFAULT, TEXT_SIZE, 20);

        if (GuiButton({ x, y, w, h }, "ADD NEXT OBJ")) {
            std::filesystem::path p(objFiles[currentObjIndex]);
            std::string name = objFiles[currentObjIndex].empty() ? "Cube" : p.filename().string();
            addObj(objFiles[currentObjIndex], name);
            currentObjIndex = (currentObjIndex + 1) % objFiles.size();
        }
        y += h + spacing;

        if (GuiButton({ x, y, w, h }, "CLEAR ALL")) {
            for (auto& o : scene) if (o.valid) UnloadModel(o.model);
            scene.clear();
            selected = -1;
        }
        y += h + spacing + 10;

        // === НАСТРОЙКИ ===
        GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
        GuiLine({ x, y, w, 2 }, "SETTINGS");
        y += 25;

        GuiToggle({ x, y, w, h }, "Z-BUFFER", &useZBuffer);
        y += h + spacing;

        GuiToggle({ x, y, w, h }, "BACKFACE CULLING", &useBackfaceCulling);
        y += h + spacing;

        GuiToggle({ x, y, w, h }, "SHOW BACKFACES", &showBackfaces);
        y += h + spacing;

        GuiToggle({ x, y, w, h }, "COLORED FACES", &coloredFaces);
        y += h + spacing;

        GuiToggle({ x, y, w, h }, "WIREFRAME", &wireframeMode);
        y += h + spacing;

        GuiToggle({ x, y, w, h }, "AUTO ROTATE", &autoRotate);
        y += h + spacing;

        GuiToggle({ x, y, w, h }, "PERSPECTIVE", &usePerspective);
        y += h + spacing;

        GuiToggle({ x, y, w, h }, "ORBIT CAMERA", &orbitMode);
        y += h + spacing;

        GuiToggle({ x, y, w, h }, "CAMERA CONTROL", &camControlEnabled);
        y += h + spacing + 10;

        // === ИНФОРМАЦИЯ ===
        GuiLine({ x, y, w, 2 }, "INFO");
        y += 25;

        GuiLabel({ x, y, w, 25 }, TextFormat("Objects: %d", (int)scene.size()));
        y += 30;

        GuiLabel({ x, y, w, 25 }, TextFormat("Selected: %d", selected >= 0 ? selected : -1));
        y += 30;

        if (selected >= 0 && selected < (int)scene.size()) {
            GuiLabel({ x, y, w, 25 }, TextFormat("Name: %s", scene[selected].name.c_str()));
            y += 30;
        }

        GuiLabel({ x, y, w, 25 }, TextFormat("Available OBJs: %d", objFiles.size()));
        y += 35;

        // === СЛАЙДЕРЫ КАМЕРЫ ===
        GuiSlider({ xSlider, y, wSlider, h }, "CAM SPEED", TextFormat("%.1f", camMoveSpeed),
            &camMoveSpeed, 1.0f, 20.0f);
        y += h + spacing;

        if (orbitMode) {
            GuiSlider({ xSlider, y, wSlider, h }, "ORBIT RADIUS", TextFormat("%.1f", camOrbitRadius),
                &camOrbitRadius, 2.0f, 20.0f);
            y += h + spacing;

            GuiSlider({ xSlider, y, wSlider, h }, "ORBIT SPEED", TextFormat("%.1f", camSpeed),
                &camSpeed, 0.1f, 3.0f);
            y += h + spacing;
        }

        y += 10;

        // === УПРАВЛЕНИЕ ОБЪЕКТОМ ===
        if (selected >= 0 && selected < (int)scene.size()) {
            GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
            GuiLine({ x, y, w, 2 }, "OBJECT CONTROL");
            y += 25;

            Obj3D& obj = scene[selected];

            GuiSlider({ xSlider, y, wSlider, h }, "POS X", TextFormat("%.2f", obj.pos.x),
                &obj.pos.x, -10.0f, 10.0f);
            y += h + spacing;

            GuiSlider({ xSlider, y, wSlider, h }, "POS Y", TextFormat("%.2f", obj.pos.y),
                &obj.pos.y, -10.0f, 10.0f);
            y += h + spacing;

            GuiSlider({ xSlider, y, wSlider, h }, "POS Z", TextFormat("%.2f", obj.pos.z),
                &obj.pos.z, -10.0f, 10.0f);
            y += h + spacing;

            GuiSlider({ xSlider, y, wSlider, h }, "SCALE", TextFormat("%.2f", obj.scale),
                &obj.scale, 0.1f, 5.0f);
            y += h + spacing;

            GuiSlider({ xSlider, y, wSlider, h }, "ROT Y", TextFormat("%.0f", obj.rot.y),
                &obj.rot.y, 0.0f, 360.0f);
            y += h + spacing;
        }

        EndScissorMode();

        DrawFPS(10, 10);
        DrawText("WASD/QE - Camera Control", 10, sh - 30, 16, LIGHTGRAY);

        EndDrawing();
    }

    for (auto& o : scene) if (o.valid) UnloadModel(o.model);
    UnloadRenderTexture(rtex);
    UnloadTexture(tex);
    UnloadImage(img);
    CloseWindow();
    return 0;
}
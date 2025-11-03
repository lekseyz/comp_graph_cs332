// женя для тебя:
//   g++ -std=c++17 poly.cpp -o poly -O2 -Wall -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
// егор для тебя:
//   g++ -std=c++17 src/main.cpp -o main.exe -Iinclude -lraylib -lopengl32 -lgdi32 -lwinmm

#include "raylib.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_ICONS
#include "raygui.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

struct Point3D {
    float x{ 0 }, y{ 0 }, z{ 0 };
    Point3D() = default;
    Point3D(float X, float Y, float Z) : x(X), y(Y), z(Z) {}
    explicit Point3D(const Vector3& v) : x(v.x), y(v.y), z(v.z) {}
    Vector3 toVec3() const { return { x, y, z }; }
};

struct Face {
    std::vector<int> idx;
};

enum class ReflectPlane { XY = 0, YZ = 1, XZ = 2 };
enum ProjectionType { PROJ_PERSPECTIVE = 0, PROJ_ORTHOGRAPHIC = 1 };

struct Polyhedron {
    std::vector<Point3D> vertices;
    std::vector<Face>    faces;
    std::vector<std::pair<int, int>> edges;
    Matrix model{ MatrixIdentity() };

    void clear() { vertices.clear(); faces.clear(); edges.clear(); model = MatrixIdentity(); }

    bool loadOBJ(const std::string& path) {
        clear();
        std::ifstream in(path);
        if (!in.is_open()) return false;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::istringstream ss(line);
            std::string tag; ss >> tag;
            if (tag == "v") {
                float x, y, z; ss >> x >> y >> z; vertices.emplace_back(x, y, z);
            }
            else if (tag == "f") {
                Face f; std::string tok;
                while (ss >> tok) {
                    std::string num;
                    for (char c : tok) { if (c == '/') break; num.push_back(c); }
                    if (num.empty() || num == "-") continue;
                    int idx = std::stoi(num), vi = 0;
                    if (idx > 0) vi = idx - 1;
                    else if (idx < 0) vi = (int)vertices.size() + idx;
                    f.idx.push_back(vi);
                }
                if (f.idx.size() >= 2) faces.push_back(std::move(f));
            }
        }
        in.close();
        if (vertices.empty()) return false;
        computeEdges();
        recenterAndAutoscale();
        return true;
    }

    void computeEdges() {
        edges.clear();
        std::set<std::pair<int, int>> uniq;
        // 1) edges from faces
        for (const auto& f : faces) {
            if (f.idx.size() < 2) continue;
            for (size_t i = 0; i < f.idx.size(); ++i) {
                int a = f.idx[i];
                int b = f.idx[(i + 1) % f.idx.size()];
                if (a == b) continue;
                if (a > b) std::swap(a, b);
                if (uniq.insert({ a,b }).second) edges.emplace_back(a, b);
            }
        }
        if (edges.empty() && vertices.size() >= 2) {
            float dmin2 = FLT_MAX;
            for (size_t i = 0; i < vertices.size(); ++i)
                for (size_t j = i + 1; j < vertices.size(); ++j) {
                    float dx = vertices[i].x - vertices[j].x;
                    float dy = vertices[i].y - vertices[j].y;
                    float dz = vertices[i].z - vertices[j].z;
                    float d2 = dx * dx + dy * dy + dz * dz;
                    if (d2 > 1e-8f && d2 < dmin2) dmin2 = d2;
                }
            if (dmin2 < FLT_MAX / 2) {
                const float rel = 1e-3f;
                for (size_t i = 0; i < vertices.size(); ++i)
                    for (size_t j = i + 1; j < vertices.size(); ++j) {
                        float dx = vertices[i].x - vertices[j].x;
                        float dy = vertices[i].y - vertices[j].y;
                        float dz = vertices[i].z - vertices[j].z;
                        float d2 = dx * dx + dy * dy + dz * dz;
                        if (fabsf(d2 - dmin2) <= dmin2 * rel + 1e-9f) edges.emplace_back((int)i, (int)j);
                    }
            }
        }
    }

    Vector3 centroidObj() const {
        Vector3 c{ 0,0,0 };
        for (auto& p : vertices) { c.x += p.x; c.y += p.y; c.z += p.z; }
        if (!vertices.empty()) { c.x /= vertices.size(); c.y /= vertices.size(); c.z /= vertices.size(); }
        return c;
    }
    Vector3 centroidWorld() const { return Vector3Transform(centroidObj(), model); }

    void recenterAndAutoscale() {
        if (vertices.empty()) return;
        Vector3 c = centroidObj();
        float maxd = 0.0f;
        for (auto& p : vertices) {
            float dx = p.x - c.x, dy = p.y - c.y, dz = p.z - c.z;
            float d = sqrtf(dx * dx + dy * dy + dz * dz);
            if (d > maxd) maxd = d;
        }
        if (maxd < 1e-6f) maxd = 1.0f;
        Matrix T = MatrixTranslate(-c.x, -c.y, -c.z);
        float s = 1.0f / maxd;
        Matrix S = MatrixScale(s, s, s);
        model = MatrixIdentity();
        model = MatrixMultiply(model, T);
        model = MatrixMultiply(model, S);
    }

    void applyTranslation(float dx, float dy, float dz) {
        Matrix T = MatrixTranslate(dx, dy, dz);
        model = MatrixMultiply(model, T);
    }

    void applyRotationXYZ(float rx, float ry, float rz) {
        if (fabsf(ry) > 0) { Matrix R = MatrixRotateY(ry); model = MatrixMultiply(model, R); }
        if (fabsf(rx) > 0) { Matrix R = MatrixRotateX(rx); model = MatrixMultiply(model, R); }
        if (fabsf(rz) > 0) { Matrix R = MatrixRotateZ(rz); model = MatrixMultiply(model, R); }
    }

    void applyUniformScaleAboutCenter(float scaleFactor) {
        if (scaleFactor <= 0) return;
        Vector3 c = centroidWorld();
        Matrix T1 = MatrixTranslate(-c.x, -c.y, -c.z);
        Matrix S = MatrixScale(scaleFactor, scaleFactor, scaleFactor);
        Matrix T2 = MatrixTranslate(c.x, c.y, c.z);
        Matrix C = MatrixMultiply(T1, S);
        C = MatrixMultiply(C, T2);
        model = MatrixMultiply(model, C);
    }

    void applyReflection(ReflectPlane plane) {
        Matrix R = MatrixIdentity();
        switch (plane) {
        case ReflectPlane::XY: R = MatrixScale(1, 1, -1); break;
        case ReflectPlane::YZ: R = MatrixScale(-1, 1, 1); break;
        case ReflectPlane::XZ: R = MatrixScale(1, -1, 1); break;
        }
        model = MatrixMultiply(model, R);
    }

    void applyRotationAroundLine(Point3D p1, Point3D p2, float angleRad) {
        Vector3 dir = { p2.x - p1.x, p2.y - p1.y, p2.z - p1.z };
        float len = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
        if (len < 1e-6f) return;
        Vector3 axis = { dir.x / len, dir.y / len, dir.z / len };
        Matrix T1 = MatrixTranslate(-p1.x, -p1.y, -p1.z);
        Matrix R = MatrixRotate(axis, angleRad);
        Matrix T2 = MatrixTranslate(p1.x, p1.y, p1.z);
        Matrix combined = MatrixMultiply(T1, R);
        combined = MatrixMultiply(combined, T2);
        model = MatrixMultiply(model, combined);
    }
};

static std::vector<std::string> listObjFiles(const std::string& dir) {
    std::vector<std::string> out;
    if (!fs::exists(dir)) return out;
    for (auto& p : fs::directory_iterator(dir)) {
        if (!p.is_regular_file()) continue;
        if (p.path().extension() == ".obj") out.push_back(p.path().filename().string());
    }
    std::sort(out.begin(), out.end());
    return out;
}

static float UniformScaleFromMatrix(const Matrix& m) {
    Vector3 o = Vector3Transform({ 0,0,0 }, m);
    Vector3 ex = Vector3Transform({ 1,0,0 }, m);
    return Vector3Distance(o, ex);
}

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(900, 600, "Polyhedron Transform Tool - Projections");
    SetTargetFPS(60);

    const Rectangle viewport = { 0, 0, 600, 600 };
    RenderTexture2D target = LoadRenderTexture((int)viewport.width, (int)viewport.height);

    Camera3D cam = { 0 };
    cam.position = { 3.0f, 3.0f, 3.0f };
    cam.target = { 0.0f, 0.0f, 0.0f };
    cam.up = { 0.0f, 1.0f, 0.0f };
    cam.fovy = 45.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    // Ортографическая камера с увеличенным размером для уменьшения объектов
    Camera3D orthoCam = { 0 };
    orthoCam.position = { 3.0f, 3.0f, 3.0f };
    orthoCam.target = { 0.0f, 0.0f, 0.0f };
    orthoCam.up = { 0.0f, 1.0f, 0.0f };
    orthoCam.fovy = 4.0f; // Увеличенный размер для меньших объектов
    orthoCam.projection = CAMERA_ORTHOGRAPHIC;

    int projectionType = PROJ_PERSPECTIVE;
    float orthoSizeMultiplier = 5.0f; // Множитель для настройки размера

    Polyhedron poly;
    std::vector<std::string> files = listObjFiles("figures");
    int fileIndex = 0;

    auto loadCurrentFile = [&]() {
        if (files.empty()) return false;
        std::string path = std::string("figures/") + files[fileIndex];
        bool ok = poly.loadOBJ(path);
        if (!ok) TraceLog(LOG_WARNING, "Failed to load OBJ: %s", path.c_str());
        return ok;
        };

    if (!files.empty()) {
        loadCurrentFile();
    }
    else {
        poly.clear();
        poly.vertices = {
            {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
            {-1,-1, 1},{1,-1, 1},{1,1, 1},{-1,1, 1}
        };
        poly.faces = {
            {{0,1,2,3}}, {{4,5,6,7}}, {{0,1,5,4}}, {{2,3,7,6}}, {{1,2,6,5}}, {{0,3,7,4}}
        };
        poly.computeEdges();
        poly.recenterAndAutoscale();
    }

    enum Tool { MOVE = 0, ROTATE = 1, SCALE = 2, REFLECT = 3, ROTATE_LINE = 4 };
    int tool = MOVE;
    int reflectPlane = 0;

    char p1x_text[32] = "0.0";
    char p1y_text[32] = "0.0";
    char p1z_text[32] = "0.0";
    char p2x_text[32] = "1.0";
    char p2y_text[32] = "0.0";
    char p2z_text[32] = "0.0";
    char angle_text[32] = "45.0";

    enum TextFieldID { P1X = 0, P1Y, P1Z, P2X, P2Y, P2Z, ANGLE, NONE };
    int activeField = NONE;

    bool dragging = false, reflectAppliedThisDrag = false;

    const float transSens = 0.01f;   // world units per pixel
    const float rotSens = 0.01f;   // radians per pixel
    const float scaleSens = 0.01f;   // scale delta per pixel (uniform)

    const int PANEL_X = 600, PANEL_W = 300;
    const int LINE_H = 26, GAP = 6;

    Vector2 scrollOffset = { 0, 0 };
    Rectangle scrollView = { 0 };

    while (!WindowShouldClose()) {
        // Обновление ортографической камеры с увеличенным множителем
        if (projectionType == PROJ_ORTHOGRAPHIC) {
            float scale = UniformScaleFromMatrix(poly.model);
            orthoCam.fovy = scale * orthoSizeMultiplier; // Используем настраиваемый множитель
        }

        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        Rectangle uiPanel = { (float)PANEL_X, 0, (float)PANEL_W, 600 };
        DrawRectangleRec(uiPanel, Color{ 245,245,245,255 });
        DrawLine(PANEL_X, 0, PANEL_X, 600, LIGHTGRAY);

        Rectangle panelView = { (float)PANEL_X, 0, (float)PANEL_W, 600 };
        Rectangle panelContent = { (float)PANEL_X, 0, (float)(PANEL_W - 12), 1200 };

        Rectangle scissor = { 0 };
        GuiScrollPanel(panelView, NULL, panelContent, &scrollOffset, &scissor);

        BeginScissorMode((int)scissor.x, (int)scissor.y, (int)scissor.width, (int)scissor.height);

        int y = 10 + (int)scrollOffset.y;
        const int UI_X = PANEL_X + 10;
        const int UI_W = PANEL_W - 30;

        DrawText("Figure:", UI_X, y, 16, DARKGRAY); y += 20;

        Rectangle rPrev = { (float)UI_X, (float)y, 80, (float)LINE_H };
        Rectangle rNext = { (float)(UI_X + UI_W - 80), (float)y, 80, (float)LINE_H };
        if (GuiButton(rPrev, "< Prev") && !files.empty()) {
            fileIndex = (fileIndex + (int)files.size() - 1) % (int)files.size();
            loadCurrentFile();
        }
        if (GuiButton(rNext, "Next >") && !files.empty()) {
            fileIndex = (fileIndex + 1) % (int)files.size();
            loadCurrentFile();
        }
        y += LINE_H + GAP;

        std::string label = files.empty() ? std::string("(no .obj)") : files[fileIndex];
        DrawText(label.c_str(), UI_X, y, 14, DARKGRAY);
        y += 18;

        Rectangle rRefresh = { (float)UI_X, (float)y, (float)UI_W, (float)LINE_H };
        if (GuiButton(rRefresh, "Refresh list")) {
            files = listObjFiles("figures");
            if (!files.empty()) { fileIndex = fileIndex % (int)files.size(); loadCurrentFile(); }
        }
        y += LINE_H + GAP + 4;

        // Добавляем переключатель проекций
        DrawText("Projection:", UI_X, y, 16, DARKGRAY); y += 20;
        Rectangle rPersp = { (float)UI_X, (float)y, (float)UI_W/2-5, (float)LINE_H };
        Rectangle rOrtho = { (float)(UI_X + UI_W/2 + 5), (float)y, (float)UI_W/2-5, (float)LINE_H };
        if (GuiButton(rPersp, projectionType == PROJ_PERSPECTIVE ? "[Perspective]" : "Perspective")) 
            projectionType = PROJ_PERSPECTIVE;
        if (GuiButton(rOrtho, projectionType == PROJ_ORTHOGRAPHIC ? "[Orthographic]" : "Orthographic")) 
            projectionType = PROJ_ORTHOGRAPHIC;
        y += LINE_H + GAP + 4;

        // Настройки размера для ортографической проекции
        if (projectionType == PROJ_ORTHOGRAPHIC) {
            DrawText("Ortho Size:", UI_X, y, 14, DARKGRAY); y += 16;
            GuiSliderBar({ (float)UI_X, (float)y, (float)UI_W, (float)LINE_H }, "Small", "Large", &orthoSizeMultiplier, 2.0f, 10.0f);
            y += LINE_H + GAP;
        }

        DrawText("Tool:", UI_X, y, 16, DARKGRAY); y += 20;

        Rectangle rMove = { (float)UI_X, (float)y, (float)UI_W, (float)LINE_H };
        Rectangle rRotate = { (float)UI_X, (float)(y + LINE_H + GAP), (float)UI_W, (float)LINE_H };
        Rectangle rScale = { (float)UI_X, (float)(y + 2 * (LINE_H + GAP)), (float)UI_W, (float)LINE_H };
        Rectangle rRefl = { (float)UI_X, (float)(y + 3 * (LINE_H + GAP)), (float)UI_W, (float)LINE_H };
        Rectangle rRotLine = { (float)UI_X, (float)(y + 4 * (LINE_H + GAP)), (float)UI_W, (float)LINE_H };

        if (GuiButton(rMove, tool == MOVE ? "[Move]" : "Move")) tool = MOVE;
        if (GuiButton(rRotate, tool == ROTATE ? "[Rotate]" : "Rotate")) tool = ROTATE;
        if (GuiButton(rScale, tool == SCALE ? "[Scale]" : "Scale")) tool = SCALE;
        if (GuiButton(rRefl, tool == REFLECT ? "[Reflect]" : "Reflect")) tool = REFLECT;
        if (GuiButton(rRotLine, tool == ROTATE_LINE ? "[Rot.Line]" : "Rot.Line")) tool = ROTATE_LINE;

        y += 5 * (LINE_H + GAP) + 4;

        if (tool == REFLECT) {
            DrawText("Reflect plane:", UI_X, y, 16, DARKGRAY); y += 20;
            Rectangle rXY = { (float)UI_X, (float)y, (float)UI_W, (float)LINE_H };
            Rectangle rYZ = { (float)UI_X, (float)(y + LINE_H + GAP), (float)UI_W, (float)LINE_H };
            Rectangle rXZ = { (float)UI_X, (float)(y + 2 * (LINE_H + GAP)), (float)UI_W, (float)LINE_H };
            if (GuiButton(rXY, reflectPlane == 0 ? "[XY]" : "XY")) reflectPlane = 0;
            if (GuiButton(rYZ, reflectPlane == 1 ? "[YZ]" : "YZ")) reflectPlane = 1;
            if (GuiButton(rXZ, reflectPlane == 2 ? "[XZ]" : "XZ")) reflectPlane = 2;
            y += 3 * (LINE_H + GAP) + 4;
            DrawText("Drag to apply", UI_X, y, 12, DARKGRAY);
            y += 20;
        }

        if (tool == ROTATE_LINE) {
            DrawText("Rotation around line:", UI_X, y, 16, DARKGRAY); y += 20;

            const int inputW = 75;
            const int spacing = 8;

            DrawText("P1:", UI_X, y, 14, DARKGRAY); y += 16;

            DrawText("X:", UI_X, y + 5, 11, DARKGRAY);
            Rectangle p1x_rect = { (float)(UI_X + 15), (float)y, (float)inputW, (float)LINE_H };
            if (GuiTextBox(p1x_rect, p1x_text, 32, activeField == P1X)) activeField = P1X;

            DrawText("Y:", UI_X + inputW + spacing + 15, y + 5, 11, DARKGRAY);
            Rectangle p1y_rect = { (float)(UI_X + inputW + spacing + 30), (float)y, (float)inputW, (float)LINE_H };
            if (GuiTextBox(p1y_rect, p1y_text, 32, activeField == P1Y)) activeField = P1Y;
            y += LINE_H + GAP;

            DrawText("Z:", UI_X, y + 5, 11, DARKGRAY);
            Rectangle p1z_rect = { (float)(UI_X + 15), (float)y, (float)inputW, (float)LINE_H };
            if (GuiTextBox(p1z_rect, p1z_text, 32, activeField == P1Z)) activeField = P1Z;
            y += LINE_H + GAP + 4;

            DrawText("P2:", UI_X, y, 14, DARKGRAY); y += 16;

            DrawText("X:", UI_X, y + 5, 11, DARKGRAY);
            Rectangle p2x_rect = { (float)(UI_X + 15), (float)y, (float)inputW, (float)LINE_H };
            if (GuiTextBox(p2x_rect, p2x_text, 32, activeField == P2X)) activeField = P2X;

            DrawText("Y:", UI_X + inputW + spacing + 15, y + 5, 11, DARKGRAY);
            Rectangle p2y_rect = { (float)(UI_X + inputW + spacing + 30), (float)y, (float)inputW, (float)LINE_H };
            if (GuiTextBox(p2y_rect, p2y_text, 32, activeField == P2Y)) activeField = P2Y;
            y += LINE_H + GAP;

            DrawText("Z:", UI_X, y + 5, 11, DARKGRAY);
            Rectangle p2z_rect = { (float)(UI_X + 15), (float)y, (float)inputW, (float)LINE_H };
            if (GuiTextBox(p2z_rect, p2z_text, 32, activeField == P2Z)) activeField = P2Z;
            y += LINE_H + GAP + 4;

            DrawText("Angle (deg):", UI_X, y, 14, DARKGRAY); y += 16;
            Rectangle angle_rect = { (float)UI_X, (float)y, (float)UI_W, (float)LINE_H };
            if (GuiTextBox(angle_rect, angle_text, 32, activeField == ANGLE)) activeField = ANGLE;
            y += LINE_H + GAP + 4;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mouse = GetMousePosition();
                bool clickedOnField = CheckCollisionPointRec(mouse, p1x_rect) ||
                    CheckCollisionPointRec(mouse, p1y_rect) ||
                    CheckCollisionPointRec(mouse, p1z_rect) ||
                    CheckCollisionPointRec(mouse, p2x_rect) ||
                    CheckCollisionPointRec(mouse, p2y_rect) ||
                    CheckCollisionPointRec(mouse, p2z_rect) ||
                    CheckCollisionPointRec(mouse, angle_rect);
                if (!clickedOnField && !CheckCollisionPointRec(mouse, panelView)) activeField = NONE;
            }

            Rectangle rApply = { (float)UI_X, (float)y, (float)UI_W, (float)(LINE_H + 4) };
            if (GuiButton(rApply, "APPLY ROTATION")) {
                activeField = NONE;
                try {
                    Point3D p1(std::stof(p1x_text), std::stof(p1y_text), std::stof(p1z_text));
                    Point3D p2(std::stof(p2x_text), std::stof(p2y_text), std::stof(p2z_text));
                    float angleDeg = std::stof(angle_text);
                    float angleRad = angleDeg * DEG2RAD;
                    poly.applyRotationAroundLine(p1, p2, angleRad);
                    TraceLog(LOG_INFO, "Rotation applied: P1(%.2f,%.2f,%.2f) P2(%.2f,%.2f,%.2f) Angle=%.2f",
                        p1.x, p1.y, p1.z, p2.x, p2.y, p2.z, angleDeg);
                }
                catch (...) {
                    TraceLog(LOG_WARNING, "Invalid input");
                }
            }
            y += LINE_H + GAP + 6;

            DrawText("Click field, type value,\nclick outside, then APPLY.",
                UI_X, y, 11, DARKGRAY);
            y += 30;
        }

        Rectangle rReset = { (float)UI_X, (float)y, (float)UI_W, (float)LINE_H };
        if (GuiButton(rReset, "Reset transforms")) {
            poly.recenterAndAutoscale();
        }
        y += LINE_H + GAP + 4;

        DrawText("Interaction:", UI_X, y, 16, DARKGRAY); y += 18;
        DrawText("Drag LMB in 3D view\nMove: L/R->X, U/D->Y\nRotate: U/D->X, L/R->Y\nScale: U/D",
            UI_X, y, 12, DARKGRAY);

        EndScissorMode();

        Vector2 m = GetMousePosition();
        bool inViewport = CheckCollisionPointRec(m, viewport);

        if (inViewport && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && tool != ROTATE_LINE) {
            dragging = true; reflectAppliedThisDrag = false;
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
            dragging = false; reflectAppliedThisDrag = false;
        }

        if (dragging) {
            Vector2 d = GetMouseDelta();
            float dx = d.x, dy = -d.y;
            if (tool == MOVE) { float dz = IsMouseButtonDown(MOUSE_RIGHT_BUTTON) ? dy : 0.0f; poly.applyTranslation(dx * transSens, dy * transSens, dz * transSens); }
            else if (tool == ROTATE) { float rz = IsMouseButtonDown(MOUSE_RIGHT_BUTTON) ? dx : 0.0f; poly.applyRotationXYZ(dy * rotSens, dx * rotSens, rz * rotSens); }
            else if (tool == SCALE) { float s = 1.0f + (dy * scaleSens); if (s < 0.02f) s = 0.02f; poly.applyUniformScaleAboutCenter(s); }
            else if (tool == REFLECT) { if (!reflectAppliedThisDrag && (fabsf(dx) + fabsf(dy) > 2.0f)) { poly.applyReflection(static_cast<ReflectPlane>(reflectPlane)); reflectAppliedThisDrag = true; } }
        }

        BeginTextureMode(target);
        ClearBackground(RAYWHITE);
        
        // Выбираем камеру в зависимости от типа проекции
        Camera3D currentCamera = (projectionType == PROJ_ORTHOGRAPHIC) ? orthoCam : cam;
        
        BeginMode3D(currentCamera);
        DrawLine3D({ 0,0,0 }, { 1,0,0 }, RED);
        DrawLine3D({ 0,0,0 }, { 0,1,0 }, GREEN);
        DrawLine3D({ 0,0,0 }, { 0,0,1 }, BLUE);
        for (auto& e : poly.edges) {
            Vector3 a = Vector3Transform(poly.vertices[e.first].toVec3(), poly.model);
            Vector3 b = Vector3Transform(poly.vertices[e.second].toVec3(), poly.model);
            DrawLine3D(a, b, BLACK);
        }
        float vr = 0.025f * UniformScaleFromMatrix(poly.model);
        for (auto& p : poly.vertices) {
            Vector3 v = Vector3Transform(p.toVec3(), poly.model);
            DrawSphere(v, vr, DARKGRAY);
        }
        if (tool == ROTATE_LINE) {
            try {
                Point3D p1(std::stof(p1x_text), std::stof(p1y_text), std::stof(p1z_text));
                Point3D p2(std::stof(p2x_text), std::stof(p2y_text), std::stof(p2z_text));
                DrawLine3D(p1.toVec3(), p2.toVec3(), RED);
                DrawSphere(p1.toVec3(), 0.05f, RED);
                DrawSphere(p2.toVec3(), 0.05f, BLUE);
            }
            catch (...) {}
        }
        EndMode3D();
        EndTextureMode();

        Rectangle src = { 0, 0, (float)target.texture.width, -(float)target.texture.height };
        Rectangle dst = viewport;
        DrawTexturePro(target.texture, src, dst, { 0,0 }, 0.0f, WHITE);
        DrawRectangleLinesEx(viewport, 2.0f, DARKGRAY);

        EndDrawing();
    }

    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}

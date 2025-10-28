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
    float x{0}, y{0}, z{0};
    Point3D() = default;
    Point3D(float X, float Y, float Z) : x(X), y(Y), z(Z) {}
    explicit Point3D(const Vector3 &v) : x(v.x), y(v.y), z(v.z) {}
    Vector3 toVec3() const { return {x, y, z}; }
};

struct Face {         
    std::vector<int> idx;    
};

enum class ReflectPlane { XY = 0, YZ = 1, XZ = 2 };

struct Polyhedron {
    std::vector<Point3D> vertices;
    std::vector<Face>    faces;
    std::vector<std::pair<int,int>> edges;
    Matrix model{ MatrixIdentity() };

    void clear() { vertices.clear(); faces.clear(); edges.clear(); model = MatrixIdentity(); }


    bool loadOBJ(const std::string &path) {
        clear();
        std::ifstream in(path);
        if (!in.is_open()) return false;
        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            std::istringstream ss(line);
            std::string tag; ss >> tag;
            if (tag == "v") {
                float x,y,z; ss >> x >> y >> z; vertices.emplace_back(x,y,z);
            } else if (tag == "f") {
                Face f; std::string tok;
                while (ss >> tok) {
                    std::string num;
                    for (char c : tok) { if (c=='/') break; num.push_back(c); }
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
        std::set<std::pair<int,int>> uniq;
        // 1) edges from faces
        for (const auto &f : faces) {
            if (f.idx.size() < 2) continue;
            for (size_t i = 0; i < f.idx.size(); ++i) {
                int a = f.idx[i];
                int b = f.idx[(i+1)%f.idx.size()];
                if (a == b) continue;
                if (a > b) std::swap(a,b);
                if (uniq.insert({a,b}).second) edges.emplace_back(a,b);
            }
        }

        if (edges.empty() && vertices.size() >= 2) {
            float dmin2 = FLT_MAX;
            for (size_t i=0;i<vertices.size();++i)
                for (size_t j=i+1;j<vertices.size();++j) {
                    float dx = vertices[i].x - vertices[j].x;
                    float dy = vertices[i].y - vertices[j].y;
                    float dz = vertices[i].z - vertices[j].z;
                    float d2 = dx*dx + dy*dy + dz*dz;
                    if (d2 > 1e-8f && d2 < dmin2) dmin2 = d2;
                }
            if (dmin2 < FLT_MAX/2) {
                const float rel = 1e-3f;
                for (size_t i=0;i<vertices.size();++i)
                    for (size_t j=i+1;j<vertices.size();++j) {
                        float dx = vertices[i].x - vertices[j].x;
                        float dy = vertices[i].y - vertices[j].y;
                        float dz = vertices[i].z - vertices[j].z;
                        float d2 = dx*dx + dy*dy + dz*dz;
                        if (fabsf(d2 - dmin2) <= dmin2*rel + 1e-9f) edges.emplace_back((int)i,(int)j);
                    }
            }
        }
    }

    Vector3 centroidObj() const {
        Vector3 c{0,0,0};
        for (auto &p : vertices) { c.x += p.x; c.y += p.y; c.z += p.z; }
        if (!vertices.empty()) { c.x/=vertices.size(); c.y/=vertices.size(); c.z/=vertices.size(); }
        return c;
    }
    Vector3 centroidWorld() const { return Vector3Transform(centroidObj(), model); }

    void recenterAndAutoscale() {
        if (vertices.empty()) return;
        Vector3 c = centroidObj();
        float maxd = 0.0f;
        for (auto &p : vertices) {
            float dx = p.x - c.x, dy = p.y - c.y, dz = p.z - c.z;
            float d = sqrtf(dx*dx + dy*dy + dz*dz);
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
        Matrix S  = MatrixScale(scaleFactor, scaleFactor, scaleFactor);
        Matrix T2 = MatrixTranslate(c.x, c.y, c.z);
        Matrix C  = MatrixMultiply(T1, S);
        C = MatrixMultiply(C, T2);
        model = MatrixMultiply(model, C);
    }
    void applyReflection(ReflectPlane plane) {
        Matrix R = MatrixIdentity();
        switch (plane) {
            case ReflectPlane::XY: R = MatrixScale(1,1,-1); break;
            case ReflectPlane::YZ: R = MatrixScale(-1,1,1); break;
            case ReflectPlane::XZ: R = MatrixScale(1,-1,1); break;
        }
        model = MatrixMultiply(model, R);
    }
};


static std::vector<std::string> listObjFiles(const std::string &dir) {
    std::vector<std::string> out;
    if (!fs::exists(dir)) return out;
    for (auto &p : fs::directory_iterator(dir)) {
        if (!p.is_regular_file()) continue;
        if (p.path().extension() == ".obj") out.push_back(p.path().filename().string());
    }
    std::sort(out.begin(), out.end());
    return out;
}

static float UniformScaleFromMatrix(const Matrix &m) {
    Vector3 o  = Vector3Transform({0,0,0}, m);
    Vector3 ex = Vector3Transform({1,0,0}, m);
    return Vector3Distance(o, ex);
}


int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(800, 600, "Polyhedron (OBJ) Transform Tool - raylib + raygui");
    SetTargetFPS(60);

    const Rectangle viewport = { 0, 0, 600, 600 };
    RenderTexture2D target = LoadRenderTexture((int)viewport.width, (int)viewport.height);

    Camera3D cam = {0};
    cam.position   = { 3.0f, 3.0f, 3.0f };
    cam.target     = { 0.0f, 0.0f, 0.0f };
    cam.up         = { 0.0f, 1.0f, 0.0f };
    cam.fovy       = 45.0f;
    cam.projection = CAMERA_PERSPECTIVE;

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
    } else {
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

    enum Tool { MOVE=0, ROTATE=1, SCALE=2, REFLECT=3 };
    int tool = MOVE;
    int reflectPlane = 0; // 0 XY, 1 YZ, 2 XZ

    bool dragging = false, reflectAppliedThisDrag = false;

    const float transSens = 0.01f;   // world units per pixel
    const float rotSens   = 0.01f;   // radians per pixel
    const float scaleSens = 0.01f;   // scale delta per pixel (uniform)

    const int PANEL_X = 600, PANEL_W = 200, UI_PAD = 10;
    const int UI_X = PANEL_X + UI_PAD, UI_W = PANEL_W - 2*UI_PAD; // 180
    const int LINE_H = 28, GAP = 8;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        Rectangle uiPanel = { (float)PANEL_X, 0, (float)PANEL_W, 600 };
        DrawRectangleRec(uiPanel, Color{245,245,245,255});
        DrawLine(PANEL_X, 0, PANEL_X, 600, LIGHTGRAY);
        int y = 10;

        DrawText("Figure:", UI_X, y, 18, DARKGRAY); y += 22;

        Rectangle rPrev = { (float)UI_X, (float)y, 86, (float)LINE_H };
        Rectangle rNext = { (float)(UI_X + UI_W - 86), (float)y, 86, (float)LINE_H };
        if (GuiButton(rPrev, "< Prev") && !files.empty()) {
            fileIndex = (fileIndex + (int)files.size() - 1) % (int)files.size();
            loadCurrentFile();
        }
        if (GuiButton(rNext, "Next >") && !files.empty()) {
            fileIndex = (fileIndex + 1) % (int)files.size();
            loadCurrentFile();
        }
        y += LINE_H + GAP;

        std::string label = files.empty() ? std::string("(no .obj in ./figures)") : files[fileIndex];
        DrawText(label.c_str(), UI_X, y, 16, DARKGRAY);
        y += 20;

        Rectangle rRefresh = { (float)UI_X, (float)y, (float)UI_W, (float)LINE_H };
        if (GuiButton(rRefresh, "Refresh list")) {
            files = listObjFiles("figures");
            if (!files.empty()) { fileIndex = fileIndex % (int)files.size(); loadCurrentFile(); }
        }
        y += LINE_H + GAP + 6;

        DrawText("Tool:", UI_X, y, 18, DARKGRAY); y += 22;

        Rectangle rMove   = { (float)UI_X, (float)y, (float)UI_W, (float)LINE_H };
        Rectangle rRotate = { (float)UI_X, (float)(y+LINE_H+GAP), (float)UI_W, (float)LINE_H };
        Rectangle rScale  = { (float)UI_X, (float)(y+2*(LINE_H+GAP)), (float)UI_W, (float)LINE_H };
        Rectangle rRefl   = { (float)UI_X, (float)(y+3*(LINE_H+GAP)), (float)UI_W, (float)LINE_H };

        if (GuiButton(rMove,   tool==MOVE   ? "[Move]"   : "Move"  )) tool = MOVE;
        if (GuiButton(rRotate, tool==ROTATE ? "[Rotate]" : "Rotate")) tool = ROTATE;
        if (GuiButton(rScale,  tool==SCALE  ? "[Scale]"  : "Scale" )) tool = SCALE;
        if (GuiButton(rRefl,   tool==REFLECT? "[Reflect]": "Reflect")) tool = REFLECT;

        y += 4*(LINE_H+GAP) + 6;

        if (tool == REFLECT) {
            DrawText("Reflect plane:", UI_X, y, 18, DARKGRAY); y += 22;

            Rectangle rXY = { (float)UI_X, (float)y, (float)UI_W, (float)LINE_H };
            Rectangle rYZ = { (float)UI_X, (float)(y+LINE_H+GAP), (float)UI_W, (float)LINE_H };
            Rectangle rXZ = { (float)UI_X, (float)(y+2*(LINE_H+GAP)), (float)UI_W, (float)LINE_H };

            if (GuiButton(rXY, reflectPlane==0 ? "[XY]" : "XY")) reflectPlane = 0;
            if (GuiButton(rYZ, reflectPlane==1 ? "[YZ]" : "YZ")) reflectPlane = 1;
            if (GuiButton(rXZ, reflectPlane==2 ? "[XZ]" : "XZ")) reflectPlane = 2;

            y += 3*(LINE_H+GAP) + 6;
            DrawText("Drag on image to apply\nreflection (once per drag).", UI_X, y, 16, DARKGRAY);
            y += 40;
        }

        Rectangle rReset = { (float)UI_X, (float)y, (float)UI_W, (float)LINE_H };
        if (GuiButton(rReset, "Reset transforms")) poly.recenterAndAutoscale();
        y += LINE_H + GAP;

        DrawText("Interaction:", UI_X, y, 18, DARKGRAY); y += 22;
        DrawText("Drag LMB inside the\nleft image to apply\nselected transform.\n\nMove: L/R->X, U/D->Y,\n      RMB vertical->Z\nRotate: U/D->X, L/R->Y,\n        RMB horizontal->Z\nScale: up increases,\n       down decreases", UI_X, y, 16, DARKGRAY);

        Vector2 m = GetMousePosition();
        bool inViewport = CheckCollisionPointRec(m, viewport);

        if (inViewport && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { dragging = true; reflectAppliedThisDrag = false; }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) { dragging = false; reflectAppliedThisDrag = false; }

        if (dragging) {
            Vector2 d = GetMouseDelta();
            float dx = d.x, dy = -d.y;
            if      (tool == MOVE)   { float dz = IsMouseButtonDown(MOUSE_RIGHT_BUTTON) ? dy : 0.0f; poly.applyTranslation(dx*transSens, dy*transSens, dz*transSens); }
            else if (tool == ROTATE) { float rz = IsMouseButtonDown(MOUSE_RIGHT_BUTTON) ? dx : 0.0f; poly.applyRotationXYZ(dy*rotSens, dx*rotSens, rz*rotSens); }
            else if (tool == SCALE)  { float s = 1.0f + (dy * scaleSens); if (s < 0.02f) s = 0.02f; poly.applyUniformScaleAboutCenter(s); }
            else if (tool == REFLECT) { if (!reflectAppliedThisDrag && (fabsf(dx)+fabsf(dy) > 2.0f)) { poly.applyReflection(static_cast<ReflectPlane>(reflectPlane)); reflectAppliedThisDrag = true; } }
        }

        
        BeginTextureMode(target);
        ClearBackground(RAYWHITE);
        BeginMode3D(cam);
            DrawLine3D({0,0,0},{1,0,0}, RED);
            DrawLine3D({0,0,0},{0,1,0}, GREEN);
            DrawLine3D({0,0,0},{0,0,1}, BLUE);
            for (auto &e : poly.edges) {
                Vector3 a = Vector3Transform(poly.vertices[e.first].toVec3(), poly.model);
                Vector3 b = Vector3Transform(poly.vertices[e.second].toVec3(), poly.model);
                DrawLine3D(a, b, BLACK);
            }
            float vr = 0.025f * UniformScaleFromMatrix(poly.model);
            for (auto &p : poly.vertices) {
                Vector3 v = Vector3Transform(p.toVec3(), poly.model);
                DrawSphere(v, vr, DARKGRAY);
            }
        EndMode3D();
        EndTextureMode();

        Rectangle src = { 0, 0, (float)target.texture.width, -(float)target.texture.height };
        Rectangle dst = viewport;
        DrawTexturePro(target.texture, src, dst, {0,0}, 0.0f, WHITE);
        DrawRectangleLinesEx(viewport, 2.0f, DARKGRAY);

        EndDrawing();
    }

    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}

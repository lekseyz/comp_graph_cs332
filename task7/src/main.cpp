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
#include <functional>

namespace fs = std::filesystem;

namespace MathFunctions {

    float func1(float x, float y) { return std::sin(std::sqrt(x * x + y * y)); }
    float func2(float x, float y) { return x * x + y * y; }
    float func3(float x, float y) { return std::sin(x) * std::cos(y); }
    float func4(float x, float y) { return x * x - y * y; }
    float func5(float x, float y) { return std::exp(-(x * x + y * y)); }

    float func6(float x, float y) { return std::sin(x * y); }
    float func7(float x, float y) { return std::cos(x) * std::sin(y); }
    float func8(float x, float y) { return std::sin(x) + std::cos(y); }
    float func9(float x, float y) { return std::sin(x * x + y * y); }
    float func10(float x, float y) { return std::cos(x * y) * std::sin(x + y); }

    float func11(float x, float y) { return x * x * x - 3 * x * y * y; }
    float func12(float x, float y) { return x * x * x + y * y * y; }
    float func13(float x, float y) { return x * x * x * x - y * y * y * y; }
    float func14(float x, float y) { return x * x * y - y * y * y / 3; }

    float func15(float x, float y) { return std::exp(-(x * x + y * y) / 2); }
    float func16(float x, float y) { return std::exp(std::sin(x + y)); }
    float func17(float x, float y) { return std::exp(-std::abs(x) - std::abs(y)); }

    float func18(float x, float y) { return std::sin(x) * std::sin(y) * std::exp(-(x * x + y * y) / 4); }
    float func19(float x, float y) { return (x * x + y * y) * std::sin(std::sqrt(x * x + y * y)); }
    float func20(float x, float y) { return std::atan2(y, x); }

    float func21(float x, float y) {
        float r2 = x * x + y * y;
        return (2 - r2) * std::exp(-r2 / 2);
    }
    float func22(float x, float y) {
        return std::sin(x) * std::sin(y);
    }
    float func23(float x, float y) {
        return 3 * (1 - x) * (1 - x) * std::exp(-(x * x) - (y + 1) * (y + 1))
            - 10 * (x / 5 - x * x * x - y * y * y * y * y) * std::exp(-x * x - y * y)
            - 1.0f / 3 * std::exp(-(x + 1) * (x + 1) - y * y);
    }
    float func24(float x, float y) {
        float r = std::sqrt(x * x + y * y);
        return r > 0.001f ? std::sin(10 * r) / r : 10.0f;
    }
    float func25(float x, float y) {
        return std::sqrt(1 + x * x + y * y);
    }
    float func26(float x, float y) {
        return std::sqrt(x * x + y * y);
    }
    float func27(float x, float y) {
        return x * std::cos(y) + y * std::sin(x);
    }
    float func28(float x, float y) {
        float r = std::sqrt(x * x + y * y);
        return std::cos(4 * r) * std::exp(-r / 2);
    }
    float func29(float x, float y) {
        return x * x * x - 3 * x * y * y;
    }
    float func30(float x, float y) {
        return (x * x + y - 11) * (x * x + y - 11) + (x + y * y - 7) * (x + y * y - 7);
    }
}

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

    bool saveOBJ(const std::string& path) const {
        std::ofstream out(path);
        if (!out.is_open()) return false;

        out << "# OBJ file generated by Surface Plotter\n";
        out << "# Vertices: " << vertices.size() << "\n";
        out << "# Faces: " << faces.size() << "\n\n";

        for (const auto& v : vertices) {
            out << "v " << v.x << " " << v.y << " " << v.z << "\n";
        }
        out << "\n";

        for (const auto& f : faces) {
            out << "f";
            for (int idx : f.idx) {
                out << " " << (idx + 1);
            }
            out << "\n";
        }

        out.close();
        return true;
    }

    static std::string getNextSurfaceFilename() {

        if (!fs::exists("figures")) {
            fs::create_directory("figures");
        }

        int index = 1;
        std::string filename;

        do {
            filename = "figures/surface_" + std::to_string(index) + ".obj";
            index++;
        } while (fs::exists(filename));

        return filename;
    }

    void generateSurface(std::function<float(float, float)> func,
        float x0, float x1, float y0, float y1,
        int xDivisions, int yDivisions) {
        clear();

        if (xDivisions < 2) xDivisions = 2;
        if (yDivisions < 2) yDivisions = 2;

        float dx = (x1 - x0) / (xDivisions - 1);
        float dy = (y1 - y0) / (yDivisions - 1);

        for (int i = 0; i < yDivisions; i++) {
            for (int j = 0; j < xDivisions; j++) {
                float x = x0 + j * dx;
                float y = y0 + i * dy;
                float z = func(x, y);
                vertices.emplace_back(x, y, z);
            }
        }

        for (int i = 0; i < yDivisions - 1; i++) {
            for (int j = 0; j < xDivisions - 1; j++) {
                int idx0 = i * xDivisions + j;
                int idx1 = idx0 + 1;
                int idx2 = idx0 + xDivisions;
                int idx3 = idx2 + 1;

                Face f1;
                f1.idx.push_back(idx0);
                f1.idx.push_back(idx1);
                f1.idx.push_back(idx2);
                faces.push_back(f1);

                Face f2;
                f2.idx.push_back(idx1);
                f2.idx.push_back(idx3);
                f2.idx.push_back(idx2);
                faces.push_back(f2);
            }
        }

        computeEdges();
        recenterAndAutoscale();
    }

    // Новая функция для создания фигуры вращения
    void generateSurfaceOfRevolution(const std::vector<Vector2>& profile, int axis, int divisions) {
        clear();
        
        if (profile.size() < 2 || divisions < 3) return;
        
        // Создаем вершины
        float angleStep = 2.0f * PI / divisions;
        
        for (int i = 0; i <= divisions; i++) {
            float angle = i * angleStep;
            float cosA = cosf(angle);
            float sinA = sinf(angle);
            
            for (const auto& point : profile) {
                float x, y, z;
                
                switch (axis) {
                    case 0: // Вращение вокруг оси X
                        x = point.y;  // высота
                        y = point.x * cosA;  // радиус * cos
                        z = point.x * sinA;  // радиус * sin
                        break;
                    case 1: // Вращение вокруг оси Y
                        x = point.x * cosA;
                        y = point.y;
                        z = point.x * sinA;
                        break;
                    case 2: // Вращение вокруг оси Z
                        x = point.x * cosA;
                        y = point.x * sinA;
                        z = point.y;
                        break;
                    default:
                        x = point.x * cosA;
                        y = point.y;
                        z = point.x * sinA;
                        break;
                }
                
                vertices.emplace_back(x, y, z);
            }
        }
        
        // Создаем грани
        int profileSize = profile.size();
        int totalDivisions = divisions + 1; // +1 для замыкания
        
        for (int i = 0; i < divisions; i++) {
            for (int j = 0; j < profileSize - 1; j++) {
                int current = i * profileSize + j;
                int next = i * profileSize + j + 1;
                int nextRing = ((i + 1) % totalDivisions) * profileSize + j;
                int nextRingNext = ((i + 1) % totalDivisions) * profileSize + j + 1;
                
                // Первый треугольник
                Face f1;
                f1.idx.push_back(current);
                f1.idx.push_back(next);
                f1.idx.push_back(nextRingNext);
                faces.push_back(f1);
                
                // Второй треугольник
                Face f2;
                f2.idx.push_back(current);
                f2.idx.push_back(nextRingNext);
                f2.idx.push_back(nextRing);
                faces.push_back(f2);
            }
        }
        
        computeEdges();
        recenterAndAutoscale();
    }

    void computeEdges() {
        edges.clear();
        std::set<std::pair<int, int>> uniq;
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
    InitWindow(1000, 700, "Polyhedron & Surface Tool - Lab 7");
    SetTargetFPS(60);

    const Rectangle viewport = { 0, 0, 650, 700 };
    RenderTexture2D target = LoadRenderTexture((int)viewport.width, (int)viewport.height);

    Camera3D cam = { 0 };
    cam.position = { 3.0f, 3.0f, 3.0f };
    cam.target = { 0.0f, 0.0f, 0.0f };
    cam.up = { 0.0f, 1.0f, 0.0f };
    cam.fovy = 45.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    Camera3D orthoCam = { 0 };
    orthoCam.position = { 3.0f, 3.0f, 3.0f };
    orthoCam.target = { 0.0f, 0.0f, 0.0f };
    orthoCam.up = { 0.0f, 1.0f, 0.0f };
    orthoCam.fovy = 4.0f;
    orthoCam.projection = CAMERA_ORTHOGRAPHIC;

    int projectionType = PROJ_PERSPECTIVE;
    float orthoSizeMultiplier = 5.0f;

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

    enum TextFieldID { P1X = 0, P1Y, P1Z, P2X, P2Y, P2Z, ANGLE, X0, X1, Y0, Y1, XDIV, YDIV, 
                      REV_PROFILE, REV_DIVISIONS, REV_AXIS, NONE };
    int activeField = NONE;

    bool dragging = false, reflectAppliedThisDrag = false;

    const float transSens = 0.01f;
    const float rotSens = 0.01f;
    const float scaleSens = 0.01f;

    const int PANEL_X = 650, PANEL_W = 350;
    const int LINE_H = 24, GAP = 4;

    Vector2 scrollOffset = { 0, 0 };

    int currentFunction = 0;
    const char* functionNames[] = {
        "sin(sqrt(x^2+y^2))",
        "x^2 + y^2",
        "sin(x)*cos(y)",
        "x^2 - y^2",
        "exp(-(x^2+y^2))",
        "sin(x*y)",
        "cos(x)*sin(y)",
        "sin(x)+cos(y)",
        "sin(x^2+y^2)",
        "cos(xy)*sin(x+y)",
        "x^3-3xy^2",
        "x^3+y^3",
        "x^4-y^4",
        "x^2*y-y^3/3",
        "exp(-(x^2+y^2)/2)",
        "exp(sin(x+y))",
        "exp(-|x|-|y|)",
        "sin(x)sin(y)exp(-r^2/4)",
        "(x^2+y^2)sin(r)",
        "atan2(y,x)",
        "Mexican hat",
        "Egg box",
        "Peaks (Matlab)",
        "Wave packet",
        "Hyperboloid",
        "Cone",
        "Twisted plane",
        "Ripples",
        "Monkey saddle",
        "Himmelblau"
    };
    std::function<float(float, float)> functions[] = {
        MathFunctions::func1, MathFunctions::func2, MathFunctions::func3,
        MathFunctions::func4, MathFunctions::func5, MathFunctions::func6,
        MathFunctions::func7, MathFunctions::func8, MathFunctions::func9,
        MathFunctions::func10, MathFunctions::func11, MathFunctions::func12,
        MathFunctions::func13, MathFunctions::func14, MathFunctions::func15,
        MathFunctions::func16, MathFunctions::func17, MathFunctions::func18,
        MathFunctions::func19, MathFunctions::func20, MathFunctions::func21,
        MathFunctions::func22, MathFunctions::func23, MathFunctions::func24,
        MathFunctions::func25, MathFunctions::func26, MathFunctions::func27,
        MathFunctions::func28, MathFunctions::func29, MathFunctions::func30
    };

    char x0_text[32] = "-5";
    char x1_text[32] = "5";
    char y0_text[32] = "-5";
    char y1_text[32] = "5";
    char xdiv_text[32] = "50";
    char ydiv_text[32] = "50";

    // Новые переменные для фигур вращения
    bool showSurfacePanel = false;
    bool showRevolutionPanel = false;
    std::string lastSavedFile = "";

    char revolutionProfileText[512] = "0.0 0.0\n1.0 0.0\n1.0 1.0\n0.0 1.0";
    char revolutionDivisionsText[32] = "36";
    int revolutionAxis = 1; // 0-X, 1-Y, 2-Z

    while (!WindowShouldClose()) {
        if (projectionType == PROJ_ORTHOGRAPHIC) {
            float scale = UniformScaleFromMatrix(poly.model);
            orthoCam.fovy = scale * orthoSizeMultiplier;
        }

        BeginDrawing();
        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        Rectangle uiPanel = { (float)PANEL_X, 0, (float)PANEL_W, 700 };
        DrawRectangleRec(uiPanel, Color{ 245,245,245,255 });
        DrawLine(PANEL_X, 0, PANEL_X, 700, LIGHTGRAY);

        Rectangle panelView = { (float)PANEL_X, 0, (float)PANEL_W, 700 };
        Rectangle panelContent = { (float)PANEL_X, 0, (float)(PANEL_W - 12), 2500 };

        Rectangle scissor = { 0 };
        GuiScrollPanel(panelView, NULL, panelContent, &scrollOffset, &scissor);

        BeginScissorMode((int)scissor.x, (int)scissor.y, (int)scissor.width, (int)scissor.height);

        int y = 10 + (int)scrollOffset.y;
        const int UI_X = PANEL_X + 10;
        const int UI_W = PANEL_W - 30;

        DrawText("3D Tool - Lab 7", UI_X, y, 18, BLACK);
        y += 30;

        Rectangle rFigureMode = { (float)UI_X, (float)y, (float)(UI_W / 3 - 2), (float)LINE_H };
        Rectangle rSurfaceMode = { (float)(UI_X + UI_W / 3 + 2), (float)y, (float)(UI_W / 3 - 2), (float)LINE_H };
        Rectangle rRevolutionMode = { (float)(UI_X + 2 * UI_W / 3 + 4), (float)y, (float)(UI_W / 3 - 2), (float)LINE_H };

        if (GuiButton(rFigureMode, (!showSurfacePanel && !showRevolutionPanel) ? "[Load Figures]" : "Load Figures")) {
            showSurfacePanel = false;
            showRevolutionPanel = false;
            activeField = NONE;
        }
        if (GuiButton(rSurfaceMode, showSurfacePanel ? "[Surface Gen]" : "Surface Gen")) {
            showSurfacePanel = true;
            showRevolutionPanel = false;
            activeField = NONE;
        }
        if (GuiButton(rRevolutionMode, showRevolutionPanel ? "[Revolution]" : "Revolution")) {
            showSurfacePanel = false;
            showRevolutionPanel = true;
            activeField = NONE;
        }
        y += LINE_H + GAP + 10;

        if (!showSurfacePanel && !showRevolutionPanel) {
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
            y += LINE_H + GAP + 10;
        }

        if (showSurfacePanel) {
            DrawText("Surface Generation:", UI_X, y, 16, DARKGRAY);
            y += 20;

            DrawText(TextFormat("Function #%d:", currentFunction + 1), UI_X, y, 14, DARKGRAY);
            y += 16;

            Rectangle rFuncPrev = { (float)UI_X, (float)y, 60, (float)LINE_H };
            Rectangle rFuncNext = { (float)(UI_X + UI_W - 60), (float)y, 60, (float)LINE_H };
            if (GuiButton(rFuncPrev, "< Prev")) {
                currentFunction = (currentFunction + 29) % 30;
            }
            if (GuiButton(rFuncNext, "Next >")) {
                currentFunction = (currentFunction + 1) % 30;
            }
            y += LINE_H + GAP;

            DrawText(functionNames[currentFunction], UI_X, y, 11, DARKGRAY);
            y += 18;

            DrawText("Range X:", UI_X, y, 14, DARKGRAY);
            y += 16;
            DrawText("From:", UI_X, y + 5, 11, DARKGRAY);
            Rectangle rx0 = { (float)(UI_X + 45), (float)y, (float)(UI_W / 2 - 50), (float)LINE_H };
            if (GuiTextBox(rx0, x0_text, 32, activeField == X0)) activeField = X0;
            DrawText("To:", UI_X + UI_W / 2 + 5, y + 5, 11, DARKGRAY);
            Rectangle rx1 = { (float)(UI_X + UI_W / 2 + 30), (float)y, (float)(UI_W / 2 - 35), (float)LINE_H };
            if (GuiTextBox(rx1, x1_text, 32, activeField == X1)) activeField = X1;
            y += LINE_H + GAP;

            DrawText("Range Y:", UI_X, y, 14, DARKGRAY);
            y += 16;
            DrawText("From:", UI_X, y + 5, 11, DARKGRAY);
            Rectangle ry0 = { (float)(UI_X + 45), (float)y, (float)(UI_W / 2 - 50), (float)LINE_H };
            if (GuiTextBox(ry0, y0_text, 32, activeField == Y0)) activeField = Y0;
            DrawText("To:", UI_X + UI_W / 2 + 5, y + 5, 11, DARKGRAY);
            Rectangle ry1 = { (float)(UI_X + UI_W / 2 + 30), (float)y, (float)(UI_W / 2 - 35), (float)LINE_H };
            if (GuiTextBox(ry1, y1_text, 32, activeField == Y1)) activeField = Y1;
            y += LINE_H + GAP;

            DrawText("Divisions:", UI_X, y, 14, DARKGRAY);
            y += 16;
            DrawText("X:", UI_X, y + 5, 11, DARKGRAY);
            Rectangle rxdiv = { (float)(UI_X + 20), (float)y, (float)(UI_W / 2 - 25), (float)LINE_H };
            if (GuiTextBox(rxdiv, xdiv_text, 32, activeField == XDIV)) activeField = XDIV;
            DrawText("Y:", UI_X + UI_W / 2 + 5, y + 5, 11, DARKGRAY);
            Rectangle rydiv = { (float)(UI_X + UI_W / 2 + 25), (float)y, (float)(UI_W / 2 - 30), (float)LINE_H };
            if (GuiTextBox(rydiv, ydiv_text, 32, activeField == YDIV)) activeField = YDIV;
            y += LINE_H + GAP + 10;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mouse = GetMousePosition();
                bool clickedOnField = CheckCollisionPointRec(mouse, rx0) ||
                    CheckCollisionPointRec(mouse, rx1) ||
                    CheckCollisionPointRec(mouse, ry0) ||
                    CheckCollisionPointRec(mouse, ry1) ||
                    CheckCollisionPointRec(mouse, rxdiv) ||
                    CheckCollisionPointRec(mouse, rydiv);
                if (!clickedOnField && !CheckCollisionPointRec(mouse, panelView)) activeField = NONE;
            }

            Rectangle rGenerate = { (float)UI_X, (float)y, (float)UI_W, (float)(LINE_H + 4) };
            if (GuiButton(rGenerate, "GENERATE SURFACE")) {
                activeField = NONE;
                try {
                    float x0 = std::stof(x0_text);
                    float x1 = std::stof(x1_text);
                    float y0 = std::stof(y0_text);
                    float y1 = std::stof(y1_text);
                    int xdiv = std::stoi(xdiv_text);
                    int ydiv = std::stoi(ydiv_text);

                    poly.generateSurface(functions[currentFunction], x0, x1, y0, y1, xdiv, ydiv);
                    TraceLog(LOG_INFO, "Surface generated: %d vertices, %d faces",
                        (int)poly.vertices.size(), (int)poly.faces.size());
                }
                catch (...) {
                    TraceLog(LOG_WARNING, "Invalid input parameters");
                }
            }
            y += LINE_H + GAP + 10;

            Rectangle rSave = { (float)UI_X, (float)y, (float)UI_W, (float)LINE_H };
            if (GuiButton(rSave, "Save to figures/")) {
                activeField = NONE;
                if (!poly.vertices.empty()) {
                    std::string filename = Polyhedron::getNextSurfaceFilename();
                    if (poly.saveOBJ(filename)) {
                        lastSavedFile = filename;
                        TraceLog(LOG_INFO, "Saved to %s", filename.c_str());

                        files = listObjFiles("figures");
                    }
                    else {
                        TraceLog(LOG_WARNING, "Failed to save to %s", filename.c_str());
                    }
                }
            }
            y += LINE_H + GAP;

            if (!lastSavedFile.empty()) {

                size_t pos = lastSavedFile.find_last_of("/\\");
                std::string displayName = (pos != std::string::npos) ? lastSavedFile.substr(pos + 1) : lastSavedFile;
                DrawText("Last saved:", UI_X, y, 11, DARKGRAY);
                y += 14;
                DrawText(displayName.c_str(), UI_X, y, 10, GREEN);
                y += 16;
            }
            y += 4;
        }

        // Новая панель для фигур вращения
        if (showRevolutionPanel) {
            DrawText("Revolution Surface:", UI_X, y, 16, DARKGRAY);
            y += 20;

            DrawText("Profile points (x y per line):", UI_X, y, 14, DARKGRAY);
            y += 16;
            Rectangle rProfile = { (float)UI_X, (float)y, (float)UI_W, (float)LINE_H };
            if (GuiTextBox(rProfile, revolutionProfileText, sizeof(revolutionProfileText), activeField == REV_PROFILE)) {
                activeField = (activeField == REV_PROFILE) ? NONE : REV_PROFILE;
            }
            y += LINE_H + GAP;

            DrawText("Axis of revolution:", UI_X, y, 14, DARKGRAY);
            y += 16;
            Rectangle rAxisX = { (float)UI_X, (float)y, (float)(UI_W / 3 - 2), (float)LINE_H };
            Rectangle rAxisY = { (float)(UI_X + UI_W / 3 + 2), (float)y, (float)(UI_W / 3 - 2), (float)LINE_H };
            Rectangle rAxisZ = { (float)(UI_X + 2 * UI_W / 3 + 4), (float)y, (float)(UI_W / 3 - 2), (float)LINE_H };
            if (GuiButton(rAxisX, revolutionAxis == 0 ? "[X]" : "X")) revolutionAxis = 0;
            if (GuiButton(rAxisY, revolutionAxis == 1 ? "[Y]" : "Y")) revolutionAxis = 1;
            if (GuiButton(rAxisZ, revolutionAxis == 2 ? "[Z]" : "Z")) revolutionAxis = 2;
            y += LINE_H + GAP;

            DrawText("Divisions:", UI_X, y, 14, DARKGRAY);
            y += 16;
            Rectangle rRevDiv = { (float)UI_X, (float)y, (float)UI_W, (float)LINE_H };
            if (GuiTextBox(rRevDiv, revolutionDivisionsText, 32, activeField == REV_DIVISIONS)) activeField = REV_DIVISIONS;
            y += LINE_H + GAP + 10;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                Vector2 mouse = GetMousePosition();
                bool clickedOnField = CheckCollisionPointRec(mouse, rProfile) ||
                    CheckCollisionPointRec(mouse, rRevDiv);
                if (!clickedOnField && !CheckCollisionPointRec(mouse, panelView)) activeField = NONE;
            }

            Rectangle rGenerateRev = { (float)UI_X, (float)y, (float)UI_W, (float)(LINE_H + 4) };
            if (GuiButton(rGenerateRev, "GENERATE REVOLUTION")) {
                activeField = NONE;
                try {
                    // Парсим точки образующей
                    std::vector<Vector2> profile;
                    std::istringstream iss(revolutionProfileText);
                    std::string line;
                    while (std::getline(iss, line)) {
                        std::istringstream ls(line);
                        float x, y;
                        if (ls >> x >> y) {
                            profile.push_back({x, y});
                        }
                    }
                    
                    int divisions = std::stoi(revolutionDivisionsText);
                    
                    if (profile.size() >= 2 && divisions >= 3) {
                        poly.generateSurfaceOfRevolution(profile, revolutionAxis, divisions);
                        TraceLog(LOG_INFO, "Revolution surface generated: %d vertices, %d faces",
                            (int)poly.vertices.size(), (int)poly.faces.size());
                    } else {
                        TraceLog(LOG_WARNING, "Need at least 2 profile points and 3 divisions");
                    }
                }
                catch (...) {
                    TraceLog(LOG_WARNING, "Invalid input parameters for revolution");
                }
            }
            y += LINE_H + GAP + 10;

            Rectangle rSave = { (float)UI_X, (float)y, (float)UI_W, (float)LINE_H };
            if (GuiButton(rSave, "Save to figures/")) {
                activeField = NONE;
                if (!poly.vertices.empty()) {
                    std::string filename = Polyhedron::getNextSurfaceFilename();
                    if (poly.saveOBJ(filename)) {
                        lastSavedFile = filename;
                        TraceLog(LOG_INFO, "Saved to %s", filename.c_str());

                        files = listObjFiles("figures");
                    }
                    else {
                        TraceLog(LOG_WARNING, "Failed to save to %s", filename.c_str());
                    }
                }
            }
            y += LINE_H + GAP;

            if (!lastSavedFile.empty()) {
                size_t pos = lastSavedFile.find_last_of("/\\");
                std::string displayName = (pos != std::string::npos) ? lastSavedFile.substr(pos + 1) : lastSavedFile;
                DrawText("Last saved:", UI_X, y, 11, DARKGRAY);
                y += 14;
                DrawText(displayName.c_str(), UI_X, y, 10, GREEN);
                y += 16;
            }
            y += 4;
        }

        DrawText("Projection:", UI_X, y, 16, DARKGRAY); y += 20;
        Rectangle rPersp = { (float)UI_X, (float)y, (float)UI_W / 2 - 5, (float)LINE_H };
        Rectangle rOrtho = { (float)(UI_X + UI_W / 2 + 5), (float)y, (float)UI_W / 2 - 5, (float)LINE_H };
        if (GuiButton(rPersp, projectionType == PROJ_PERSPECTIVE ? "[Perspective]" : "Perspective"))
            projectionType = PROJ_PERSPECTIVE;
        if (GuiButton(rOrtho, projectionType == PROJ_ORTHOGRAPHIC ? "[Orthographic]" : "Orthographic"))
            projectionType = PROJ_ORTHOGRAPHIC;
        y += LINE_H + GAP + 4;

        if (projectionType == PROJ_ORTHOGRAPHIC) {
            DrawText("Ortho Size:", UI_X, y, 14, DARKGRAY); y += 16;
            GuiSliderBar({ (float)UI_X, (float)y, (float)UI_W, (float)LINE_H }, "Small", "Large", &orthoSizeMultiplier, 2.0f, 10.0f);
            y += LINE_H + GAP + 4;
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

            const int inputW = 85;
            const int spacing = 6;

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
                    TraceLog(LOG_INFO, "Rotation applied");
                }
                catch (...) {
                    TraceLog(LOG_WARNING, "Invalid input");
                }
            }
            y += LINE_H + GAP + 6;
        }

        Rectangle rReset = { (float)UI_X, (float)y, (float)UI_W, (float)LINE_H };
        if (GuiButton(rReset, "Reset transforms")) {
            poly.recenterAndAutoscale();
        }
        y += LINE_H + GAP + 4;

        DrawText("Interaction:", UI_X, y, 16, DARKGRAY); y += 18;
        DrawText("Drag LMB:\nMove: L/R->X, U/D->Y\nRotate: U/D->X, L/R->Y\nScale: U/D",
            UI_X, y, 12, DARKGRAY);
        y += 50;

        DrawText(TextFormat("Vertices: %d", (int)poly.vertices.size()), UI_X, y, 12, DARKGRAY);
        y += 16;
        DrawText(TextFormat("Faces: %d", (int)poly.faces.size()), UI_X, y, 12, DARKGRAY);
        y += 16;
        DrawText(TextFormat("Edges: %d", (int)poly.edges.size()), UI_X, y, 12, DARKGRAY);

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
            if (tool == MOVE) {
                float dz = IsMouseButtonDown(MOUSE_RIGHT_BUTTON) ? dy : 0.0f;
                poly.applyTranslation(dx * transSens, dy * transSens, dz * transSens);
            }
            else if (tool == ROTATE) {
                float rz = IsMouseButtonDown(MOUSE_RIGHT_BUTTON) ? dx : 0.0f;
                poly.applyRotationXYZ(dy * rotSens, dx * rotSens, rz * rotSens);
            }
            else if (tool == SCALE) {
                float s = 1.0f + (dy * scaleSens);
                if (s < 0.02f) s = 0.02f;
                poly.applyUniformScaleAboutCenter(s);
            }
            else if (tool == REFLECT) {
                if (!reflectAppliedThisDrag && (fabsf(dx) + fabsf(dy) > 2.0f)) {
                    poly.applyReflection(static_cast<ReflectPlane>(reflectPlane));
                    reflectAppliedThisDrag = true;
                }
            }
        }

        BeginTextureMode(target);
        ClearBackground(RAYWHITE);

        Camera3D currentCamera = (projectionType == PROJ_ORTHOGRAPHIC) ? orthoCam : cam;

        BeginMode3D(currentCamera);
        DrawGrid(10, 1.0f);
        DrawLine3D({ 0,0,0 }, { 1,0,0 }, RED);
        DrawLine3D({ 0,0,0 }, { 0,1,0 }, GREEN);
        DrawLine3D({ 0,0,0 }, { 0,0,1 }, BLUE);

        for (auto& e : poly.edges) {
            Vector3 a = Vector3Transform(poly.vertices[e.first].toVec3(), poly.model);
            Vector3 b = Vector3Transform(poly.vertices[e.second].toVec3(), poly.model);
            DrawLine3D(a, b, BLACK);
        }

        if (poly.vertices.size() < 2000) {
            float vr = 0.015f * UniformScaleFromMatrix(poly.model);
            for (auto& p : poly.vertices) {
                Vector3 v = Vector3Transform(p.toVec3(), poly.model);
                DrawSphere(v, vr, DARKGRAY);
            }
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

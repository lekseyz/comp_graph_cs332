#define RAYGUI_IMPLEMENTATION
#include "raylib.h"
#include "raygui.h"
#include "raymath.h"
#include "rlgl.h"
#include <vector>
#include <string>
#include <filesystem>
#include <cmath>

#define TEST_BACKFACE


struct Obj3D {
    Model model;
    Vector3 pos{0,0,0};
    Vector3 rot{0,0,0};
    float rotSpeed{0.8f};
    float scale{1.0f};
    Color color{200,200,220,255};
    bool valid{false};
};

static Vector3 GetVertex(const Mesh& m, int idx) {
    int i = idx*3;
    return { m.vertices[i+0], m.vertices[i+1], m.vertices[i+2] };
}

static Vector3 TransformVec(Vector3 v, Matrix M) {
    return Vector3Transform(v, M);
}

static void DrawCulledMesh(const Mesh& mesh, Matrix objM, const Camera3D& cam, const Vector3& orthoViewDir, bool perspective, Color col, bool test) {
    rlDisableBackfaceCulling();
    //rlEnableDepthTest();
    rlDisableDepthTest();
    rlBegin(RL_TRIANGLES);
    int tcount = mesh.triangleCount;
    bool hasIdx = mesh.indices != nullptr;
    for (int t = 0; t < tcount; t++) {
        int i1, i2, i3;
        if (hasIdx) {
            i1 = mesh.indices[t*3+0];
            i2 = mesh.indices[t*3+1];
            i3 = mesh.indices[t*3+2];
        } else {
            i1 = t*3+0; i2 = t*3+1; i3 = t*3+2;
        }
        Vector3 v1 = TransformVec(GetVertex(mesh, i1), objM);
        Vector3 v2 = TransformVec(GetVertex(mesh, i2), objM);
        Vector3 v3 = TransformVec(GetVertex(mesh, i3), objM);
        Vector3 e1 = Vector3Subtract(v2, v1);
        Vector3 e2 = Vector3Subtract(v3, v1);
        Vector3 n = Vector3Normalize(Vector3CrossProduct(e1, e2));
        Vector3 center = {(v1.x+v2.x+v3.x)/3.0f,(v1.y+v2.y+v3.y)/3.0f,(v1.z+v2.z+v3.z)/3.0f};
        Vector3 lookVec = Vector3Subtract(center, cam.position);
        if (Vector3DotProduct(n, lookVec) < 0.0f) {
            rlColor4ub(col.r, col.g, col.b, col.a);
            rlVertex3f(v1.x, v1.y, v1.z);
            rlVertex3f(v2.x, v2.y, v2.z);
            rlVertex3f(v3.x, v3.y, v3.z);
        }
        else if (test) {
            rlColor4ub(0, 0, 0, 100);
            rlVertex3f(v1.x, v1.y, v1.z);
            rlVertex3f(v2.x, v2.y, v2.z);
            rlVertex3f(v3.x, v3.y, v3.z);
        }
    }
    rlEnd();
}

static void DrawObjCulled(const Obj3D& obj, const Camera3D& cam, const Vector3& orthoViewDir, bool perspective, bool test) {
    Matrix S = MatrixScale(obj.scale, obj.scale, obj.scale);
    Matrix R = MatrixRotateXYZ({obj.rot.x, obj.rot.y, obj.rot.z});
    Matrix T = MatrixTranslate(obj.pos.x, obj.pos.y, obj.pos.z);
    Matrix M = MatrixMultiply(MatrixMultiply(S, R), T);
    for (int i = 0; i < obj.model.meshCount; i++) {
        DrawCulledMesh(obj.model.meshes[i], M, cam, orthoViewDir, perspective, obj.color, test);
    }
}

int main() {
    InitWindow(1280, 800, "CG TASK 8");

    Camera3D cam = {0};
    cam.position = {6.0f, 4.0f, 8.0f};
    cam.target = {0.0f, 0.8f, 0.0f};
    cam.up = {0.0f, 1.0f, 0.0f};
    cam.fovy = 60.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    std::vector<std::string> objFiles;
    for (auto& p : std::filesystem::directory_iterator("figures")) {
        if (!p.is_regular_file()) continue;
        auto ext = p.path().extension().string();
        for (auto& c : ext) c = (char)tolower(c);
        if (ext == ".obj") objFiles.push_back(p.path().string());
    }
    size_t nextFile = 0;

    std::vector<Obj3D> scene;
    int selected = -1;
    bool moveMode = false;
    bool perspective = true;
    bool test = false;

    float viewYaw = 3.1415926f;
    float viewPitch = 0.0f;

    auto addObj = [&](const std::string& path){
        Obj3D o;
        o.model = LoadModel(path.c_str());
        o.valid = true;
        o.pos = {(float)(int)scene.size()*1.8f, 0.0f, 0.0f};
        o.rot = {0,0,0};
        o.scale = 1.0f;
        Color cols[6] = {RED, ORANGE, YELLOW, GREEN, SKYBLUE, VIOLET};
        o.color = cols[scene.size()%6];
        scene.push_back(o);
        selected = (int)scene.size()-1;
    };

    int panelW = 280;
    bool dummy = false;

    while (!WindowShouldClose()) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        Rectangle panel = {(float)(sw - panelW), 0, (float)panelW, (float)sh};

        float dt = GetFrameTime();

        if (IsKeyPressed(KEY_P)) {
            perspective = !perspective;
            cam.projection = perspective ? CAMERA_PERSPECTIVE : CAMERA_ORTHOGRAPHIC;
            cam.fovy = perspective ? 60.0f : 20.0f;
        }

        float yawSpeed = 1.4f;
        float pitchSpeed = 1.0f;
        if (IsKeyDown(KEY_J)) viewYaw -= yawSpeed*dt;
        if (IsKeyDown(KEY_L)) viewYaw += yawSpeed*dt;
        if (IsKeyDown(KEY_I)) viewPitch += pitchSpeed*dt;
        if (IsKeyDown(KEY_K)) viewPitch -= pitchSpeed*dt;
        if (viewPitch > 1.5f) viewPitch = 1.5f;
        if (viewPitch < -1.5f) viewPitch = -1.5f;

        Vector3 viewDir = {
            cosf(viewPitch)*sinf(viewYaw),
            sinf(viewPitch),
            cosf(viewPitch)*cosf(viewYaw)
        };

        if (moveMode && selected >= 0 && selected < (int)scene.size()) {
            float sp = 3.0f*dt;
            if (IsKeyDown(KEY_W)) scene[selected].pos.z -= sp;
            if (IsKeyDown(KEY_S)) scene[selected].pos.z += sp;
            if (IsKeyDown(KEY_A)) scene[selected].pos.x -= sp;
            if (IsKeyDown(KEY_D)) scene[selected].pos.x += sp;
            if (IsKeyDown(KEY_Q)) scene[selected].pos.y -= sp;
            if (IsKeyDown(KEY_E)) scene[selected].pos.y += sp;
            if (IsKeyPressed(KEY_LEFT)) { selected = (selected-1 + (int)scene.size()) % (int)scene.size(); }
            if (IsKeyPressed(KEY_RIGHT)) { selected = (selected+1) % (int)scene.size(); }
        }

        for (auto& o : scene) o.rot.y += o.rotSpeed*dt;

        BeginDrawing();
        ClearBackground(Color{18,18,24,255});

        BeginMode3D(cam);
        DrawGrid(20, 1.0f);
        for (int i = 0; i < (int)scene.size(); i++) DrawObjCulled(scene[i], cam, viewDir, perspective, test);
        if (selected >= 0 && selected < (int)scene.size()) DrawSphereWires(scene[selected].pos, 0.15f, 10, 10, GOLD);
        EndMode3D();

        GuiSetStyle(DEFAULT, TEXT_SIZE, 18);
        GuiPanel(panel, nullptr);

        Rectangle r = panel;
        float x = r.x + 16;
        float y = r.y + 16;
        float w = r.width - 32;
        float h = 44;

        if (GuiButton({x,y,w,h}, "ADD OBJ")) {
            if (!objFiles.empty()) {
                addObj(objFiles[nextFile]);
                nextFile = (nextFile+1)%objFiles.size();
            }
        }
        y += h + 12;

        if (GuiToggle({x,y,w,h}, "MOVE WASD", &moveMode)) {
            if (!scene.empty() && selected < 0) selected = 0;
        }
        y += h + 12;

        if (GuiToggle({x,y,w,h}, "TEST BACKFACE", &test)) { }
        y += h + 12;

        if (GuiButton({x,y,w,h}, "CLEAR")) {
            for (auto& o : scene) if (o.valid) UnloadModel(o.model);
            scene.clear();
            selected = -1;
        }
        y += h + 18;

        // if (GuiToggle({x,y,w,h}, perspective ? "PERSPECTIVE" : "ORTOGRAPHIC", &perspective)) {
        //     cam.projection = perspective ? CAMERA_PERSPECTIVE : CAMERA_ORTHOGRAPHIC;
        //     cam.fovy = perspective ? 60.0f : 20.0f;
        // }
        // y += h + 18;

        GuiLabel({x,y,w,28}, TextFormat("OBJs: %d", (int)scene.size())); y += 30;
        GuiLabel({x,y,w,28}, TextFormat("CHOSEN: %d", selected)); y += 30;

        if (objFiles.empty()) GuiLabel({x, (float)sh-42, w, 28}, "THERES NO OBJES");
        else GuiLabel({x, (float)sh-42, w, 28}, TextFormat("NEXT: %s", std::filesystem::path(objFiles[nextFile]).filename().string().c_str()));

        DrawFPS(10, 10);
        EndDrawing();
    }

    for (auto& o : scene) if (o.valid) UnloadModel(o.model);
    CloseWindow();
    return 0;
}

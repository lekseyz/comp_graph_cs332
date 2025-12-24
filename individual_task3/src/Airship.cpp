#include "Airship.h"
#include "Utils.h"
#include <iostream>

Airship::Airship() {
    position = (Vector3){ 0.0f, 20.0f, 0.0f };
    velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
    speed = 15.0f;
    rotationY = 0.0f;
    isAimingMode = false;
    
    camera.position = (Vector3){ 0.0f, 10.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

Airship::~Airship() {
    UnloadModel(model);
    UnloadShader(shader);
}

void Airship::Init() {
    model = LoadModel("models/Low-Poly_airship.obj");
    
    Texture2D texDiffuse = LoadTexture("models/Low-Poly_airship.png");
    if (texDiffuse.id == 0) {
        Image img = GenImageColor(1024, 1024, WHITE);
        texDiffuse = LoadTextureFromImage(img);
        UnloadImage(img);
    }
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texDiffuse;

    GenMeshTangents(&model.meshes[0]);

    shader = LoadShader("shaders/base.vs", "shaders/lighting.fs");
    shader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(shader, "texture1");

    model.materials[0].shader = shader;

    Texture2D texNormal = LoadTexture("models/Low-Poly_airship_nrm.png"); 
    if (texNormal.id == 0) {
        texNormal = GenerateFlatNormalMap(1024, 1024);
    }
    model.materials[0].maps[MATERIAL_MAP_NORMAL].texture = texNormal;
}

void Airship::Update(float dt) {
    Vector3 move = { 0.0f, 0.0f, 0.0f };
    
    if (IsKeyDown(KEY_W)) move.x += 1.0f;
    if (IsKeyDown(KEY_S)) move.x -= 1.0f;
    if (IsKeyDown(KEY_A)) move.z -= 1.0f;
    if (IsKeyDown(KEY_D)) move.z += 1.0f;
    if (IsKeyDown(KEY_SPACE)) move.y += 1.0f;
    if (IsKeyDown(KEY_LEFT_SHIFT)) move.y -= 1.0f;

    if (IsKeyPressed(KEY_C)) isAimingMode = !isAimingMode;

    if (Vector3Length(move) > 0) {
        move = Vector3Normalize(move);
        
        if (move.x != 0 || move.z != 0) {
            float targetAngle = atan2(move.x, move.z) * RAD2DEG;
            rotationY = targetAngle; 
        }
    }

    position = Vector3Add(position, Vector3Scale(move, speed * dt));

    if (isAimingMode) {
        camera.position = Vector3Add(position, (Vector3){ 0.0f, -2.0f, 0.0f });
        camera.target = Vector3Add(position, (Vector3){ 0.0f, -100.0f, 0.0f });
        camera.up = (Vector3){ 1.0f, 0.0f, 0.0f };
    } else {
        camera.position = Vector3Add(position, (Vector3){ -20.0f, 15.0f, 0.0f });
        camera.target = position;
        camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    }
}

void Airship::Draw() {
    DrawModelEx(model, position, (Vector3){0, 1, 0}, rotationY - 65.f, (Vector3){.01f,.01f,.01f}, WHITE);
}
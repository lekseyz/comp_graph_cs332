#pragma once
#include "raylib.h"
#include "raymath.h"
#include <string>

class Airship {
public:
    Vector3 position;
    Vector3 velocity;
    float speed;
    float rotationY;
    
    Model model;
    Shader shader;
    Camera3D camera;
    bool isAimingMode;

    Airship();
    ~Airship();

    void Init();
    void Update(float deltaTime);
    void Draw();
};
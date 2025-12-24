#pragma once
#include "raylib.h"
#include <vector>
#include <string>

struct GameObject {
    Model model;
    Vector3 position;
    Vector3 scale;
    float rotation;
    Color color;
    bool isActive;
};

class Scene {
private:
    std::vector<GameObject> houses;
    std::vector<GameObject> decorations;
    std::vector<GameObject> clouds;
    std::vector<GameObject> balloons;
    
    Model modelCottage;
    Model modelPlant;
    Model modelBalloon;
    
public:
    Scene();
    ~Scene();

    void Init();
    void GenerateLevel();
    void Draw();
    void Unload();
};
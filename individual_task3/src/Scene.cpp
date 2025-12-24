#include "Scene.h"
#include <iostream>

Scene::Scene() {}

Scene::~Scene() {
    Unload();
}

void Scene::Init() {
    modelCottage = LoadModel("models/home.obj");
    Texture2D texCottage = LoadTexture("models/home.png");
    if (texCottage.id != 0) modelCottage.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texCottage;

    modelPlant = LoadModel("models/plant.obj");
    Texture2D texPlant = LoadTexture("models/plant.png");
    if (texPlant.id != 0) modelPlant.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texPlant;
    
    modelBalloon = LoadModel("models/bird.obj");
    Texture2D texBird = LoadTexture("models/bird.png");
    if (texBird.id != 0) modelBalloon.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texBird;

    GenerateLevel();
}

void Scene::GenerateLevel() {
    for (int i = 0; i < 6; i++) {
        GameObject obj;
        obj.model = modelCottage;
        obj.position = (Vector3){ 
            (float)GetRandomValue(-100, 100), 
            0.0f, 
            (float)GetRandomValue(-100, 100) 
        };
        obj.scale = (Vector3){ .02f, .02f, .02f };
        obj.rotation = (float)GetRandomValue(0, 360);
        obj.color = WHITE;
        houses.push_back(obj);
    }

    for (int i = 0; i < 10; i++) {
        GameObject obj;
        obj.model = modelPlant;

        obj.position = (Vector3){ 
            (float)GetRandomValue(-120, 120), 
            0.0f, 
            (float)GetRandomValue(-120, 120) 
        };
        obj.scale = (Vector3){ 0.9f, 0.9f, 0.9f };
        obj.rotation = 0.f;
        obj.color = GRAY; 
        decorations.push_back(obj);
    }

    for (int i = 0; i < 15; i++) {
        GameObject obj;
        obj.position = (Vector3){ 
            (float)GetRandomValue(-150, 150), 
            (float)GetRandomValue(30, 60), 
            (float)GetRandomValue(-150, 150) 
        };
        obj.scale = (Vector3){ (float)GetRandomValue(5, 10), (float)GetRandomValue(3, 5), (float)GetRandomValue(5, 10) };
        
        obj.color = (Color){ 255, 255, 255, 150 }; 
        clouds.push_back(obj);
    }

    for (int i = 0; i < 5; i++) {
        GameObject obj;
        obj.model = modelBalloon;
        obj.position = (Vector3){ 
            (float)GetRandomValue(-100, 100), 
            (float)GetRandomValue(20, 50), 
            (float)GetRandomValue(-100, 100) 
        };
        obj.scale = (Vector3){ 0.1f, .1f, .1f };
        obj.color = WHITE;
        obj.rotation = (float)GetRandomValue(90, 270);
        balloons.push_back(obj);
    }
}

void Scene::Draw() {
    DrawPlane((Vector3){0, -0.1f, 0}, (Vector2){1000, 1000}, (Color){50, 100, 50, 255});

    for (const auto& h : houses) {
        DrawModelEx(h.model, h.position, (Vector3){1, 0, 0}, -90.0f, h.scale, h.color);
    }
    for (const auto& d : decorations) {
        DrawModelEx(d.model, d.position, (Vector3){1, 0, 0}, d.rotation, d.scale, d.color);
    }
    for (const auto& b : balloons) {
        DrawModelEx(b.model, b.position, (Vector3){1, 1, 0}, b.rotation, b.scale, b.color);
    }

    for (const auto& c : clouds) {
        DrawSphere(c.position, c.scale.x, c.color);
        DrawSphere((Vector3){c.position.x + 5, c.position.y, c.position.z}, c.scale.x * 0.7f, c.color);
        DrawSphere((Vector3){c.position.x - 5, c.position.y + 2, c.position.z}, c.scale.x * 0.8f, c.color);
    }
}

void Scene::Unload() {
    UnloadModel(modelCottage);
    UnloadModel(modelPlant);
    UnloadModel(modelBalloon);
}
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <omp.h> 

#define MAX_DEPTH 100
#define RENDER_WIDTH 600
#define RENDER_HEIGHT 600
#define EPSILON 0.000001f

struct MyMaterial {
    Color color;
    float reflectivity;
    float transparency;
    float refractiveIndex;
};

struct Sphere {
    Vector3 position;
    float radius;
    MyMaterial material;
};

struct Box {
    Vector3 min;
    Vector3 max;
    MyMaterial material;
};

struct Light {
    Vector3 position;
    Color color;
    float intensity;
    bool active;
};

struct HitInfo {
    bool hit;
    float dist;
    Vector3 point;
    Vector3 normal;
    MyMaterial material;
};

bool RaySphereIntersect(Vector3 rayOrigin, Vector3 rayDir, const Sphere& sphere, float* dist) {
    Vector3 oc = Vector3Subtract(rayOrigin, sphere.position);
    float b = Vector3DotProduct(oc, rayDir);
    float c = Vector3DotProduct(oc, oc) - sphere.radius * sphere.radius;
    float h = b * b - c;
    if (h > 0.0f) {
        float sqrtH = sqrtf(h);
        float t = -b - sqrtH;
        if (t > EPSILON) { *dist = t; return true; }
        t = -b + sqrtH;
        if (t > EPSILON) { *dist = t; return true; }
    }
    return false;
}

bool RayBoxIntersect(Vector3 rayOrigin, Vector3 rayDir, const Box& box, float* dist, Vector3* outNormal) {
    float tMin = (box.min.x - rayOrigin.x) / rayDir.x;
    float tMax = (box.max.x - rayOrigin.x) / rayDir.x;
    Vector3 nMin = { -1, 0, 0 };
    Vector3 nMax = { 1, 0, 0 };

    if (rayDir.x < 0) { std::swap(tMin, tMax); std::swap(nMin, nMax); }

    float tyMin = (box.min.y - rayOrigin.y) / rayDir.y;
    float tyMax = (box.max.y - rayOrigin.y) / rayDir.y;
    Vector3 nyMin = { 0, -1, 0 };
    Vector3 nyMax = { 0, 1, 0 };

    if (rayDir.y < 0) { std::swap(tyMin, tyMax); std::swap(nyMin, nyMax); }

    if ((tMin > tyMax) || (tyMin > tMax)) return false;
    
    if (tyMin > tMin) { tMin = tyMin; nMin = nyMin; }
    if (tyMax < tMax) { tMax = tyMax; nMax = nyMax; }

    float tzMin = (box.min.z - rayOrigin.z) / rayDir.z;
    float tzMax = (box.max.z - rayOrigin.z) / rayDir.z;
    Vector3 nzMin = { 0, 0, -1 };
    Vector3 nzMax = { 0, 0, 1 };

    if (rayDir.z < 0) { std::swap(tzMin, tzMax); std::swap(nzMin, nzMax); }

    if ((tMin > tzMax) || (tzMin > tMax)) return false;

    if (tzMin > tMin) { tMin = tzMin; nMin = nzMin; }
    if (tzMax < tMax) { tMax = tzMax; nMax = nzMax; }

    if (tMin < EPSILON) {
        if (tMax < EPSILON) return false;
        *dist = tMax;
        *outNormal = nMax;
    } else {
        *dist = tMin;
        *outNormal = nMin;
    }
    return true;
}

std::vector<Sphere> spheres;
std::vector<Box> boxes;
std::vector<Box> walls;
std::vector<Light> lights;
int mirrorWallIndex = 0;

HitInfo Trace(Vector3 origin, Vector3 dir) {
    HitInfo closest = { false, 1e30f, {0}, {0}, {{0},0,0,0} };
    float dist;
    Vector3 norm;

    for (const auto& s : spheres) {
        if (RaySphereIntersect(origin, dir, s, &dist)) {
            if (dist < closest.dist) {
                closest.hit = true;
                closest.dist = dist;
                closest.point = Vector3Add(origin, Vector3Scale(dir, dist));
                closest.normal = Vector3Normalize(Vector3Subtract(closest.point, s.position));
                closest.material = s.material;
            }
        }
    }

    for (const auto& b : boxes) {
        if (RayBoxIntersect(origin, dir, b, &dist, &norm)) {
            if (dist < closest.dist) {
                closest.hit = true;
                closest.dist = dist;
                closest.point = Vector3Add(origin, Vector3Scale(dir, dist));
                closest.normal = norm;
                closest.material = b.material;
            }
        }
    }

    for (int i = 0; i < walls.size(); i++) {
        if (RayBoxIntersect(origin, dir, walls[i], &dist, &norm)) {
            if (dist < closest.dist) {
                closest.hit = true;
                closest.dist = dist;
                closest.point = Vector3Add(origin, Vector3Scale(dir, dist));
                closest.normal = norm;
                closest.material = walls[i].material;
                if ((i + 1) == mirrorWallIndex) {
                    closest.material.reflectivity = 0.95f;
                    closest.material.color = WHITE;
                }
            }
        }
    }
    return closest;
}

Vector3 CastRay(Vector3 origin, Vector3 dir, int depth) {
    if (depth <= 0) return {0,0,0};

    HitInfo hit = Trace(origin, dir);
    if (!hit.hit) return { 0.0f, 0.0f, 0.0f };

    Vector3 ambient = { 0.15f, 0.15f, 0.15f }; 
    Vector3 totalColor = {0,0,0};

    Vector3 baseColor = { hit.material.color.r / 255.0f, hit.material.color.g / 255.0f, hit.material.color.b / 255.0f };

    for (const auto& light : lights) {
        if (!light.active) continue;

        Vector3 lightDir = Vector3Subtract(light.position, hit.point);
        float distToLight = Vector3Length(lightDir);
        lightDir = Vector3Scale(lightDir, 1.0f / distToLight);

        HitInfo shadowHit = Trace(Vector3Add(hit.point, Vector3Scale(hit.normal, 0.01f)), lightDir);
        float shadowFactor = 1.0f;
        
        if (shadowHit.hit && shadowHit.dist < distToLight) {
             if (shadowHit.material.transparency > 0.0f) shadowFactor = 0.7f;
             else shadowFactor = 0.0f;
        }

        if (shadowFactor > 0.01f) {
            float diff = fmaxf(0.0f, Vector3DotProduct(hit.normal, lightDir));
            Vector3 diffuse = Vector3Scale(baseColor, diff * light.intensity);

            Vector3 viewDir = Vector3Scale(dir, -1.0f);
            Vector3 reflectDir = Vector3Reflect(Vector3Scale(lightDir, -1.0f), hit.normal);
            float spec = powf(fmaxf(0.0f, Vector3DotProduct(viewDir, reflectDir)), 50);
            Vector3 specular = Vector3Scale({1,1,1}, spec * 0.8f);

            Vector3 lightColor = {light.color.r/255.0f, light.color.g/255.0f, light.color.b/255.0f};
            Vector3 combined = Vector3Multiply(Vector3Add(diffuse, specular), lightColor);
            
            totalColor = Vector3Add(totalColor, Vector3Scale(combined, shadowFactor));
        }
    }

    Vector3 finalColor = Vector3Add(Vector3Multiply(baseColor, ambient), totalColor);

    if (hit.material.reflectivity > 0.0f) {
        Vector3 reflectDir = Vector3Reflect(dir, hit.normal);
        Vector3 reflectColor = CastRay(Vector3Add(hit.point, Vector3Scale(hit.normal, 0.01f)), reflectDir, depth - 1);
        finalColor = Vector3Add(Vector3Scale(finalColor, 1.0f - hit.material.reflectivity), Vector3Scale(reflectColor, hit.material.reflectivity));
    }

    if (hit.material.transparency > 0.0f) {
        float ior = hit.material.refractiveIndex;
        float eta = 1.0f / ior;
        Vector3 n = hit.normal;
        float cosI = -Vector3DotProduct(n, dir);
        
        if (cosI < 0) { cosI = -cosI; n = Vector3Scale(n, -1.0f); eta = ior; }
        
        float k = 1.0f - eta * eta * (1.0f - cosI * cosI);
        Vector3 refractColor = {0,0,0};
        
        if (k >= 0.0f) {
            Vector3 refractDir = Vector3Add(Vector3Scale(dir, eta), Vector3Scale(n, eta * cosI - sqrtf(k)));
            refractColor = CastRay(Vector3Add(hit.point, Vector3Scale(refractDir, 0.01f)), refractDir, depth - 1);
        }
        finalColor = Vector3Add(Vector3Scale(finalColor, 1.0f - hit.material.transparency), Vector3Scale(refractColor, hit.material.transparency));
    }

    return finalColor;
}

int main() {
    InitWindow(800, 600, "Improved Cornell Box");
    SetTargetFPS(60);

    Image screenImg = GenImageColor(RENDER_WIDTH, RENDER_HEIGHT, BLACK);
    Texture2D screenTex = LoadTextureFromImage(screenImg);

    float wallThick = 20.0f;
    float roomSize = 6.0f;
    
    // Walls overlapped to prevent leaks
    walls.push_back({{-roomSize-wallThick, -roomSize-wallThick, 10}, {roomSize+wallThick, roomSize+wallThick, 10 + wallThick}, {WHITE, 0, 0, 1}}); // Back
    walls.push_back({{-roomSize-wallThick, -roomSize-wallThick, -11-wallThick}, {roomSize+wallThick, roomSize+wallThick, -11}, {WHITE, 0, 0, 1}}); // Front (Behind camera)
    walls.push_back({{-11-wallThick, -roomSize-wallThick, -15}, {-10, roomSize+wallThick, 15}, {RED, 0, 0, 1}}); // Left
    walls.push_back({{ 10, -roomSize-wallThick, -15}, { 11+wallThick, roomSize+wallThick, 15}, {GREEN, 0, 0, 1}}); // Right
    walls.push_back({{-15, -11-wallThick, -15}, { 15, -10, 15}, {WHITE, 0, 0, 1}}); // Floor
    walls.push_back({{-15, 10, -15}, { 15, 11+wallThick, 15}, {WHITE, 0, 0, 1}}); // Ceiling

    spheres.push_back({{ -2.5f, -3.0f, 2.0f }, 2.0f, { BLUE, 0.0f, 0.0f, 1.5f }});
    spheres.push_back({{ 2.5f, 2.0f, -2.0f }, 1.5f, { YELLOW, 0.0f, 0.0f, 1.5f }});
    boxes.push_back({ { 1.5f, -5.0f, 1.0f }, { 4.5f, -2.0f, 4.0f }, { PURPLE, 0.0f, 0.0f, 1.5f } });
    boxes.push_back({ { -4.0f, 0.0f, -4.0f }, { -2.0f, 2.0f, -2.0f }, { ORANGE, 0.0f, 0.0f, 1.5f } });

    lights.push_back({ { 0, 4.5f, 0 }, WHITE, 1.0f, true });
    lights.push_back({ { 0, 0, 0 }, RAYWHITE, 0.8f, false });

    bool objReflective = false;
    bool objTransparent = false;
    Vector3 camPos = { 0, 0, -9.5f };

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_L)) lights[1].active = !lights[1].active;
        if (IsKeyPressed(KEY_Z)) objReflective = !objReflective;
        if (IsKeyPressed(KEY_X)) objTransparent = !objTransparent;

        if (IsKeyDown(KEY_W)) lights[1].position.y += 0.2f;
        if (IsKeyDown(KEY_S)) lights[1].position.y -= 0.2f;
        if (IsKeyDown(KEY_A)) lights[1].position.x -= 0.2f;
        if (IsKeyDown(KEY_D)) lights[1].position.x += 0.2f;
        if (IsKeyDown(KEY_Q)) lights[1].position.z -= 0.2f;
        if (IsKeyDown(KEY_E)) lights[1].position.z += 0.2f;

        for (int i = 0; i <= 6; i++) {
            if (IsKeyPressed(KEY_ZERO + i)) mirrorWallIndex = i;
        }

        for (auto& s : spheres) {
            s.material.reflectivity = objReflective ? 0.8f : 0.0f;
            s.material.transparency = objTransparent ? 0.8f : 0.0f;
        }
        for (auto& b : boxes) {
            b.material.reflectivity = objReflective ? 0.8f : 0.0f;
            b.material.transparency = objTransparent ? 0.8f : 0.0f;
        }

        Color* pixels = (Color*)screenImg.data;

        // OpenMP acceleration (Enable in compiler settings: -fopenmp)
        #pragma omp parallel for schedule(dynamic)
        for (int y = 0; y < RENDER_HEIGHT; y++) {
            for (int x = 0; x < RENDER_WIDTH; x++) {
                float u = (float)x / RENDER_WIDTH;
                float v = (float)y / RENDER_HEIGHT;
                
                Vector3 screenPoint = { 
                    (u - 0.5f) * 2.66f, 
                    (0.5f - v) * 2.0f, 
                    -8.5f 
                };
                
                Vector3 dir = Vector3Normalize(Vector3Subtract(screenPoint, camPos));
                Vector3 col = CastRay(camPos, dir, MAX_DEPTH);
                
                if (col.x > 1.0f) col.x = 1.0f;
                if (col.y > 1.0f) col.y = 1.0f;
                if (col.z > 1.0f) col.z = 1.0f;
                
                pixels[y * RENDER_WIDTH + x] = { 
                    (unsigned char)(col.x * 255), 
                    (unsigned char)(col.y * 255), 
                    (unsigned char)(col.z * 255), 
                    255 
                };
            }
        }

        UpdateTexture(screenTex, pixels);

        BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(screenTex, 
                {0, 0, (float)RENDER_WIDTH, (float)RENDER_HEIGHT}, 
                {0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()}, 
                {0, 0}, 0.0f, WHITE);
            
            DrawText("Controls: Z (Refl), X (Transp), 0-6 (MirrWall), L (Light2)", 10, 10, 20, BLACK);
            DrawText(TextFormat("Light2: %.1f %.1f %.1f", lights[1].position.x, lights[1].position.y, lights[1].position.z), 10, 35, 20, GRAY);
            DrawFPS(10, 60);
        EndDrawing();
    }

    UnloadTexture(screenTex);
    UnloadImage(screenImg);
    CloseWindow();

    return 0;
}
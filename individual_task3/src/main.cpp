#include "raylib.h"
#include "Airship.h"
#include "Scene.h"

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    
    InitWindow(screenWidth, screenHeight, "Airship Delivery - Shader Edition");
    SetTargetFPS(60);

    Airship player;
    player.Init();

    Scene level;
    level.Init();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        player.Update(dt);

        BeginDrawing();
            ClearBackground(SKYBLUE);

            BeginMode3D(player.camera);
                
                level.Draw();
                player.Draw();
                
                DrawGrid(20, 10.0f);

            EndMode3D();

        
            if (player.isAimingMode) {
                DrawText("MODE: TARGETING", 10, 70, 20, RED);
                DrawText("+", screenWidth/2 - 10, screenHeight/2 - 10, 40, RED); // Прицел
            }

        EndDrawing();
    }
    CloseWindow();

    return 0;
}
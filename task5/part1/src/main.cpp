#include <iostream>
using namespace std;

#include "helper.hpp"
#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

void fillCanvas(Image& canvas, const Lexer& lexer, bool isTree = false) {
    Rectangle borders;
    auto lines = isTree ? lexer.drawTree(borders) : lexer.draw(borders);
    float perUnit = 700.0 / max(borders.height - borders.y, borders.width - borders.x);
    Vector2 displacement = { (700 - (borders.width - borders.x) * perUnit) / 2 - borders.x * perUnit, (700 - (borders.height - borders.y) * perUnit) / 2 - borders.y * perUnit };
    for (auto line : lines) {
        ImageDrawLineEx(&canvas,
            { line.start.x * perUnit + displacement.x, line.start.y * perUnit + displacement.y},
            { line.end.x * perUnit + displacement.x, line.end.y * perUnit + displacement.y},
            line.width,
            line.color);
    }
    ImageRotate(&canvas, 180);
}

int main(int argc, char* argv[]) {
    argv++;
    cout << *argv << "\n";
    Lexer lexer = Lexer(*argv);

    InitWindow(800, 800, "title");
    
    bool isTree = false;
    Image canvas = GenImageColor(700, 700, WHITE);
    Texture texture;
    fillCanvas(canvas, lexer);
    texture = LoadTextureFromImage(canvas);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(LIGHTGRAY);

        if (GuiButton({ .x = 50, .y = 720, .width = 200, .height = 50}, "Prev iteration")) {
            canvas = GenImageColor(700, 700, WHITE);
            lexer.prevIteration();
            fillCanvas(canvas, lexer, isTree);
            UpdateTexture(texture, canvas.data);
        }

        GuiLabel({.x = 300, .y = 720, .width = 100, .height = 50}, TextFormat("Current itreation %d", lexer.getIteration()));

        if (GuiButton({.x = 400, .y = 720, .width = 100, .height = 50}, "Tree")) {
            isTree = !isTree;
            canvas = GenImageColor(700, 700, WHITE);
            fillCanvas(canvas, lexer, isTree);
            UpdateTexture(texture, canvas.data);
        }

        if (GuiButton({ .x = 550, .y = 720, .width = 200, .height = 50}, "Next iteration")) {
            canvas = GenImageColor(700, 700, WHITE);
            lexer.nextIteration();
            fillCanvas(canvas, lexer, isTree);
            UpdateTexture(texture, canvas.data);
        }


        DrawTexture(texture, 50, 0, WHITE);
        EndDrawing();
    }

    UnloadImage(canvas);
    UnloadTexture(texture);
}
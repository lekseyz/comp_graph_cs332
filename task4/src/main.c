#include "stdio.h"

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#pragma region layout
#define WINDOW_WIDTH 600
#define WINDOW_HIGHT 600

#define DRAW_BOX_WIDTH 400
#define DRAW_BOX_HIGHT 600

#define ELEMENTS_X DRAW_BOX_WIDTH + 10
#define ELEMENTS_WIDTH WINDOW_WIDTH - DRAW_BOX_WIDTH - 10 * 2
#define ELEMENTS_HIGHT 50
#define ELEMENTS_PADDING 2
#pragma endregion




int main() {
    InitWindow(WINDOW_WIDTH, WINDOW_HIGHT, "Fill tools");
    SetTargetFPS(60);

    int borderWidth = GuiGetStyle(DEFAULT, BORDER_WIDTH);

    /// Панель с границами
    Rectangle panel = (Rectangle) { .x = 0,
                                    .y = 0,
                                    .width = DRAW_BOX_WIDTH,
                                    .height = DRAW_BOX_HIGHT };

    /// Канвас для рисования внутри панели
    Rectangle canvas = (Rectangle) { .x = borderWidth,
                                     .y = borderWidth,
                                     .width = DRAW_BOX_WIDTH - borderWidth * 2,
                                     .height = DRAW_BOX_HIGHT - borderWidth * 2 };
    



    Image canvasImage;      // Изображение канваса (2d массив для работы с изображением)
    Texture canvasTexture;  // Текструа канваса - то же самое изображение, но уже загруженное на видеокарту

    canvasImage = GenImageColor(canvas.width, canvas.height, WHITE);
    canvasTexture = LoadTextureFromImage(canvasImage);   // загружаем текструры на видекарту

    Vector2 prevMouseInner = {-1, -1};
    Vector2 curMouseInner = {-1, -1};

    while(!WindowShouldClose()) {
        BeginDrawing(); // Начало зоны рисования

        ClearBackground(LIGHTGRAY);
        
        

        Vector2 mousePosition = GetMousePosition();
        if (CheckCollisionPointRec(mousePosition, canvas)) {
            curMouseInner = (Vector2){ mousePosition.x -  borderWidth, mousePosition.y - borderWidth };
            printf("Мышь находится внутри канваса. Знай это сучара");
        }

        GuiPanel(panel, NULL);  // Рисуем панель без названия (чисто границы)
        DrawTexture(canvasTexture, canvas.x, canvas.y, WHITE);  // приказываем видекарте нарисовать нашу текстуру

        EndDrawing();  // Конец зоны рисования
    }
}
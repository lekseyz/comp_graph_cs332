#include "stdio.h"
#include "helper.h"

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

typedef enum States {
    POLYGON_DRAWING,
    POLYGON_MOVMENT,
    POLYGON_CENTER_CHANGING,
    POLYGON_POINT_CHANGING,
    LINE_CREATION,
    INTERSECTION_CHECK,
    SIDE_CHECK
} States;

void DrawPolygon(Image image, Color color, Polygon) {
    
}

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

    States state = POLYGON_DRAWING;
    char* buttonActions[] = {"Create Polygon",
                             "Move Polygon",
                             "Change From Center",
                             "Change From Point",
                             "Create Line",
                             "Check Polygon Intersection",
                             "Check Point Side"};

    while(!WindowShouldClose()) {
        BeginDrawing(); // Начало зоны рисования

        ClearBackground(LIGHTGRAY);
        
        GuiLabel((Rectangle) {ELEMENTS_X, ELEMENTS_PADDING, ELEMENTS_WIDTH, ELEMENTS_HIGHT}, buttonActions[state]);

        if (GuiButton((Rectangle) {ELEMENTS_X, ELEMENTS_PADDING * 2 + ELEMENTS_HIGHT, ELEMENTS_WIDTH, ELEMENTS_HIGHT}, "Clean")) {
            // TODO: clean screen
            printf("some cleaning stuff\n");
        }

        for (int i = 0; i < SIDE_CHECK; i++) {
            if (GuiButton((Rectangle) {ELEMENTS_X, ELEMENTS_PADDING * (i + 3) + ELEMENTS_HIGHT * (i + 2), ELEMENTS_WIDTH, ELEMENTS_HIGHT}, buttonActions[i])) {
                state = (States)i;
            }
        }

        Vector2 mousePosition = GetMousePosition();
        if (CheckCollisionPointRec(mousePosition, canvas)) {
            curMouseInner = (Vector2){ mousePosition.x -  borderWidth, mousePosition.y - borderWidth };
            
            printf("мы сейчас находимся в состоянии %s", buttonActions[state]);
            // TODO: states handling
        }

        GuiPanel(panel, NULL);  // Рисуем панель без названия (чисто границы)
        DrawTexture(canvasTexture, canvas.x, canvas.y, WHITE);  // приказываем видекарте нарисовать нашу текстуру

        EndDrawing();  // Конец зоны рисования
    }
}
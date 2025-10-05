#include "stdio.h"
#include "stdlib.h"
#include "helper.h"
#include "math.h"
#include "inttypes.h"

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

void BrezinheimLow(Image* image, Color color, Line line) {
    int dx = line.end.x - line.start.x;
    int dy = line.end.y - line.start.y;
    int iy = 1;
    if (dy < 0) {
        iy = -1;
        dy = -dy;
    }
    int d = dy * 2 + dx;

    int y = line.start.y;
    for (int x = line.start.x; x <= line.end.x; x++) {
        ImageDrawPixel(image, x, y, color);
        if (d > 0) {
            y += iy;
            d += 2 * (dy - dx);
        }
        else {
            d += 2 * dy;
        }
    }
}

void BrezinheimHight(Image* image, Color color, Line line) {
    int dx = line.end.x - line.start.x;
    int dy = line.end.y - line.start.y;
    int ix = 1;
    if (dx < 0) {
        ix = -1;
        dx = -dx;
    }
    int d = dx * 2 + dy;

    int x = line.start.x;
    for (int y = line.start.y; y <= line.end.y; y++) {
        ImageDrawPixel(image, x, y, color);
        if (d > 0) {
            x += ix;
            d += 2 * (dx - dy);
        }
        else {
            d += 2 * dx;
        }
    }
}

void DrawBrezinheim(Image* image, Color color, Line line) {
    int dx = line.end.x - line.start.x;
    int dy = line.end.y - line.start.y;

    if (abs(dx) >= abs(dy)) {
        if (dx < 0) {
            BrezinheimLow(image, color, (Line) {
                .start = line.end,
                .end = line.start,
            });
        }
        else {
            BrezinheimLow(image, color, line);
        }
    }
    else {
        if (dy < 0) {
            BrezinheimHight(image, color, (Line) {
                .start = line.end,
                .end = line.start,
            });
        }
        else {
            BrezinheimHight(image, color, line);
        }
    }
}

void DrawPolygon(Image* image, Color color, Polygon polygon) {

    int xmin = INT32_MAX;
    int xmax = INT32_MIN;
    int ymin = INT32_MAX;
    int ymax = INT32_MIN;

    PolygonVert* vert = polygon.verts;
    for (int i = 0; i < polygon.size; i++) {
        Line line = (Line) {
            .start = vert->cur,
            .end = vert->next->cur
        };
        if (polygon.size >= 2) {
            DrawBrezinheim(image, BLUE, line);
        }
        xmin = (int)fmin(xmin, vert->cur.x);
        xmax = (int)fmax(xmax, vert->cur.x);
        ymin = (int)fmin(ymin, vert->cur.y);
        ymax = (int)fmax(ymax, vert->cur.y);

        ImageDrawCircle(image, vert->cur.x, vert->cur.y, 3, RED);
        vert = vert->next;
    }

    if (polygon.size < 3) return;

    int* intersections = malloc(sizeof(int) * polygon.size);

    for (int y = ymin; y <= ymax; y++) {
        int size = 0;
        vert = polygon.verts;

        for (int i = 0; i < polygon.size; i++) {
            Vector2 v1 = vert->cur;
            Vector2 v2 = vert->next->cur;

            if (v1.y == v2.y) {
                vert = vert->next;
                continue;
            }

            if ((y >= fmin(v1.y, v2.y)) && (y < fmax(v1.y, v2.y))) {
                float x = v1.x + (y - v1.y) * (v2.x - v1.x) / (v2.y - v1.y);
                intersections[size++] = (int)x;
            }

            vert = vert->next;
        }

        qsort(intersections, size, sizeof(int), compare_function);

        for (int j = 0; j + 1 < size; j += 2) {
            int x1 = intersections[j];
            int x2 = intersections[j + 1];
            for (int x = x1; x <= x2; x++) {
                ImageDrawPixel(image, x, y, color);
            }
        }
    }

    free(intersections);
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

    Polygon* polygons = NULL;
    Polygon* curPolygon = NULL;
    size_t polygonsCount = 0;

    Line* lines = NULL;
    size_t linesCount = 0;
    bool isFirstPoint = true;

    States state = 7;
    char* buttonActions[] = {"Create Polygon",
                             "Move Polygon",
                             "Change From Center",
                             "Change From Point",
                             "Create Line",
                             "Check Polygon Intersection",
                             "Check Point Side",
                             "Doing Nothing"};

    while(!WindowShouldClose()) {
        BeginDrawing(); // Начало зоны рисования

        ClearBackground(LIGHTGRAY);
        
        GuiLabel((Rectangle) {ELEMENTS_X, ELEMENTS_PADDING, ELEMENTS_WIDTH, ELEMENTS_HIGHT}, buttonActions[state]);

        if (GuiButton((Rectangle) {ELEMENTS_X, ELEMENTS_PADDING * 2 + ELEMENTS_HIGHT, ELEMENTS_WIDTH, ELEMENTS_HIGHT}, "Clean")) {
            canvasImage = GenImageColor(canvas.width, canvas.height, WHITE);
            UpdateTexture(canvasTexture, canvasImage.data);
        }

        for (int i = 0; i < SIDE_CHECK; i++) {
            if (GuiButton((Rectangle) {ELEMENTS_X, ELEMENTS_PADDING * (i + 3) + ELEMENTS_HIGHT * (i + 2), ELEMENTS_WIDTH, ELEMENTS_HIGHT}, buttonActions[i])) {
                state = (States)i;

                switch (state) {
                    case POLYGON_DRAWING:
                        if (polygonsCount == 0) {
                            polygons = (Polygon*)malloc(sizeof(Polygon));
                            polygonsCount++;
                        }
                        else {
                            polygonsCount++;
                            polygons = (Polygon*)realloc(polygons, polygonsCount * sizeof(Polygon));
                        }
                        polygons[polygonsCount - 1] = CreatePolygon(0);
                        curPolygon = polygons + (polygonsCount - 1);
                        break;
                    case POLYGON_MOVMENT:

                        break;
                    case POLYGON_CENTER_CHANGING:

                        break;
                    case POLYGON_POINT_CHANGING:

                        break;
                    case INTERSECTION_CHECK:

                        break;
                    case SIDE_CHECK:

                        break;

                    default:
                        break;

                }
            }
        }

        Vector2 mousePosition = GetMousePosition();
        if (CheckCollisionPointRec(mousePosition, canvas)) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                curMouseInner = (Vector2){ mousePosition.x -  borderWidth, mousePosition.y - borderWidth };
                
                switch (state)
                {
                case POLYGON_DRAWING:
                    AddPolygonPoint(curPolygon, curMouseInner);
                    DrawPolygon(&canvasImage, BLACK, *curPolygon);
                    UpdateTexture(canvasTexture, canvasImage.data);
                    break;
                case LINE_CREATION: 
                    if (isFirstPoint) {
                        Line *tmp = (Line*)realloc(lines, (linesCount + 1) * sizeof(Line));
                        if (!tmp) { perror("realloc"); break; }
                        lines = tmp;
                        ImageDrawCircle(&canvasImage, curMouseInner.x, curMouseInner.y, 3, GREEN);
                        UpdateTexture(canvasTexture, canvasImage.data);
                        lines[linesCount].start = curMouseInner;
                        lines[linesCount].end = curMouseInner;
                        isFirstPoint = false;
                    } else {
                        lines[linesCount].end = curMouseInner;
                        ImageDrawCircle(&canvasImage, curMouseInner.x, curMouseInner.y, 3, GREEN);
                        DrawBrezinheim(&canvasImage, BLACK, lines[linesCount]);
                        UpdateTexture(canvasTexture, canvasImage.data);
                        linesCount++;
                        isFirstPoint = true;
                    }
                    break;
                default:
                    printf("мы сейчас находимся в состоянии %s", buttonActions[state]);
                    break;
                }

                prevMouseInner = curMouseInner;
            }
            
        }

        GuiPanel(panel, NULL);  // Рисуем панель без названия (чисто границы)
        DrawTexture(canvasTexture, canvas.x, canvas.y, WHITE);  // приказываем видекарте нарисовать нашу текстуру

        EndDrawing();  // Конец зоны рисования
    }
}
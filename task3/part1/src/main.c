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

typedef void (*DrawAction)();

typedef enum DrawModes {
    PEN,
    FILL,
    IMAGE_FILL,
    BORDER_FILL
} DrawModes;

typedef struct {
    DrawAction click;
    DrawAction slide;
} ActionPair;

Image canvasImage;
Image* fill;
Image imageFill;
Image paintFill;

typedef struct {
  Vector2 *array;
  size_t used;
  size_t size;
} Array;

void initArray(Array *a, size_t initialSize) {
  a->array = malloc(initialSize * sizeof(Vector2));
  a->used = 0;
  a->size = initialSize;
}

void insertArray(Array *a, Vector2 element) {
  // a->used is the number of used entries, because a->array[a->used++] updates a->used only *after* the array has been accessed.
  // Therefore a->used can go up to a->size 
  if (a->used == a->size) {
    a->size *= 2;
    a->array = realloc(a->array, a->size * sizeof(Vector2));
  }
  a->array[a->used++] = element;
}

void freeArray(Array *a) {
  free(a->array);
  a->array = NULL;
  a->used = a->size = 0;
}


bool CmpColors(Color a, Color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

int modulo(int a, int b) {
    return (b + (a % b)) % b;
}


void Paint(Vector2* const prevPos, const Vector2* const curPos) {
    ImageDrawLineEx(&canvasImage, *prevPos, *curPos, 3, BLACK);
}

void Fill(Vector2 coord, bool line, Color def, Vector2 offset) {
    if (line) {
        int l, r;
        l = coord.x - 1;
        r = coord.x + 1;

        while (l >= 0 && l < canvasImage.width && CmpColors(def, GetImageColor(canvasImage, l, coord.y))) {
            l--;
        }

        while (r >= 0 && r < canvasImage.width && CmpColors(def, GetImageColor(canvasImage, r, coord.y))) {
            r++;
        }

        if (r - l == 2)
            return;

        for (int i = l + 1; i < r; i++) {
            int dx = coord.x - i + offset.x;
            Color color = GetImageColor(*fill, modulo(dx, fill->width), fill->height - offset.y - 1);
            ImageDrawPixel(&canvasImage, i, coord.y, color);
        }

        for (int i = l + 1; i < r; i++) {
            int dx = coord.x - i + offset.x;
            Fill((Vector2) {i, coord.y}, !line, def, (Vector2) {modulo(dx, fill->width), offset.y});
        }
    }
    else {
        int l, r;
        l = coord.y - 1;
        r = coord.y + 1;

        while (l >= 0 && l < canvasImage.height && CmpColors(def, GetImageColor(canvasImage, coord.x, l))) {
            l--;
        }

        while (r >= 0 && r < canvasImage.height && CmpColors(def, GetImageColor(canvasImage, coord.x, r))) {
            r++;
        }

        if (r - l == 2)
            return;

        for (int i = l + 1; i < r; i++) {
            int dx = coord.y - i + offset.y;
            Color color = GetImageColor(*fill, offset.x, fill->height - modulo(dx, fill->height) - 1);
            ImageDrawPixel(&canvasImage, coord.x, i, color);
        }

        for (int i = l + 1; i < r; i++) {
            int dy = coord.y - i + offset.y;
            Fill((Vector2) {coord.x, i}, !line, def, (Vector2) {offset.x, modulo(dy, fill->height)});
        }
    }
    
}

void FillPaint(Vector2* const prevPos, const Vector2* const curPos) {
    fill = &paintFill;
    Color color = GetImageColor(canvasImage, curPos->x, curPos->y);
    Fill(*curPos, true, color, (Vector2) {0, 0});
}

void FillImage(Vector2* const prevPos, const Vector2* const curPos) {
    fill = &imageFill;
    Color color = GetImageColor(canvasImage, curPos->x, curPos->y);
    Fill(*curPos, true, color, (Vector2) {0, 0});
}

static inline bool InBoundsI(int x, int y) {
    return x >= 0 && x < canvasImage.width && y >= 0 && y < canvasImage.height;
}

static bool GetFirstBorderPointC(const Image *img, Color inner, int sx, int sy,
                                 int *outX, int *outY)
{
    int w = img->width;
    for (int x = sx + 1; x < w; x++) {
        Color c = GetImageColor(*img, x, sy);
        if (!CmpColors(c, inner)) { *outX = x; *outY = sy; return true; }
    }
    return false;
}

void DrawBorder(Vector2* const prevPos, const Vector2* const curPos)
{
    Array arr;
    initArray(&arr, 10);

                            
    static const int DX[8] = { 0,-1,-1,-1, 0, 1, 1, 1 };
    static const int DY[8] = { 1, 1, 0,-1,-1,-1, 0, 1 };

    #define RIGHT_X(dir) (-DY[(dir)])
    #define RIGHT_Y(dir) ( DX[(dir)])

    int w = canvasImage.width;
    int h = canvasImage.height;

    int sx = (int)curPos->x;
    int sy = (int)curPos->y;
    if (!InBoundsI(sx, sy)) return;

    Color inside = GetImageColor(canvasImage, sx, sy);

    int bx, by;
    if (!GetFirstBorderPointC(&canvasImage, inside, sx, sy, &bx, &by)) return;

    Color border = GetImageColor(canvasImage, bx, by);

    unsigned char *visited = (unsigned char*)calloc((size_t)w * h, 1);
    if (!visited) return;

    int x = bx, y = by;
    int startX = bx, startY = by;
    int safety = w * h * 4;
    int dir = 0;

    do {
        bool moved = false;

        for (int i = 0; i < 8; ++i) {
            int nx = x + DX[dir];
            int ny = y + DY[dir];

            if (InBoundsI(nx, ny) &&
                !visited[ny * w + nx]) {

                if ((dir % 2) == 0) {
                    int rx = RIGHT_X(dir), ry = RIGHT_Y(dir);
                    int ix = nx + rx, iy = ny + ry;
                    if (InBoundsI(ix, iy)) {
                        Color innerAt = GetImageColor(canvasImage, ix, iy);
                        Color cand    = GetImageColor(canvasImage, nx, ny);

                        if (CmpColors(cand, border) && CmpColors(innerAt, inside)) {
                            x = nx; y = ny;
                            visited[y * w + x] = 1;
                            insertArray(&arr, (Vector2) {ix, iy});
                            moved = true;
                            dir = (dir + 2) % 8;
                            break;
                        }
                    }
                } else {
                    int dirNext = (dir + 1) % 8;
                    int dirPrev = dir == 0 ? 7 : dir - 1;

                    int ix1 = nx + RIGHT_X(dirNext), iy1 = ny + RIGHT_Y(dirNext);
                    int ix2 = nx + RIGHT_X(dirPrev), iy2 = ny + RIGHT_Y(dirPrev);

                    bool isInner1 = InBoundsI(ix1, iy1) &&
                                    CmpColors(GetImageColor(canvasImage, ix1, iy1), inside);
                    bool isInner2 = InBoundsI(ix1, iy2) &&
                                    CmpColors(GetImageColor(canvasImage, ix2, iy2), inside);

                    Color cand = GetImageColor(canvasImage, nx, ny);

                    if (CmpColors(cand, border) && (isInner1 || isInner2)) {
                        x = nx; y = ny;
                        visited[y * w + x] = 1;
                        if (isInner1) insertArray(&arr, (Vector2){ix1, iy1});
                        if (isInner2) insertArray(&arr, (Vector2){ix2, iy2});
                        moved = true;
                        dir = (dir + 2) % 8;
                        break;
                    }
                }
                dir = dir == 0 ? 7 : dir - 1;
            }
        }
        if (!moved) break;
        if (--safety <= 0) break;
    } while (!(x == startX && y == startY));

    free(visited);

    for (int i = 0; i < arr.size; i++) {
        ImageDrawPixel(&canvasImage, arr.array[i].x, arr.array[i].y, BLUE);
    }

    freeArray(&arr);
}


void Empty(Vector2* const prevPos, const Vector2* const curPos) {

}


bool HandleMouse(Vector2* const prevPos, const Vector2* const curPos, const ActionPair* const pair) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        prevPos->x = curPos->x;
        prevPos->y = curPos->y;
        pair->click(prevPos, curPos);
        return true;
    }
    else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if (prevPos->x > 0) {
            pair->slide(prevPos, curPos);
            prevPos->x = curPos->x;
            prevPos->y = curPos->y;
        }

        return true;
    }
    else {
        prevPos->x = -1;
        prevPos->y = -1;

        return false;
    }
}

int main() {
    InitWindow(WINDOW_WIDTH, WINDOW_HIGHT, "Fill tools");
    SetTargetFPS(60);

    int borderWidth = GuiGetStyle(DEFAULT, BORDER_WIDTH);

    Rectangle panel = (Rectangle) { .x = 0,
                                    .y = 0,
                                    .width = DRAW_BOX_WIDTH,
                                    .height = DRAW_BOX_HIGHT };

    Rectangle canvas = (Rectangle) { .x = borderWidth,
                                     .y = borderWidth,
                                     .width = DRAW_BOX_WIDTH - borderWidth * 2,
                                     .height = DRAW_BOX_HIGHT - borderWidth * 2 };
    
    char* labels[] = { "#23#pen mode", 
                       "#26#fill mode",
                       "#12#image mode",
                       "#80#border mode", };

    ActionPair pairs[] = { (ActionPair) {Empty, Paint},
                           (ActionPair) {FillPaint, Empty},
                           (ActionPair) {FillImage, Empty},
                           (ActionPair) {DrawBorder, Empty}, };

    DrawModes mode = PEN; 

    Texture canvasTexture;
    fill = NULL;
    imageFill = LoadImage("./images/small.png");
    paintFill = GenImageColor(1, 1, BLUE);

    canvasImage = GenImageColor(canvas.width, canvas.height, WHITE);
    canvasTexture = LoadTextureFromImage(canvasImage);

    Vector2 prevMouseInner = {-1, -1};
    Vector2 curMouseInner = {-1, -1};

    while(!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(LIGHTGRAY);
        
        GuiLabel((Rectangle){ELEMENTS_X, ELEMENTS_PADDING, ELEMENTS_WIDTH, ELEMENTS_HIGHT}, labels[mode]);
        if (GuiButton((Rectangle){ELEMENTS_X, ELEMENTS_PADDING * 2 + ELEMENTS_HIGHT * 1, ELEMENTS_WIDTH, ELEMENTS_HIGHT}, "Clean")) {
            canvasImage = GenImageColor(canvas.width, canvas.height, WHITE);
            UpdateTexture(canvasTexture, canvasImage.data);
        }
        for (int i = 0; i <= BORDER_FILL; i++) {
            if (GuiButton((Rectangle){ELEMENTS_X, ELEMENTS_PADDING * (i + 3) + ELEMENTS_HIGHT * (i + 2), ELEMENTS_WIDTH, ELEMENTS_HIGHT}, labels[i])){
                mode = (DrawModes)i;
            }
        }

        Vector2 mousePosition = GetMousePosition();
        if (CheckCollisionPointRec(mousePosition, canvas)) {
            curMouseInner = (Vector2){ mousePosition.x -  borderWidth, mousePosition.y - borderWidth };
            if (HandleMouse(&prevMouseInner, &curMouseInner, pairs + mode)) {
                UpdateTexture(canvasTexture, canvasImage.data);
            }
        }

        GuiPanel(panel, NULL);
        DrawTexture(canvasTexture, canvas.x, canvas.y, WHITE);
        EndDrawing();
    }
}
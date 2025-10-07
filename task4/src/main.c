#include "helper.h"
#include "inttypes.h"
#include "math.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

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
  POLYGON_INTERSECTION_CHECK,
  POLYGON_MOVMENT,
  POLYGON_CENTER_CHANGING,
  POLYGON_POINT_CHANGING,
  LINE_CREATION,
  INTERSECTION_CHECK,
  SIDE_CHECK
} States;

//
// ---------------- Вспомогательные функции ----------------
//

// вращение точки вокруг центра
Vector2 RotatePoint(Vector2 point, Vector2 center, float angle) {
  float s = sinf(angle);
  float c = cosf(angle);
  point.x -= center.x;
  point.y -= center.y;
  float xnew = point.x * c - point.y * s;
  float ynew = point.x * s + point.y * c;
  point.x = xnew + center.x;
  point.y = ynew + center.y;
  return point;
}

// масштабирование точки относительно центра
Vector2 ScalePoint(Vector2 point, Vector2 center, float scale) {
  point.x = center.x + (point.x - center.x) * scale;
  point.y = center.y + (point.y - center.y) * scale;
  return point;
}

// центр всех полигонов (среднеарифметическое всех вершин)
Vector2 GetAllPolygonsCenter(Polygon *polygons, size_t count) {
  Vector2 sum = {0, 0};
  int total = 0;
  for (size_t i = 0; i < count; i++) {
    PolygonVert *vert = polygons[i].verts;
    if (!vert)
      continue;
    for (int j = 0; j < polygons[i].size; j++) {
      sum.x += vert->cur.x;
      sum.y += vert->cur.y;
      total++;
      vert = vert->next;
    }
  }
  if (total == 0)
    return (Vector2){0, 0};
  sum.x /= total;
  sum.y /= total;
  return sum;
}

// применить поворот и масштаб ко всем полигонам
void TransformAllPolygons(Polygon *polygons, size_t count, Vector2 center,
                          float angle, float scale) {
  for (size_t i = 0; i < count; i++) {
    PolygonVert *vert = polygons[i].verts;
    if (!vert)
      continue;
    for (int j = 0; j < polygons[i].size; j++) {
      vert->cur = RotatePoint(vert->cur, center, angle);
      vert->cur = ScalePoint(vert->cur, center, scale);
      vert = vert->next;
    }
  }
}

//
// ---------------- Алгоритмы Брезенхема и отрисовка ----------------
//

void BrezinheimLow(Image *image, Color color, Line line) {
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
    } else {
      d += 2 * dy;
    }
  }
}

void BrezinheimHight(Image *image, Color color, Line line) {
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
    } else {
      d += 2 * dx;
    }
  }
}

void DrawBrezinheim(Image *image, Color color, Line line) {
  int dx = line.end.x - line.start.x;
  int dy = line.end.y - line.start.y;

  if (abs(dx) >= abs(dy)) {
    if (dx < 0)
      BrezinheimLow(image, color, (Line){.start = line.end, .end = line.start});
    else
      BrezinheimLow(image, color, line);
  } else {
    if (dy < 0)
      BrezinheimHight(image, color,
                      (Line){.start = line.end, .end = line.start});
    else
      BrezinheimHight(image, color, line);
  }
}

void DrawPolygon(Image *image, Color color, Polygon polygon) {
  int xmin = INT32_MAX, xmax = INT32_MIN, ymin = INT32_MAX, ymax = INT32_MIN;

  PolygonVert *vert = polygon.verts;
  for (int i = 0; i < polygon.size; i++) {
    Line line = (Line){.start = vert->cur, .end = vert->next->cur};
    if (polygon.size >= 2)
      DrawBrezinheim(image, BLUE, line);
    xmin = (int)fmin(xmin, vert->cur.x);
    xmax = (int)fmax(xmax, vert->cur.x);
    ymin = (int)fmin(ymin, vert->cur.y);
    ymax = (int)fmax(ymax, vert->cur.y);
    ImageDrawCircle(image, vert->cur.x, vert->cur.y, 3, RED);
    vert = vert->next;
  }

  if (polygon.size < 3)
    return;

  int *intersections = malloc(sizeof(int) * polygon.size);

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
    for (int j = 0; j + 1 < size; j += 2)
      for (int x = intersections[j]; x <= intersections[j + 1]; x++)
        ImageDrawPixel(image, x, y, color);
  }

  free(intersections);
}

//
// ---------------- main ----------------
//

int main() {
  InitWindow(WINDOW_WIDTH, WINDOW_HIGHT, "Fill tools");
  SetTargetFPS(60);

  int borderWidth = GuiGetStyle(DEFAULT, BORDER_WIDTH);
  Rectangle panel = {0, 0, DRAW_BOX_WIDTH, DRAW_BOX_HIGHT};
  Rectangle canvas = {borderWidth, borderWidth,
                      DRAW_BOX_WIDTH - borderWidth * 2,
                      DRAW_BOX_HIGHT - borderWidth * 2};

  Image canvasImage = GenImageColor(canvas.width, canvas.height, WHITE);
  Texture canvasTexture = LoadTextureFromImage(canvasImage);

  Vector2 prevMouseInner = {-1, -1};
  Vector2 curMouseInner = {-1, -1};

  Polygon *polygons = NULL;
  Polygon *curPolygon = NULL;
  size_t polygonsCount = 0;

  States state = POLYGON_DRAWING;
  char *buttonActions[] = {"Create Polygon",     "Check Polygon Intersection",
                           "Move Polygon",       "Change Center",
                           "Change Point",       "Create Line",
                           "Check Intersection", "Check Point Side",
                           "Doing Nothing"};

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(LIGHTGRAY);

    GuiLabel((Rectangle){ELEMENTS_X, ELEMENTS_PADDING, ELEMENTS_WIDTH,
                         ELEMENTS_HIGHT},
             buttonActions[state]);

    // Clean
    if (GuiButton((Rectangle){ELEMENTS_X, ELEMENTS_PADDING * 2 + ELEMENTS_HIGHT,
                              ELEMENTS_WIDTH, ELEMENTS_HIGHT},
                  "Clean")) {
      canvasImage = GenImageColor(canvas.width, canvas.height, WHITE);
      UpdateTexture(canvasTexture, canvasImage.data);
      if (polygons) {
        for (size_t i = 0; i < polygonsCount; i++)
          FreePolygon(&polygons[i]); // ✅ исправлено
        free(polygons);
        polygons = NULL;
        polygonsCount = 0;
      }
    }

    // Buttons
    for (int i = 0; i <= SIDE_CHECK; i++) {
      if (GuiButton(
              (Rectangle){ELEMENTS_X,
                          ELEMENTS_PADDING * (i + 3) + ELEMENTS_HIGHT * (i + 2),
                          ELEMENTS_WIDTH, ELEMENTS_HIGHT},
              buttonActions[i])) {
        state = (States)i;
        if (state == POLYGON_DRAWING) {
          polygonsCount++;
          polygons = realloc(polygons, polygonsCount * sizeof(Polygon));
          polygons[polygonsCount - 1] = CreatePolygon(0);
          curPolygon = polygons + (polygonsCount - 1);
        }
      }
    }

    Vector2 mousePosition = GetMousePosition();
    if (CheckCollisionPointRec(mousePosition, canvas)) {
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        curMouseInner = (Vector2){mousePosition.x - borderWidth,
                                  mousePosition.y - borderWidth};
        if (state == POLYGON_DRAWING && curPolygon) {
          AddPolygonPoint(curPolygon, curMouseInner);
          DrawPolygon(&canvasImage, BLACK, *curPolygon);
          UpdateTexture(canvasTexture, canvasImage.data);
        }
        prevMouseInner = curMouseInner;
      }

      // Аффинные преобразования
      if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        curMouseInner = (Vector2){mousePosition.x - borderWidth,
                                  mousePosition.y - borderWidth};

        switch (state) {
        case POLYGON_CENTER_CHANGING: {
          if (polygonsCount == 0)
            break;
          Vector2 center = GetAllPolygonsCenter(polygons, polygonsCount);
          float dx = curMouseInner.x - prevMouseInner.x;
          float dy = curMouseInner.y - prevMouseInner.y;
          float angle = dx * 0.01f;
          float scale = 1.0f + dy * 0.005f;
          TransformAllPolygons(polygons, polygonsCount, center, angle, scale);
          canvasImage = GenImageColor(canvas.width, canvas.height, WHITE);
          for (size_t i = 0; i < polygonsCount; i++)
            DrawPolygon(&canvasImage, BLACK, polygons[i]);
          UpdateTexture(canvasTexture, canvasImage.data);
        } break;

        case POLYGON_POINT_CHANGING: {
          if (polygonsCount == 0)
            break;
          static Vector2 pivot = {-1, -1};
          if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
            pivot = curMouseInner;
          float dx = curMouseInner.x - prevMouseInner.x;
          float dy = curMouseInner.y - prevMouseInner.y;
          float angle = dx * 0.01f;
          float scale = 1.0f + dy * 0.005f;
          TransformAllPolygons(polygons, polygonsCount, pivot, angle, scale);
          canvasImage = GenImageColor(canvas.width, canvas.height, WHITE);
          for (size_t i = 0; i < polygonsCount; i++)
            DrawPolygon(&canvasImage, BLACK, polygons[i]);
          UpdateTexture(canvasTexture, canvasImage.data);
        } break;

        default:
          break;
        }
        prevMouseInner = curMouseInner;
      }
    }

    GuiPanel(panel, NULL);
    DrawTexture(canvasTexture, canvas.x, canvas.y, WHITE);
    EndDrawing();
  }

  UnloadTexture(canvasTexture);
  UnloadImage(canvasImage);

  if (polygons) {
    for (size_t i = 0; i < polygonsCount; i++)
      FreePolygon(&polygons[i]);
    free(polygons);
  }

  CloseWindow();
  return 0;
}

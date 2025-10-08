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


Vector2 ScalePoint(Vector2 point, Vector2 center, float scale) {
  point.x = center.x + (point.x - center.x) * scale;
  point.y = center.y + (point.y - center.y) * scale;
  return point;
}


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
    if (dx < 0) {
      BrezinheimLow(image, color,
                    (Line){
                        .start = line.end,
                        .end = line.start,
                    });
    } else {
      BrezinheimLow(image, color, line);
    }
  } else {
    if (dy < 0) {
      BrezinheimHight(image, color,
                      (Line){
                          .start = line.end,
                          .end = line.start,
                      });
    } else {
      BrezinheimHight(image, color, line);
    }
  }
}

void DrawPolygon(Image *image, Color color, Polygon polygon) {
  int xmin = INT32_MAX;
  int xmax = INT32_MIN;
  int ymin = INT32_MAX;
  int ymax = INT32_MIN;

  PolygonVert *vert = polygon.verts;
  for (int i = 0; i < polygon.size; i++) {
    Line line = (Line){.start = vert->cur, .end = vert->next->cur};
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

Vector2 ScalePoint(Vector2 point, Vector2 center, float scale) {
  point.x = center.x + (point.x - center.x) * scale;
  point.y = center.y + (point.y - center.y) * scale;
  return point;
}

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
int main() {
  InitWindow(WINDOW_WIDTH, WINDOW_HIGHT, "Fill tools");
  SetTargetFPS(60);

  int borderWidth = GuiGetStyle(DEFAULT, BORDER_WIDTH);

  Rectangle panel = (Rectangle){
      .x = 0, .y = 0, .width = DRAW_BOX_WIDTH, .height = DRAW_BOX_HIGHT};

  Rectangle canvas = (Rectangle){.x = borderWidth,
                                 .y = borderWidth,
                                 .width = DRAW_BOX_WIDTH - borderWidth * 2,
                                 .height = DRAW_BOX_HIGHT - borderWidth * 2};

  Image canvasImage;
  Texture canvasTexture;

  canvasImage = GenImageColor(canvas.width, canvas.height, WHITE);
  canvasTexture = LoadTextureFromImage(canvasImage);

  Vector2 prevMouseInner = {-1, -1};
  Vector2 curMouseInner = {-1, -1};

  Polygon *polygons = NULL;
  Polygon *curPolygon = NULL;
  size_t polygonsCount = 0;

  Line *lines = NULL;
  size_t linesCount = 0;
  bool isFirstPoint = true;

  States state = 7;
  char *buttonActions[] = {"Create Polygon",     "Check Polygon Intersection",
                           "Move Polygon",       "Change Center",
                           "Change Point",       "Create Line",
                           "Check Intersection", "Check Point Side",
                           "Doing Nothing"};

  bool showMessageBox = false;
  char messageBoxText[1024] = {0};
  int messageBoxResult = 0;

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(LIGHTGRAY);

    GuiLabel((Rectangle){ELEMENTS_X, ELEMENTS_PADDING, ELEMENTS_WIDTH,
                         ELEMENTS_HIGHT},
             buttonActions[state]);

    // Clean button
    if (GuiButton((Rectangle){ELEMENTS_X, ELEMENTS_PADDING * 2 + ELEMENTS_HIGHT,
                              ELEMENTS_WIDTH, ELEMENTS_HIGHT},
                  "Clean")) {
      canvasImage = GenImageColor(canvas.width, canvas.height, WHITE);
      UpdateTexture(canvasTexture, canvasImage.data);
      printf("Ceanitg polygons. Poligons to be deallocated: %d\n",
             polygonsCount);
      if (polygons) {
        for (size_t i = 0; i < polygonsCount; i++)
          FreePolygon(polygons[i]);
        free(polygons);
        polygons = NULL;
      }
      printf("Ceaning lines. Lines to be deallocated: %d\n", linesCount);
      if (lines) {
        free(lines);
        linesCount = 0;
        lines = NULL;
      }
      printf("Everything is clean. Poligons to be deallocated: %d\n",
             linesCount);
    }

    for (int i = 0; i <= SIDE_CHECK; i++) {
      if (GuiButton(
              (Rectangle){ELEMENTS_X,
                          ELEMENTS_PADDING * (i + 3) + ELEMENTS_HIGHT * (i + 2),
                          ELEMENTS_WIDTH, ELEMENTS_HIGHT},
              buttonActions[i])) {
        state = (States)i;

        switch (state) {
        case POLYGON_DRAWING:
          if (polygonsCount == 0) {
            polygons = (Polygon *)malloc(sizeof(Polygon));
            polygonsCount++;
          } else {
            polygonsCount++;
            polygons =
                (Polygon *)realloc(polygons, polygonsCount * sizeof(Polygon));
          }
          polygons[polygonsCount - 1] = CreatePolygon(0);
          curPolygon = polygons + (polygonsCount - 1);

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
    if (CheckCollisionPointRec(mousePosition, canvas) && !showMessageBox) {
      curMouseInner = (Vector2){mousePosition.x - borderWidth,
                                mousePosition.y - borderWidth};

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        switch (state) {
        case POLYGON_DRAWING:
          if (curPolygon != NULL) {
            AddPolygonPoint(curPolygon, curMouseInner);
            DrawPolygon(&canvasImage, BLACK, *curPolygon);
            UpdateTexture(canvasTexture, canvasImage.data);
          }
          break;

        case LINE_CREATION:
          if (isFirstPoint) {
            Line *tmp = (Line *)realloc(lines, (linesCount + 1) * sizeof(Line));
            if (!tmp) {
              snprintf(messageBoxText, sizeof(messageBoxText), "Memory error");
              showMessageBox = true;
              break;
            }
            lines = tmp;
            lines[linesCount].start = curMouseInner;
            lines[linesCount].end = curMouseInner;

            ImageDrawCircle(&canvasImage, curMouseInner.x, curMouseInner.y, 3,
                            GREEN);
            UpdateTexture(canvasTexture, canvasImage.data);

            isFirstPoint = false;
          } else {
            lines[linesCount].end = curMouseInner;

            ImageDrawCircle(&canvasImage, curMouseInner.x, curMouseInner.y, 3,
                            GREEN);
            DrawBrezinheim(&canvasImage, BLACK, lines[linesCount]);

            for (size_t i = 0; i < linesCount; i++) {
              Vector2 intersection;
              if (GetLineIntersection(lines[i], lines[linesCount],
                                      &intersection)) {
                ImageDrawCircle(&canvasImage, (int)intersection.x,
                                (int)intersection.y, 5, RED);
              }
            }

            linesCount++;
            isFirstPoint = true;
            UpdateTexture(canvasTexture, canvasImage.data);
          }
          break;
        case INTERSECTION_CHECK:
          if (linesCount >= 2) {
            int intersectionCount = 0;
            for (size_t i = 0; i < linesCount; i++) {
              for (size_t j = i + 1; j < linesCount; j++) {
                Vector2 intersection;
                if (GetLineIntersection(lines[i], lines[j], &intersection)) {
                  ImageDrawCircle(&canvasImage, (int)intersection.x,
                                  (int)intersection.y, 5, RED);
                  intersectionCount++;
                }
              }
            }
            UpdateTexture(canvasTexture, canvasImage.data);
            snprintf(messageBoxText, sizeof(messageBoxText),
                     "Intersections found: %d", intersectionCount);
            showMessageBox = true;
          } else {
            snprintf(messageBoxText, sizeof(messageBoxText),
                     "Not enough lines. Created: %zu", linesCount);
            showMessageBox = true;
          }
          break;

        case SIDE_CHECK:
          if (linesCount > 0) {
            strcpy(messageBoxText, "Point position:\n");
            for (size_t i = 0; i < linesCount; i++) {
              int side = ClassifyPointToLine(curMouseInner, lines[i]);
              char lineInfo[128];
              snprintf(lineInfo, sizeof(lineInfo), "Line %zu: %s\n", i + 1,
                       side == 1 ? "LEFT" : (side == -1 ? "RIGHT" : "ON LINE"));
              strcat(messageBoxText, lineInfo);
            }
            showMessageBox = true;
          } else {
            snprintf(messageBoxText, sizeof(messageBoxText),
                     "No lines created");
            showMessageBox = true;
          }
          break;
        case POLYGON_INTERSECTION_CHECK:
          if (polygons && polygonsCount > 0) {
            bool foundPolygon = false;
            for (size_t i = 0; i < polygonsCount; i++) {
              if (polygons[i].verts &&
                  IsPointInsidePolygon(curMouseInner, polygons[i])) {
                snprintf(messageBoxText, sizeof(messageBoxText),
                         "Point is INSIDE polygon #%zu", i + 1);
                foundPolygon = true;
                showMessageBox = true;
                break;
              }
            }
            if (!foundPolygon) {
              snprintf(messageBoxText, sizeof(messageBoxText),
                       "Point is NOT inside any polygon");
              showMessageBox = true;
            }
          } else {
            snprintf(messageBoxText, sizeof(messageBoxText),
                     "No polygons created");
            showMessageBox = true;
          }
          break;
        default:
          break;
        }

        prevMouseInner = curMouseInner;
      }

      
      if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        switch (state) {
        case POLYGON_MOVMENT:
        case POLYGON_CENTER_CHANGING:
          if (polygonsCount > 0) {
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
          }
          break;

        case POLYGON_POINT_CHANGING:
          if (polygonsCount > 0) {
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
          }
          break;

        default:
          break;
        }

        prevMouseInner = curMouseInner;
      }
    }

    GuiPanel(panel, NULL);
    DrawTexture(canvasTexture, canvas.x, canvas.y, WHITE);

    if (showMessageBox) {
      messageBoxResult = GuiMessageBox(
          (Rectangle){WINDOW_WIDTH / 2 - 200, WINDOW_HIGHT / 2 - 100, 400, 200},
          "Result", messageBoxText, "OK");
      if (messageBoxResult >= 0) {
        showMessageBox = false;
      }
    }

    EndDrawing();
  }

  UnloadTexture(canvasTexture);
  UnloadImage(canvasImage);
  if (lines)
    free(lines);
  if (polygons) {
    for (size_t i = 0; i < polygonsCount; i++)
      FreePolygon(polygons[i]);
    free(polygons);
  }

  CloseWindow();
  return 0;
}

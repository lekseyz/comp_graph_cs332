void Plot(int x, int y, float c) {
  if (c <= 0)
    return;
  if (c > 1)
    c = 1;
  unsigned char intensity = (unsigned char)(c * 255);
  DrawPixel(x, y, (Color){0, 0, 0, intensity});
}

void MyBresenhamLine(struct Point a, struct Point b) {
  int x0 = a.x, y0 = a.y;
  int x1 = b.x, y1 = b.y;

  int dx = abs(x1 - x0);
  int sx = (x0 < x1) ? 1 : -1;
  int dy = -abs(y1 - y0);
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx + dy;

  while (true) {
    DrawPixel(x0, y0, BLACK);
    if (x0 == x1 && y0 == y1)
      break;
    int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

void MyWuLine(struct Point a, struct Point b) {
  float x0 = a.x, y0 = a.y;
  float x1 = b.x, y1 = b.y;
  bool steep = fabsf(y1 - y0) > fabsf(x1 - x0);

  if (steep) {
    float tmp;
    tmp = x0;
    x0 = y0;
    y0 = tmp;
    tmp = x1;
    x1 = y1;
    y1 = tmp;
  }
  if (x0 > x1) {
    float tmp;
    tmp = x0;
    x0 = x1;
    x1 = tmp;
    tmp = y0;
    y0 = y1;
    y1 = tmp;
  }

  float dx = x1 - x0;
  float dy = y1 - y0;
  float gradient = (dx == 0) ? 1 : dy / dx;

  float xend = roundi(x0);
  float yend = y0 + gradient * (xend - x0);
  float xgap = rfpart(x0 + 0.5);
  int xpxl1 = (int)xend;
  int ypxl1 = ipart(yend);
  if (steep) {
    Plot(ypxl1, xpxl1, rfpart(yend) * xgap);
    Plot(ypxl1 + 1, xpxl1, fpart(yend) * xgap);
  } else {
    Plot(xpxl1, ypxl1, rfpart(yend) * xgap);
    Plot(xpxl1, ypxl1 + 1, fpart(yend) * xgap);
  }
  float intery = yend + gradient;

  xend = roundi(x1);
  yend = y1 + gradient * (xend - x1);
  xgap = fpart(x1 + 0.5);
  int xpxl2 = (int)xend;
  int ypxl2 = ipart(yend);
  if (steep) {
    Plot(ypxl2, xpxl2, rfpart(yend) * xgap);
    Plot(ypxl2 + 1, xpxl2, fpart(yend) * xgap);
  } else {
    Plot(xpxl2, ypxl2, rfpart(yend) * xgap);
    Plot(xpxl2, ypxl2 + 1, fpart(yend) * xgap);
  }

  if (steep) {
    for (int x = xpxl1 + 1; x < xpxl2; x++) {
      Plot(ipart(intery), x, rfpart(intery));
      Plot(ipart(intery) + 1, x, fpart(intery));
      intery += gradient;
    }
  } else {
    for (int x = xpxl1 + 1; x < xpxl2; x++) {
      Plot(x, ipart(intery), rfpart(intery));
      Plot(x, ipart(intery) + 1, fpart(intery));
      intery += gradient;
    }
  }
}

int main(void) {
  InitWindow(800, 600, "Lines: Bresenham / Wu");
  SetTargetFPS(60);

  struct Line lines[MAX_LINES];
  int lineCount = 0;
  bool isFirst = true;
  bool useWu = false;

  Camera2D camera = {0};
  camera.zoom = 1.0f;
  camera.offset = (Vector2){0, 0};
  camera.target = (Vector2){0, 0};

  while (!WindowShouldClose()) {
    float wheel = GetMouseWheelMove();
    if (wheel != 0) {
      camera.zoom += wheel * 0.1f;
      if (camera.zoom < 0.1f)
        camera.zoom = 0.1f;
      if (camera.zoom > 10.0f)
        camera.zoom = 10.0f;
    }

    if (IsKeyDown(KEY_RIGHT))
      camera.target.x += 5 / camera.zoom;
    if (IsKeyDown(KEY_LEFT))
      camera.target.x -= 5 / camera.zoom;
    if (IsKeyDown(KEY_DOWN))
      camera.target.y += 5 / camera.zoom;
    if (IsKeyDown(KEY_UP))
      camera.target.y -= 5 / camera.zoom;

    if (IsKeyPressed(KEY_TAB))
      useWu = !useWu;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), camera);
      if (isFirst) {
        lines[lineCount].p1.x = (int)worldPos.x;
        lines[lineCount].p1.y = (int)worldPos.y;
        isFirst = false;
      } else {
        lines[lineCount].p2.x = (int)worldPos.x;
        lines[lineCount].p2.y = (int)worldPos.y;
        isFirst = true;
        if (lineCount < MAX_LINES)
          lineCount++;
      }
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode2D(camera);
    for (int i = 0; i < lineCount; i++) {
      if (useWu)
        MyWuLine(lines[i].p1, lines[i].p2);
      else
        MyBresenhamLine(lines[i].p1, lines[i].p2);
    }
    if (!isFirst && lineCount < MAX_LINES) {
      Vector2 curMouse = GetScreenToWorld2D(GetMousePosition(), camera);
      struct Point preview = {(int)curMouse.x, (int)curMouse.y};
      if (useWu)
        MyWuLine(lines[lineCount].p1, preview);
      else
        MyBresenhamLine(lines[lineCount].p1, preview);
    }
    EndMode2D();

    DrawText(useWu ? "Algorithm: Wu (TAB to switch)"
                   : "Algorithm: Bresenham (TAB to switch)",
             10, 10, 20, DARKGRAY);

    EndDrawing();
  }

  CloseWindow();
  return 0;
}

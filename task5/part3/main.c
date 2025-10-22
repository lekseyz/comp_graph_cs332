#include "raylib.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>

#define MAX_POINTS 100
#define POINT_RADIUS 8
#define SEGMENT_POINTS 50

typedef struct {
    Vector2 points[4];
} BezierSegment;

typedef struct {
    BezierSegment *segments;
    int segmentCount;
    Vector2 *controlPoints;
    int pointCount;
    int selectedPoint;
} BezierCurve;

// Calculate point on Bezier curve for parameter t
Vector2 CalculateBezierPoint(BezierSegment segment, float t) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;
    
    Vector2 point = {
        uuu * segment.points[0].x + 3 * uu * t * segment.points[1].x + 
        3 * u * tt * segment.points[2].x + ttt * segment.points[3].x,
        uuu * segment.points[0].y + 3 * uu * t * segment.points[1].y + 
        3 * u * tt * segment.points[2].y + ttt * segment.points[3].y
    };
    
    return point;
}

// Initialize Bezier curve structure
void InitBezierCurve(BezierCurve *curve) {
    curve->segments = NULL;
    curve->segmentCount = 0;
    curve->controlPoints = NULL;
    curve->pointCount = 0;
    curve->selectedPoint = -1;
}

// Add new control point to the curve
void AddPoint(BezierCurve *curve, Vector2 point) {
    if (curve->pointCount >= MAX_POINTS) return;
    
    curve->controlPoints = realloc(curve->controlPoints, (curve->pointCount + 1) * sizeof(Vector2));
    if (!curve->controlPoints) return;
    
    curve->controlPoints[curve->pointCount] = point;
    curve->pointCount++;
    
    free(curve->segments);
    curve->segmentCount = (curve->pointCount - 1) / 3;
    if (curve->segmentCount > 0) {
        curve->segments = malloc(curve->segmentCount * sizeof(BezierSegment));
        if (!curve->segments) return;
        
        for (int i = 0; i < curve->segmentCount; i++) {
            for (int j = 0; j < 4; j++) {
                curve->segments[i].points[j] = curve->controlPoints[i * 3 + j];
            }
        }
    } else {
        curve->segments = NULL;
    }
}

// Remove control point at specified index
void RemovePoint(BezierCurve *curve, int index) {
    if (index < 0 || index >= curve->pointCount) return;
    
    for (int i = index; i < curve->pointCount - 1; i++) {
        curve->controlPoints[i] = curve->controlPoints[i + 1];
    }
    
    curve->pointCount--;
    if (curve->pointCount > 0) {
        Vector2 *temp = realloc(curve->controlPoints, curve->pointCount * sizeof(Vector2));
        if (temp) curve->controlPoints = temp;
    } else {
        free(curve->controlPoints);
        curve->controlPoints = NULL;
    }
    
    free(curve->segments);
    curve->segmentCount = (curve->pointCount - 1) / 3;
    if (curve->segmentCount > 0) {
        curve->segments = malloc(curve->segmentCount * sizeof(BezierSegment));
        if (curve->segments) {
            for (int i = 0; i < curve->segmentCount; i++) {
                for (int j = 0; j < 4; j++) {
                    curve->segments[i].points[j] = curve->controlPoints[i * 3 + j];
                }
            }
        }
    } else {
        curve->segments = NULL;
    }
    
    if (curve->selectedPoint >= index) curve->selectedPoint--;
}

// Check if point is near mouse cursor
int IsPointNearMouse(Vector2 point, Vector2 mousePos) {
    return CheckCollisionPointCircle(mousePos, point, POINT_RADIUS);
}

// Draw the complete Bezier curve with control points and lines
void DrawBezierCurve(BezierCurve *curve) {
    for (int i = 0; i < curve->segmentCount; i++) {
        for (int j = 0; j < SEGMENT_POINTS; j++) {
            float t1 = (float)j / SEGMENT_POINTS;
            float t2 = (float)(j + 1) / SEGMENT_POINTS;
            
            Vector2 p1 = CalculateBezierPoint(curve->segments[i], t1);
            Vector2 p2 = CalculateBezierPoint(curve->segments[i], t2);
            DrawLineV(p1, p2, BLUE);
        }
        
        DrawLineV(curve->segments[i].points[0], curve->segments[i].points[1], GRAY);
        DrawLineV(curve->segments[i].points[2], curve->segments[i].points[3], GRAY);
    }
    
    for (int i = 0; i < curve->pointCount; i++) {
        Color pointColor = (i == curve->selectedPoint) ? RED : (i % 3 == 0) ? GREEN : DARKGRAY;
        
        DrawCircleV(curve->controlPoints[i], POINT_RADIUS, pointColor);
        DrawCircleLines((int)curve->controlPoints[i].x, (int)curve->controlPoints[i].y, POINT_RADIUS, BLACK);
        
        char label[12];
        snprintf(label, sizeof(label), "%d", i);
        DrawText(label, (int)curve->controlPoints[i].x + POINT_RADIUS + 2,
                (int)curve->controlPoints[i].y - POINT_RADIUS - 2, 10, BLACK);
    }
}

// Draw user interface panel
void DrawUI(BezierCurve *curve) {
    DrawRectangle(10, 10, 250, 120, Fade(WHITE, 0.8f));
    DrawRectangleLines(10, 10, 250, 120, BLACK);
    
    DrawText("Cubic Bezier Splines", 20, 20, 20, BLACK);
    DrawText("LMB: Add/Move point", 20, 50, 10, BLACK);
    DrawText("RMB: Remove point", 20, 65, 10, BLACK);
    DrawText("R: Reset curve", 20, 80, 10, BLACK);
    
    char pointCountText[50];
    snprintf(pointCountText, sizeof(pointCountText), "Points: %d", curve->pointCount);
    DrawText(pointCountText, 20, 100, 10, BLACK);
}

int main(void) {
    const int screenWidth = 1200;
    const int screenHeight = 800;
    
    InitWindow(screenWidth, screenHeight, "Cubic Bezier Splines");
    SetTargetFPS(60);
    
    BezierCurve curve;
    InitBezierCurve(&curve);
    
    AddPoint(&curve, (Vector2){200, 400});
    AddPoint(&curve, (Vector2){300, 200});
    AddPoint(&curve, (Vector2){400, 300});
    AddPoint(&curve, (Vector2){500, 400});
    AddPoint(&curve, (Vector2){600, 200});
    AddPoint(&curve, (Vector2){700, 500});
    
    while (!WindowShouldClose()) {
        Vector2 mousePos = GetMousePosition();
        
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            int pointClicked = -1;
            for (int i = 0; i < curve.pointCount; i++) {
                if (IsPointNearMouse(curve.controlPoints[i], mousePos)) {
                    pointClicked = i;
                    break;
                }
            }
            
            if (pointClicked != -1) {
                curve.selectedPoint = pointClicked;
            } else {
                AddPoint(&curve, mousePos);
                curve.selectedPoint = curve.pointCount - 1;
            }
        }
        
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && curve.selectedPoint != -1) {
            curve.controlPoints[curve.selectedPoint] = mousePos;
            
            for (int i = 0; i < curve.segmentCount; i++) {
                for (int j = 0; j < 4; j++) {
                    curve.segments[i].points[j] = curve.controlPoints[i * 3 + j];
                }
            }
        }
        
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            curve.selectedPoint = -1;
        }
        
        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            for (int i = 0; i < curve.pointCount; i++) {
                if (IsPointNearMouse(curve.controlPoints[i], mousePos)) {
                    RemovePoint(&curve, i);
                    break;
                }
            }
        }
        
        if (IsKeyPressed(KEY_R)) {
            free(curve.controlPoints);
            free(curve.segments);
            InitBezierCurve(&curve);
        }
        
        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        DrawBezierCurve(&curve);
        DrawUI(&curve);
        
        EndDrawing();
    }
    
    free(curve.controlPoints);
    free(curve.segments);
    
    CloseWindow();
    
    return 0;
}

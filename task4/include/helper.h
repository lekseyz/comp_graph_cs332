#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include "math.h"
#include "raylib.h"

int compare_function(const void* a, const void* b) {
    int* x = (int*)a;
    int* y = (int*)b;
    return *x - *y;
}

typedef struct Line {
    Vector2 start;
    Vector2 end;
} Line;

typedef struct PolygonVert {
    struct PolygonVert* prev;
    Vector2 cur;
    struct PolygonVert* next;
} PolygonVert;

typedef struct Polygon {
    PolygonVert* verts;
    size_t size;
} Polygon;

Polygon CreatePolygon(int n_args, ...) {
    return (Polygon) {
        .verts = NULL,
            .size = 0
    };
}

// FIXED: Correct memory deallocation order
void FreePolygon(Polygon polygon) {
    if (polygon.verts == NULL || polygon.size == 0) return;

    PolygonVert* currVert = polygon.verts;

    for (size_t i = 0; i < polygon.size; i++) {
        PolygonVert* nextVert = currVert->next;
        free(currVert);
        currVert = nextVert;
    }
}

void AddPolygonPoint(Polygon* polygon, Vector2 point) {
    PolygonVert* newVert = malloc(sizeof(PolygonVert));
    newVert->cur = point;

    if (polygon->verts == NULL) {
        newVert->next = newVert;
        newVert->prev = newVert;
        polygon->verts = newVert;
        polygon->size = 1;
        return;
    }

    PolygonVert* last = polygon->verts->prev;
    newVert->next = polygon->verts;
    newVert->prev = last;
    last->next = newVert;
    polygon->verts->prev = newVert;
    polygon->size++;
}

// Line intersection check
bool GetLineIntersection(Line line1, Line line2, Vector2* intersection) {
    float x1 = line1.start.x, y1 = line1.start.y;
    float x2 = line1.end.x, y2 = line1.end.y;
    float x3 = line2.start.x, y3 = line2.start.y;
    float x4 = line2.end.x, y4 = line2.end.y;

    float denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
    if (fabs(denom) < 0.0001f) return false;

    float t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
    float u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denom;

    if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
        intersection->x = x1 + t * (x2 - x1);
        intersection->y = y1 + t * (y2 - y1);
        return true;
    }
    return false;
}

// FIXED: Added safety checks
bool IsPointInsidePolygon(Vector2 point, Polygon polygon) {
    if (polygon.size < 3 || polygon.verts == NULL) return false;

    int intersections = 0;
    PolygonVert* vert = polygon.verts;

    for (size_t i = 0; i < polygon.size; i++) {
        Vector2 v1 = vert->cur;
        Vector2 v2 = vert->next->cur;

        if ((v1.y > point.y) != (v2.y > point.y)) {
            float xIntersection = (v2.x - v1.x) * (point.y - v1.y) / (v2.y - v1.y) + v1.x;
            if (point.x < xIntersection) intersections++;
        }
        vert = vert->next;
    }
    return (intersections % 2) == 1;
}

int ClassifyPointToLine(Vector2 point, Line line) {
    float cross = (line.end.x - line.start.x) * (point.y - line.start.y) -
        (line.end.y - line.start.y) * (point.x - line.start.x);
    if (fabs(cross) < 0.0001f) return 0;
    return (cross > 0) ? 1 : -1;
}
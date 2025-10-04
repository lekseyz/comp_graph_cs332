#include <stdlib.h>
#include <stdarg.h>
#include "stddef.h"

#include "raylib.h"

typedef struct Point {
    int x;
    int y;
} Point;

typedef struct Line {
    Point start;
    Point end;
} Line;

typedef struct PolygonVert {
    Point* prev;
    Point* cur;
    Point* next;
} PolygonVert;


typedef struct Polygon {
    PolygonVert* verts;
    size_t size;
    size_t capacity;
} Polygon;

Polygon CreatePolygon(int n_args, ...) {
    va_list args;
    va_start(args, n_args);
    int size = n_args;
    int capacity = n_args * 2;
    PolygonVert* verts = (PolygonVert*)malloc(sizeof(PolygonVert) * capacity);
    PolygonVert* prevVert = verts,
               * nextVert = (verts + 1);
    prevVert->cur = (Point*)malloc(sizeof(Point));
    *prevVert->cur = va_arg(args, Point);

    for (int i = 1; i <= n_args; i++, prevVert++, nextVert++) {
        nextVert->cur = (Point*)malloc(sizeof(Point));
        *prevVert->next = *nextVert->cur = va_arg(args, Point);
    }
    nextVert->next = verts->cur;
    verts->prev = nextVert->cur;

    return (Polygon) {
        .verts = verts,
        .size = size,
        .capacity = capacity
    };
}

void FreePolygon(Polygon polygon) {
    PolygonVert* vert = polygon.verts;
    for (int i = 0; i < polygon.size; i++) {
        free(vert->cur);
    }
    free(polygon.verts);
}

void AddPolygonPoint(Polygon* polygon, Point point) {
    if (polygon->size + 1 > polygon->capacity) {
        int newCapacity = polygon->capacity * 2;
        polygon->verts = (PolygonVert*)realloc(polygon->verts, sizeof(PolygonVert) * newCapacity);
        polygon->capacity = newCapacity;
    }

    Point* newPoint = (Point*)malloc(sizeof(Point));
    *newPoint = point;
    PolygonVert vert = (PolygonVert){ .prev = polygon->verts[polygon->size - 1].cur, .cur = newPoint, .next = polygon->verts[0].cur};
    polygon->verts[polygon->size - 1].next = vert.cur;
    polygon->verts[polygon->size] = vert;
    polygon->size++;
}
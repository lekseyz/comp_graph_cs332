#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>

#include "raylib.h"

int compare_function(const void *a,const void *b) {
int *x = (int *) a;
int *y = (int *) b;
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

void FreePolygon(Polygon polygon) {
    PolygonVert* currVert = polygon.verts;
    PolygonVert* nextVert;

    for (int i = 0; i < polygon.size; i++) {
        free(currVert);
        currVert = nextVert;
        nextVert = nextVert->next;
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
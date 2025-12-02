// Генератор тестовых текстур в формате PPM
#include <cstdio>
#include <cmath>
#include <cstdlib>

void generateChecker(const char* filename, int size, int cellSize) {
    FILE* f = fopen(filename, "wb");
    fprintf(f, "P6\n%d %d\n255\n", size, size);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            unsigned char c = ((x/cellSize + y/cellSize) % 2) ? 50 : 255;
            fputc(c, f); fputc(c, f); fputc(c, f);
        }
    }
    fclose(f);
    printf("Generated: %s\n", filename);
}

void generateGradient(const char* filename, int size) {
    FILE* f = fopen(filename, "wb");
    fprintf(f, "P6\n%d %d\n255\n", size, size);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            unsigned char r = (unsigned char)(255 * x / size);
            unsigned char g = (unsigned char)(100 + 155 * y / size);
            unsigned char b = (unsigned char)(50 + 205 * (size - x) / size);
            fputc(r, f); fputc(g, f); fputc(b, f);
        }
    }
    fclose(f);
    printf("Generated: %s\n", filename);
}

void generateCircles(const char* filename, int size, int rings) {
    FILE* f = fopen(filename, "wb");
    fprintf(f, "P6\n%d %d\n255\n", size, size);
    float cx = size / 2.0f, cy = size / 2.0f;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float dist = sqrtf((x-cx)*(x-cx) + (y-cy)*(y-cy));
            int ring = (int)(dist / (size / 2.0f / rings));
            if (ring % 2 == 0) { fputc(200, f); fputc(150, f); fputc(50, f); }
            else { fputc(50, f); fputc(100, f); fputc(200, f); }
        }
    }
    fclose(f);
    printf("Generated: %s\n", filename);
}

int main() {
    generateChecker("textures/texture1.ppm", 256, 32);
    generateGradient("textures/texture2.ppm", 256);
    generateCircles("textures/texture3.ppm", 256, 5);
    printf("Done! Use .ppm files or convert to PNG/JPG\n");
    return 0;
}

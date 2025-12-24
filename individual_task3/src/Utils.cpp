#include "Utils.h"

Texture2D GenerateFlatNormalMap(int width, int height) {
    Image img = GenImageColor(width, height, (Color){128, 128, 255, 255});
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}
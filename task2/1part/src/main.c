#include "math.h"
#include "stdio.h"

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define IMAGE_WINDOW_WIDTH 600
#define IMAGE_WINDOW_HEIGTH 600

Color GrayPAL(Color color);
Color GrayHDTV(Color color);
Image ImageColorManipulate(const Image* image, Color (colorManipulator)(Color));

int main()
{
    char* imageFile = "./images/color_image.jpg";

    if (!FileExists(imageFile)){
        return -1;
    }

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "gray");
    SetTargetFPS(60);
    
    Image defaultImage = LoadImage(imageFile);

    if (!IsImageValid(defaultImage)){
        return -1;
    }

    if (defaultImage.width > IMAGE_WINDOW_WIDTH || defaultImage.width > IMAGE_WINDOW_HEIGTH) {
        float wscale = defaultImage.width / (float) IMAGE_WINDOW_WIDTH;
        float hscale = defaultImage.height / (float) IMAGE_WINDOW_HEIGTH;

        float scale = fmaxf(wscale, hscale);
        ImageResize(&defaultImage, defaultImage.width / scale, defaultImage.height / scale);
    }

    Image pal = ImageColorManipulate(&defaultImage, GrayPAL);
    Image hdtv = ImageColorManipulate(&defaultImage, GrayHDTV);
    Image diffGray = GenImageColor(defaultImage.width, defaultImage.height, WHITE);

    for (int x = 0; x < pal.width; x++) {
        for (int y = 0; y < pal.height; y++) {
            Color color = (Color) {
                .a = 255,
                .r = abs(GetImageColor(pal, x, y).r - GetImageColor(hdtv, x, y).r),
                .g = abs(GetImageColor(pal, x, y).g - GetImageColor(hdtv, x, y).g),
                .b = abs(GetImageColor(pal, x, y).b - GetImageColor(hdtv, x, y).b),
            };
            ImageDrawPixel(&diffGray, x, y, color);
        }
    }

    Texture2D texture = LoadTextureFromImage(defaultImage);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        
        if (GuiButton((Rectangle){610, 50, 150, 70}, "default")) {
            UnloadTexture(texture);
            texture = LoadTextureFromImage(defaultImage);
        }
        if (GuiButton((Rectangle){610, 130, 150, 70}, "pal")) {
            UnloadTexture(texture);
            texture = LoadTextureFromImage(pal);
        }
        if (GuiButton((Rectangle){610, 210, 150, 70}, "hdtv")) {
            UnloadTexture(texture);
            texture = LoadTextureFromImage(hdtv);
        }
        if (GuiButton((Rectangle){610, 290, 150, 70}, "diff")) {
            UnloadTexture(texture);
            texture = LoadTextureFromImage(diffGray);
        }

        ClearBackground(WHITE);
        DrawTexture(texture, 0, 0, WHITE); 

        EndDrawing();
    }

    UnloadTexture(texture);
    CloseWindow();
    return 0;
}

Color GrayPAL(Color color) {
    unsigned char gray = (unsigned char)(color.r * 0.299 + color.g * 0.587 + color.b * 0.114);
    return (Color) { .r = gray,
                     .g = gray,
                     .b = gray,
                     .a = color.a};
}

Color GrayHDTV(Color color) {
    unsigned char gray = (unsigned char)(color.r * 0.2126 + color.g * 0.7152 + color.b * 0.0722);
    return (Color) { .r = gray,
                     .g = gray,
                     .b = gray,
                     .a = color.a};
}

Image ImageColorManipulate(const Image* image, Color (colorManipulator)(Color)) {
    Image newImage = ImageCopy(*image);
    for (int x = 0; x < newImage.width; x++) {
        for (int y = 0; y < newImage.height; y++) {
            Color curColor, newColor;
            curColor = GetImageColor(newImage, x, y);
            newColor = colorManipulator(curColor);

            ImageDrawPixel(&newImage, x, y, newColor);
        }
    }

    return newImage;
}
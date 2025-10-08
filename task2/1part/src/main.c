#include "math.h"
#include "stdio.h"

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define IMAGE_WINDOW_WIDTH 600
#define IMAGE_WINDOW_HEIGTH 600

#define HISTOGRAM_HEIGHT 100
#define HISTOGRAM_WIDTH 256
#define HISTOGRAM_MARGIN 20

Color GrayPAL(Color color);
Color GrayHDTV(Color color);
Image ImageColorManipulate(const Image* image, Color (colorManipulator)(Color));
void DrawHistogram(int* histogram, Color color, int posX, int posY);
int* CalculateHistogram(const Image* image);

int main()
{
    char* imageFile = "./images/color_image.jpg";

    if (!FileExists(imageFile)){
        printf("File not found: %s\n", imageFile);
        return -1;
    }

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "gray");
    SetTargetFPS(60);
    
    Image defaultImage = LoadImage(imageFile);

    if (!IsImageValid(defaultImage)){
        printf("Invalid image\n");
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

    
    int* histDefault = CalculateHistogram(&defaultImage);
    int* histPAL = CalculateHistogram(&pal);
    int* histHDTV = CalculateHistogram(&hdtv);
    int* histDiff = CalculateHistogram(&diffGray);

    Texture2D texture = LoadTextureFromImage(defaultImage);
    
    int currentMode = 0; 
    int* currentHistogram = histDefault;

    while (!WindowShouldClose())
    {
        
        if (GuiButton((Rectangle){610, 50, 150, 70}, "default")) {
            UnloadTexture(texture);
            texture = LoadTextureFromImage(defaultImage);
            currentMode = 0;
            currentHistogram = histDefault;
        }
        if (GuiButton((Rectangle){610, 130, 150, 70}, "pal")) {
            UnloadTexture(texture);
            texture = LoadTextureFromImage(pal);
            currentMode = 1;
            currentHistogram = histPAL;
        }
        if (GuiButton((Rectangle){610, 210, 150, 70}, "hdtv")) {
            UnloadTexture(texture);
            texture = LoadTextureFromImage(hdtv);
            currentMode = 2;
            currentHistogram = histHDTV;
        }
        if (GuiButton((Rectangle){610, 290, 150, 70}, "diff")) {
            UnloadTexture(texture);
            texture = LoadTextureFromImage(diffGray);
            currentMode = 3;
            currentHistogram = histDiff;
        }

        BeginDrawing();
        
        ClearBackground(WHITE);
        DrawTexture(texture, 0, 0, WHITE);

        
        const char* modeText = "";
        switch (currentMode) {
            case 0: modeText = "Original RGB"; break;
            case 1: modeText = "PAL Grayscale"; break;
            case 2: modeText = "HDTV Grayscale"; break;
            case 3: modeText = "Difference"; break;
        }
        DrawText(modeText, 610, 10, 20, DARKGRAY);

        
        DrawHistogram(currentHistogram, BLACK, 0, texture.height + 10);

        if (currentMode == 1 || currentMode == 2) {
            DrawHistogram(histPAL, RED, 300, texture.height + 10);
            DrawText("PAL", 300, texture.height + HISTOGRAM_HEIGHT + 15, 12, RED);
            
            DrawHistogram(histHDTV, BLUE, 300 + HISTOGRAM_WIDTH + 10, texture.height + 10);
            DrawText("HDTV", 300 + HISTOGRAM_WIDTH + 10, texture.height + HISTOGRAM_HEIGHT + 15, 12, BLUE);
        }

        EndDrawing();
    }

    
    free(histDefault);
    free(histPAL);
    free(histHDTV);
    free(histDiff);

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

int* CalculateHistogram(const Image* image) {
    int* histogram = (int*)calloc(256, sizeof(int));
    
    for (int x = 0; x < image->width; x++) {
        for (int y = 0; y < image->height; y++) {
            Color color = GetImageColor(*image, x, y);
            unsigned char intensity;
            if (color.r == color.g && color.g == color.b) {
                intensity = color.r; 
            } else {
                intensity = (color.r + color.g + color.b) / 3; 
            }
            histogram[intensity]++;
        }
    }
    
    return histogram;
}

void DrawHistogram(int* histogram, Color color, int posX, int posY) {
    int maxValue = 0;
    for (int i = 0; i < 256; i++) {
        if (histogram[i] > maxValue) {
            maxValue = histogram[i];
        }
    }
    
    if (maxValue == 0) return; 
    
    DrawLine(posX, posY, posX + HISTOGRAM_WIDTH, posY, DARKGRAY); 
    DrawLine(posX, posY, posX, posY - HISTOGRAM_HEIGHT, DARKGRAY); 
    
    
    for (int i = 0; i < 256; i++) {
        int barHeight = (int)((histogram[i] / (float)maxValue) * HISTOGRAM_HEIGHT);
        if (barHeight > 0) {
            DrawLine(posX + i, posY, posX + i, posY - barHeight, color);
        }
    }
    DrawText("0", posX, posY + 5, 10, DARKGRAY);
    DrawText("255", posX + HISTOGRAM_WIDTH - 20, posY + 5, 10, DARKGRAY);
    DrawText("Intensity", posX + HISTOGRAM_WIDTH/2 - 25, posY + 15, 12, DARKGRAY);
}
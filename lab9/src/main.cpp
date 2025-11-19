#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef enum {
    SHADING_GOURAUD = 0,
    SHADING_PHONG_TOON
} ShadingMode;

ShadingMode currentShading = SHADING_GOURAUD;

Mesh LoadMeshFromOBJ(const char* fileName) {
    Mesh mesh = { 0 };

    FILE* file = fopen(fileName, "r");
    if (!file) {
        printf("Error: Could not open file %s\n", fileName);
        return mesh;
    }

    Vector3* tempVertices = (Vector3*)malloc(10000 * sizeof(Vector3));
    int* tempIndices = (int*)malloc(30000 * sizeof(int));
    int vertexCount = 0;
    int indexCount = 0;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ') {
            float x, y, z;
            sscanf(line, "v %f %f %f", &x, &y, &z);
            tempVertices[vertexCount++] = (Vector3){ x, y, z };
        }
        else if (line[0] == 'f' && line[1] == ' ') {
            int v1, v2, v3;
            if (sscanf(line, "f %d %d %d", &v1, &v2, &v3) == 3) {
                tempIndices[indexCount++] = v1 - 1;
                tempIndices[indexCount++] = v2 - 1;
                tempIndices[indexCount++] = v3 - 1;
            }
            else {
                char* token = strtok(line + 2, " ");
                int indices[3];
                int idx = 0;
                while (token && idx < 3) {
                    sscanf(token, "%d", &indices[idx]);
                    tempIndices[indexCount++] = indices[idx] - 1;
                    idx++;
                    token = strtok(NULL, " ");
                }
            }
        }
    }
    fclose(file);

    mesh.vertexCount = vertexCount;
    mesh.triangleCount = indexCount / 3;

    mesh.vertices = (float*)malloc(vertexCount * 3 * sizeof(float));
    mesh.indices = (unsigned short*)malloc(indexCount * sizeof(unsigned short));
    mesh.normals = (float*)malloc(vertexCount * 3 * sizeof(float));

    for (int i = 0; i < vertexCount; i++) {
        mesh.vertices[i * 3] = tempVertices[i].x;
        mesh.vertices[i * 3 + 1] = tempVertices[i].y;
        mesh.vertices[i * 3 + 2] = tempVertices[i].z;
        mesh.normals[i * 3] = 0;
        mesh.normals[i * 3 + 1] = 0;
        mesh.normals[i * 3 + 2] = 0;
    }

    for (int i = 0; i < indexCount; i++) {
        mesh.indices[i] = tempIndices[i];
    }

    for (int i = 0; i < mesh.triangleCount; i++) {
        int idx1 = mesh.indices[i * 3];
        int idx2 = mesh.indices[i * 3 + 1];
        int idx3 = mesh.indices[i * 3 + 2];

        Vector3 v1 = { mesh.vertices[idx1 * 3], mesh.vertices[idx1 * 3 + 1], mesh.vertices[idx1 * 3 + 2] };
        Vector3 v2 = { mesh.vertices[idx2 * 3], mesh.vertices[idx2 * 3 + 1], mesh.vertices[idx2 * 3 + 2] };
        Vector3 v3 = { mesh.vertices[idx3 * 3], mesh.vertices[idx3 * 3 + 1], mesh.vertices[idx3 * 3 + 2] };

        Vector3 edge1 = Vector3Subtract(v2, v1);
        Vector3 edge2 = Vector3Subtract(v3, v1);
        Vector3 normal = Vector3Normalize(Vector3CrossProduct(edge1, edge2));

        mesh.normals[idx1 * 3] += normal.x;
        mesh.normals[idx1 * 3 + 1] += normal.y;
        mesh.normals[idx1 * 3 + 2] += normal.z;

        mesh.normals[idx2 * 3] += normal.x;
        mesh.normals[idx2 * 3 + 1] += normal.y;
        mesh.normals[idx2 * 3 + 2] += normal.z;

        mesh.normals[idx3 * 3] += normal.x;
        mesh.normals[idx3 * 3 + 1] += normal.y;
        mesh.normals[idx3 * 3 + 2] += normal.z;
    }

    for (int i = 0; i < vertexCount; i++) {
        Vector3 n = { mesh.normals[i * 3], mesh.normals[i * 3 + 1], mesh.normals[i * 3 + 2] };
        n = Vector3Normalize(n);
        mesh.normals[i * 3] = n.x;
        mesh.normals[i * 3 + 1] = n.y;
        mesh.normals[i * 3 + 2] = n.z;
    }

    free(tempVertices);
    free(tempIndices);

    UploadMesh(&mesh, false);
    return mesh;
}

Mesh GenTexturedTetrahedron(void) {
    Mesh mesh = { 0 };

    Vector3 baseVerts[4] = {
        { 1.0f,  1.0f,  1.0f },
        { -1.0f, -1.0f,  1.0f },
        { -1.0f,  1.0f, -1.0f },
        { 1.0f, -1.0f, -1.0f }
    };

    int faces[4][3] = {
        { 0, 2, 1 },
        { 0, 1, 3 },
        { 0, 3, 2 },
        { 1, 2, 3 }
    };

    float uvs[3][2] = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 0.5f, 1.0f }
    };

    mesh.triangleCount = 4;
    mesh.vertexCount = mesh.triangleCount * 3;

    mesh.vertices = (float*)malloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float*)malloc(mesh.vertexCount * 2 * sizeof(float));
    mesh.normals = (float*)malloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.indices = (unsigned short*)malloc(mesh.triangleCount * 3 * sizeof(unsigned short));

    for (int f = 0; f < 4; f++) {
        Vector3 p0 = baseVerts[faces[f][0]];
        Vector3 p1 = baseVerts[faces[f][1]];
        Vector3 p2 = baseVerts[faces[f][2]];

        Vector3 edge1 = Vector3Subtract(p1, p0);
        Vector3 edge2 = Vector3Subtract(p2, p0);
        Vector3 normal = Vector3Normalize(Vector3CrossProduct(edge1, edge2));

        for (int v = 0; v < 3; v++) {
            int dst = f * 3 + v;
            Vector3 p = baseVerts[faces[f][v]];

            mesh.vertices[dst * 3 + 0] = p.x;
            mesh.vertices[dst * 3 + 1] = p.y;
            mesh.vertices[dst * 3 + 2] = p.z;

            mesh.normals[dst * 3 + 0] = normal.x;
            mesh.normals[dst * 3 + 1] = normal.y;
            mesh.normals[dst * 3 + 2] = normal.z;

            mesh.texcoords[dst * 2 + 0] = uvs[v][0];
            mesh.texcoords[dst * 2 + 1] = uvs[v][1];

            mesh.indices[dst] = (unsigned short)dst;
        }
    }

    UploadMesh(&mesh, false);
    return mesh;
}

Mesh GenTexturedCube(void) {
    Mesh mesh = GenMeshCube(2.0f, 2.0f, 2.0f);
    UploadMesh(&mesh, false);
    return mesh;
}

Mesh GenTexturedOctahedron(void) {
    Mesh mesh = { 0 };

    Vector3 baseVerts[6] = {
        { 1.0f, 0.0f, 0.0f },
        { -1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, -1.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, -1.0f }
    };

    int faces[8][3] = {
        { 2, 4, 0 },
        { 2, 1, 4 },
        { 2, 5, 1 },
        { 2, 0, 5 },
        { 3, 0, 4 },
        { 3, 4, 1 },
        { 3, 1, 5 },
        { 3, 5, 0 }
    };

    float uvs[3][2] = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 0.5f, 1.0f }
    };

    mesh.triangleCount = 8;
    mesh.vertexCount = mesh.triangleCount * 3;

    mesh.vertices = (float*)malloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.texcoords = (float*)malloc(mesh.vertexCount * 2 * sizeof(float));
    mesh.normals = (float*)malloc(mesh.vertexCount * 3 * sizeof(float));
    mesh.indices = (unsigned short*)malloc(mesh.triangleCount * 3 * sizeof(unsigned short));

    for (int f = 0; f < 8; f++) {
        Vector3 p0 = baseVerts[faces[f][0]];
        Vector3 p1 = baseVerts[faces[f][1]];
        Vector3 p2 = baseVerts[faces[f][2]];

        Vector3 edge1 = Vector3Subtract(p1, p0);
        Vector3 edge2 = Vector3Subtract(p2, p0);
        Vector3 normal = Vector3Normalize(Vector3CrossProduct(edge1, edge2));

        for (int v = 0; v < 3; v++) {
            int dst = f * 3 + v;
            Vector3 p = baseVerts[faces[f][v]];

            mesh.vertices[dst * 3 + 0] = p.x;
            mesh.vertices[dst * 3 + 1] = p.y;
            mesh.vertices[dst * 3 + 2] = p.z;

            mesh.normals[dst * 3 + 0] = normal.x;
            mesh.normals[dst * 3 + 1] = normal.y;
            mesh.normals[dst * 3 + 2] = normal.z;

            mesh.texcoords[dst * 2 + 0] = uvs[v][0];
            mesh.texcoords[dst * 2 + 1] = uvs[v][1];

            mesh.indices[dst] = (unsigned short)dst;
        }
    }

    UploadMesh(&mesh, false);
    return mesh;
}

int main(void) {
    const int screenWidth = 1200;
    const int screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "3D Lighting: Gouraud vs Phong Toon Shading");
    rlEnableBackfaceCulling();

    Camera camera = { 0 };
    camera.position = (Vector3){ 5.0f, 5.0f, 5.0f };
    camera.target = (Vector3){ 0.0f,  0.0f,  0.0f };
    camera.up = (Vector3){ 0.0f,  1.0f,  0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    bool cameraActive = true;

    Model models[10];
    int modelCount = 0;

    Mesh tetraMesh = GenTexturedTetrahedron();
    models[modelCount++] = LoadModelFromMesh(tetraMesh);

    Mesh cubeMesh = GenTexturedCube();
    models[modelCount++] = LoadModelFromMesh(cubeMesh);

    Mesh octaMesh = GenTexturedOctahedron();
    models[modelCount++] = LoadModelFromMesh(octaMesh);

    if (DirectoryExists("models")) {
        FilePathList files = LoadDirectoryFilesEx("models", ".obj", false);
        for (unsigned int i = 0; i < files.count && modelCount < 10; i++) {
            models[modelCount++] = LoadModel(files.paths[i]);
            // Mesh mesh = LoadMeshFromOBJ(files.paths[i]);
            // if (mesh.vertexCount > 0) {
            //     models[modelCount++] = LoadModelFromMesh(mesh);
            //     printf("Loaded: %s\n", files.paths[i]);
            // }
        }
        UnloadDirectoryFiles(files);
    }

    if (modelCount == 0) {
        models[0] = LoadModelFromMesh(GenMeshSphere(1.0f, 32, 32));
        modelCount = 1;
    }

    int currentModelIndex = 0;

    const char* gouraudVS =
        "#version 330\n"
        "in vec3 vertexPosition;\n"
        "in vec3 vertexNormal;\n"
        "in vec2 vertexTexCoord;\n"
        "uniform mat4 mvp;\n"
        "uniform mat4 matModel;\n"
        "uniform vec3 lightPos;\n"
        "uniform vec3 viewPos;\n"
        "out vec3 fragColor;\n"
        "out vec2 fragTexCoord;\n"
        "void main() {\n"
        "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
        "    vec3 worldPos = (matModel * vec4(vertexPosition, 1.0)).xyz;\n"
        "    vec3 worldNormal = normalize(mat3(matModel) * vertexNormal);\n"
        "    vec3 lightDir = normalize(lightPos - worldPos);\n"
        "    float diff = max(dot(worldNormal, lightDir), 0.0);\n"
        "    vec3 ambient = vec3(0.2);\n"
        "    vec3 diffuse = vec3(0.8, 0.2, 0.2) * diff;\n"
        "    fragColor = ambient + diffuse;\n"
        "    fragTexCoord = vertexTexCoord;\n"
        "}\n";

    const char* gouraudFS =
        "#version 330\n"
        "in vec3 fragColor;\n"
        "in vec2 fragTexCoord;\n"
        "uniform sampler2D texture0;\n"
        "out vec4 finalColor;\n"
        "void main() {\n"
        "    vec4 texColor = texture(texture0, fragTexCoord);\n"
        "    if (texColor.a <= 0.0) discard;\n"
        "    finalColor = vec4(fragColor, 1.0) * texColor;\n"
        "}\n";

    const char* phongToonVS =
        "#version 330\n"
        "in vec3 vertexPosition;\n"
        "in vec3 vertexNormal;\n"
        "in vec2 vertexTexCoord;\n"
        "uniform mat4 mvp;\n"
        "uniform mat4 matModel;\n"
        "out vec3 fragPos;\n"
        "out vec3 fragNormal;\n"
        "out vec2 fragTexCoord;\n"
        "void main() {\n"
        "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
        "    fragPos = (matModel * vec4(vertexPosition, 1.0)).xyz;\n"
        "    fragNormal = mat3(matModel) * vertexNormal;\n"
        "    fragTexCoord = vertexTexCoord;\n"
        "}\n";

    const char* phongToonFS =
        "#version 330\n"
        "in vec3 fragPos;\n"
        "in vec3 fragNormal;\n"
        "in vec2 fragTexCoord;\n"
        "uniform vec3 lightPos;\n"
        "uniform vec3 viewPos;\n"
        "uniform sampler2D texture0;\n"
        "out vec4 finalColor;\n"
        "void main() {\n"
        "    vec3 normal = normalize(fragNormal);\n"
        "    vec3 lightDir = normalize(lightPos - fragPos);\n"
        "    float diff = max(dot(normal, lightDir), 0.0);\n"
        "    vec3 color;\n"
        "    if (diff < 0.3) color = vec3(0.2, 0.1, 0.5);\n"
        "    else if (diff < 0.6) color = vec3(0.4, 0.2, 0.7);\n"
        "    else color = vec3(0.6, 0.4, 0.9);\n"
        "    vec3 viewDir = normalize(viewPos - fragPos);\n"
        "    float edge = max(dot(viewDir, normal), 0.0);\n"
        "    if (edge < 0.3) color = vec3(0.0);\n"
        "    vec4 texColor = texture(texture0, fragTexCoord);\n"
        "    if (texColor.a <= 0.0) discard;\n"
        "    finalColor = vec4(color, 1.0) * texColor;\n"
        "}\n";

    Shader gouraudShader = LoadShaderFromMemory(gouraudVS, gouraudFS);
    Shader phongToonShader = LoadShaderFromMemory(phongToonVS, phongToonFS);

    gouraudShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(gouraudShader, "mvp");
    gouraudShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(gouraudShader, "matModel");
    gouraudShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(gouraudShader, "viewPos");
    gouraudShader.locs[SHADER_LOC_MAP_DIFFUSE] = GetShaderLocation(gouraudShader, "texture0");
    int gouraudLightLoc = GetShaderLocation(gouraudShader, "lightPos");

    phongToonShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(phongToonShader, "mvp");
    phongToonShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(phongToonShader, "matModel");
    phongToonShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(phongToonShader, "viewPos");
    phongToonShader.locs[SHADER_LOC_MAP_DIFFUSE] = GetShaderLocation(phongToonShader, "texture0");
    int phongLightLoc = GetShaderLocation(phongToonShader, "lightPos");

    Vector3 lightPos = { 3.0f, 4.0f, 3.0f };

    Mesh planeMesh = GenMeshPlane(20.0f, 20.0f, 1, 1);
    Model planeModel = LoadModelFromMesh(planeMesh);
    planeModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = (Color){ 80, 80, 80, 255 };

    Texture2D texModel = LoadTexture("textures/uv.png");
    Texture2D texGround = LoadTexture("textures/uv.png");

    for (int i = 0; i < modelCount; i++) {
        models[i].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texModel;
    }
    //planeModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texGround;

    Vector3 modelPosition = { 0.0f, 1.0f, 0.0f };
    float modelRotation = 0.0f;
    float modelScale = 1.0f;

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        if (cameraActive) {
            UpdateCamera(&camera, CAMERA_ORBITAL);
        }

        if (IsKeyPressed(KEY_C)) {
            cameraActive = !cameraActive;
        }

        if (IsKeyDown(KEY_UP)) modelPosition.z += 0.05f;
        if (IsKeyDown(KEY_DOWN)) modelPosition.z -= 0.05f;
        if (IsKeyDown(KEY_A)) modelPosition.x += 0.05f;
        if (IsKeyDown(KEY_D)) modelPosition.x -= 0.05f;

        if (IsKeyDown(KEY_W)) modelPosition.y += 0.05f;
        if (IsKeyDown(KEY_S)) modelPosition.y -= 0.05f;

        if (modelPosition.y < 0.0f) modelPosition.y = 0.0f;

        if (IsKeyDown(KEY_Q)) modelRotation += 1.0f;
        if (IsKeyDown(KEY_E)) modelRotation -= 1.0f;

        if (IsKeyDown(KEY_Z)) modelScale -= 0.01f;
        if (IsKeyDown(KEY_X)) modelScale += 0.01f;
        if (modelScale < 0.1f) modelScale = 0.1f;

        if (IsKeyPressed(KEY_M)) {
            currentModelIndex = (currentModelIndex + 1) % modelCount;
        }

        if (IsKeyPressed(KEY_SPACE)) {
            currentShading = (currentShading == SHADING_GOURAUD) ? SHADING_PHONG_TOON : SHADING_GOURAUD;
        }

        Shader currentShader = (currentShading == SHADING_GOURAUD) ? gouraudShader : phongToonShader;
        int lightLoc = (currentShading == SHADING_GOURAUD) ? gouraudLightLoc : phongLightLoc;

        float lightPosF[3] = { lightPos.x, lightPos.y, lightPos.z };
        float viewPosF[3] = { camera.position.x, camera.position.y, camera.position.z };

        SetShaderValue(currentShader, lightLoc, lightPosF, SHADER_UNIFORM_VEC3);
        SetShaderValue(currentShader, currentShader.locs[SHADER_LOC_VECTOR_VIEW], viewPosF, SHADER_UNIFORM_VEC3);

        models[currentModelIndex].materials[0].shader = currentShader;

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        DrawModel(planeModel, (Vector3){ 0, -0.01f, 0 }, 1.0f, GRAY);

        Matrix transform = MatrixIdentity();
        transform = MatrixMultiply(transform, MatrixScale(modelScale, modelScale, modelScale));
        transform = MatrixMultiply(transform, MatrixRotateY(modelRotation * DEG2RAD));
        transform = MatrixMultiply(transform, MatrixTranslate(modelPosition.x, modelPosition.y, modelPosition.z));

        models[currentModelIndex].transform = transform;
        DrawModel(models[currentModelIndex], Vector3Zero(), 1.0f, WHITE);

        DrawSphere(lightPos, 0.2f, YELLOW);

        DrawGrid(10, 1.0f);

        EndMode3D();

        DrawText("Controls:", 10, 10, 20, DARKGRAY);
        DrawText("SPACE - Switch shading mode", 10, 35, 20, DARKGRAY);
        DrawText("C - Toggle camera (orbital/static)", 10, 60, 20, DARKGRAY);
        DrawText("M - Switch model", 10, 85, 20, DARKGRAY);
        DrawText("Arrow keys - Move forward/backward (Z)", 10, 110, 20, DARKGRAY);
        DrawText("A/D - Move left/right (X)", 10, 135, 20, DARKGRAY);
        DrawText("W/S - Move up/down (Y)", 10, 160, 20, DARKGRAY);
        DrawText("Q/E - Rotate left/right", 10, 185, 20, DARKGRAY);
        DrawText("Z/X - Scale down/up", 10, 210, 20, DARKGRAY);

        const char* shadingName = (currentShading == SHADING_GOURAUD) ? "Gouraud (Lambert)" : "Phong (Toon)";
        const char* cameraStatus = cameraActive ? "Active" : "Static";

        DrawText(TextFormat("Shading: %s", shadingName), 10, 240, 20, RED);
        DrawText(TextFormat("Camera: %s", cameraStatus), 10, 265, 20, BLUE);
        DrawText(TextFormat("Height: %.2f", modelPosition.y), 10, 290, 20, GREEN);
        DrawText(TextFormat("Scale: %.2f", modelScale), 10, 315, 20, PURPLE);

        DrawFPS(10, 340);

        EndDrawing();
    }

    UnloadTexture(texModel);
    UnloadTexture(texGround);

    UnloadShader(gouraudShader);
    UnloadShader(phongToonShader);
    for (int i = 0; i < modelCount; i++) {
        UnloadModel(models[i]);
    }
    UnloadModel(planeModel);

    CloseWindow();
    return 0;
}

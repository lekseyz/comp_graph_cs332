#include "raylib.h"
#include "rlgl.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <raymath.h>

// Function to load a model from an OBJ file
Model LoadObjModel(const char* fileName);
void CalculateNormals(Mesh *mesh);

int main()
{
    // Initialization
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "3D Lighting");

    // Define the camera
    Camera camera = { };
    camera.position = (Vector3){ 5.0f, 5.0f, 5.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Load the models
    std::vector<Model> models;
    FilePathList fileList = LoadDirectoryFilesEx("models", ".obj", false);

    for (unsigned int i = 0; i < fileList.count; i++) {
        models.push_back(LoadObjModel(fileList.paths[i]));
    }
    UnloadDirectoryFiles(fileList);

    int currentModelIndex = 0;

    // Load the lighting shader
    Shader lightingShader = LoadShader("lighting.vs", "lighting.fs");

    // Get shader locations
    lightingShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(lightingShader, "mvp");
    lightingShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(lightingShader, "viewPos");
    lightingShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(lightingShader, "matModel");
    
    // Set shader for the models
    for (size_t i = 0; i < models.size(); i++) {
        models[i].materials[0].shader = lightingShader;
    }
    
    // Create a light
    Vector3 lightPos = { 2.0f, 2.0f, 2.0f };

    // Model transformation
    Vector3 position = { 0.0f, 0.0f, 0.0f };
    float rotation = 0.0f;
    float scale = 1.0f;

    SetTargetFPS(60);
    
    // Enable back-face culling
    rlEnableBackfaceCulling();

    // Main game loop
    while (!WindowShouldClose())
    {
        // Update
        UpdateCamera(&camera, CAMERA_ORBITAL);

        // Handle model transformations
        if (IsKeyDown(KEY_UP)) position.z += 0.1f;
        if (IsKeyDown(KEY_DOWN)) position.z -= 0.1f;
        if (IsKeyDown(KEY_LEFT)) position.x += 0.1f;
        if (IsKeyDown(KEY_RIGHT)) position.x -= 0.1f;
        if (IsKeyDown(KEY_A)) rotation += 1.0f;
        if (IsKeyDown(KEY_D)) rotation -= 1.0f;
        if (IsKeyDown(KEY_W)) scale += 0.01f;
        if (IsKeyDown(KEY_S)) scale -= 0.01f;
        if (scale < 0.1f) scale = 0.1f;

        // Handle model switching
        if (IsKeyPressed(KEY_M))
        {
            if (!models.empty()) {
                currentModelIndex = (currentModelIndex + 1) % models.size();
            }
        }


        // Update the lighting shader with the light position
        float lightPosF[3] = { lightPos.x, lightPos.y, lightPos.z };
        SetShaderValue(lightingShader, GetShaderLocation(lightingShader, "lightPos"), lightPosF, SHADER_UNIFORM_VEC3);
        
        // Update the lighting shader with the view position
        float viewPosF[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(lightingShader, lightingShader.locs[SHADER_LOC_VECTOR_VIEW], viewPosF, SHADER_UNIFORM_VEC3);


        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);

        if (!models.empty()) {
            Model currentModel = models[currentModelIndex];
            DrawModelEx(currentModel, position, { 0.0f, 1.0f, 0.0f }, rotation, { scale, scale, scale }, RED);

            // Visualize normals
            Vector3 rotationAxis = { 0.0f, 1.0f, 0.0f };
            Matrix transform = MatrixMultiply(MatrixMultiply(MatrixScale(scale, scale, scale),
                                                            MatrixRotate(rotationAxis, rotation * DEG2RAD)),
                                            MatrixTranslate(position.x, position.y, position.z));

            Mesh mesh = currentModel.meshes[0];
            for (int i = 0; i < mesh.vertexCount; i++)
            {
                Vector3 vertex = { mesh.vertices[i*3], mesh.vertices[i*3+1], mesh.vertices[i*3+2] };
                Vector3 normal = { mesh.normals[i*3], mesh.normals[i*3+1], mesh.normals[i*3+2] };

                Vector3 transformedVertex = Vector3Transform(vertex, transform);
                
                Matrix inv_transform = MatrixInvert(transform);
                Matrix inv_transpose_transform = MatrixTranspose(inv_transform);
                Vector3 transformedNormal = Vector3Transform(normal, inv_transpose_transform);
                transformedNormal = Vector3Normalize(transformedNormal);

                float normalLength = 0.2f;
                Vector3 normalEnd = Vector3Add(transformedVertex, Vector3Scale(transformedNormal, normalLength));

                DrawLine3D(transformedVertex, normalEnd, BLUE);
            }
        }
        
        DrawGrid(10, 1.0f);
        DrawSphereEx(lightPos, 0.2f, 8, 8, YELLOW);

        EndMode3D();

        DrawText("Use arrow keys to move, A/D to rotate, W/S to scale", 10, 10, 20, DARKGRAY);
        DrawText("Press M to switch models", 10, 40, 20, DARKGRAY);
        DrawFPS(10, 70);


        EndDrawing();
    }

    // De-Initialization
    UnloadShader(lightingShader);
    for (size_t i = 0; i < models.size(); i++) {
        UnloadModel(models[i]);
    }
    CloseWindow();

    return 0;
}

Model LoadObjModel(const char* fileName)
{
    printf("Loading model: %s\n", fileName);
    Model model = { };
    std::vector<Vector3> vertices;
    std::vector<int> face_indices;

    std::ifstream file(fileName);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << fileName << std::endl;
        return model;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            Vector3 vertex;
            ss >> vertex.x >> vertex.y >> vertex.z;
            vertices.push_back(vertex);
        } else if (type == "f") {
            std::string v1_str, v2_str, v3_str;
            ss >> v1_str >> v2_str >> v3_str;
            
            face_indices.push_back(std::stoi(v1_str.substr(0, v1_str.find('/'))) - 1);
            face_indices.push_back(std::stoi(v2_str.substr(0, v2_str.find('/'))) - 1);
            face_indices.push_back(std::stoi(v3_str.substr(0, v3_str.find('/'))) - 1);
        }
    }

    file.close();

    Mesh mesh = { };
    mesh.vertexCount = vertices.size();
    mesh.triangleCount = face_indices.size() / 3;

    mesh.vertices = new float[mesh.vertexCount * 3];
    mesh.indices = new unsigned short[face_indices.size()];

    for (size_t i = 0; i < vertices.size(); ++i) {
        mesh.vertices[i * 3] = vertices[i].x;
        mesh.vertices[i * 3 + 1] = vertices[i].y;
        mesh.vertices[i * 3 + 2] = vertices[i].z;
    }

    for (size_t i = 0; i < face_indices.size(); ++i) {
        mesh.indices[i] = face_indices[i];
    }
    
    mesh.normals = new float[mesh.vertexCount * 3]();

    CalculateNormals(&mesh);

    UploadMesh(&mesh, false);
    model = LoadModelFromMesh(mesh);

    // Clean up CPU memory
    // delete[] mesh.vertices;
    // delete[] mesh.indices;
    // delete[] mesh.normals;

    return model;
}

void CalculateNormals(Mesh *mesh)
{
    printf("Calculating normals...\n");
    for(int i = 0; i < mesh->triangleCount; i++) {
        unsigned short index1 = mesh->indices[i*3];
        unsigned short index2 = mesh->indices[i*3 + 1];
        unsigned short index3 = mesh->indices[i*3 + 2];

        Vector3 v1 = {mesh->vertices[index1*3], mesh->vertices[index1*3+1], mesh->vertices[index1*3+2]};
        Vector3 v2 = {mesh->vertices[index2*3], mesh->vertices[index2*3+1], mesh->vertices[index2*3+2]};
        Vector3 v3 = {mesh->vertices[index3*3], mesh->vertices[index3*3+1], mesh->vertices[index3*3+2]};

        Vector3 edge1 = {v2.x - v1.x, v2.y - v1.y, v2.z - v1.z};
        Vector3 edge2 = {v3.x - v1.x, v3.y - v1.y, v3.z - v1.z};

        Vector3 normal = {
            edge1.y * edge2.z - edge1.z * edge2.y,
            edge1.z * edge2.x - edge1.x * edge2.z,
            edge1.x * edge2.y - edge1.y * edge2.x
        };
        
        // Normalize the normal
        float length = sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
        if (length != 0) {
            normal.x /= length;
            normal.y /= length;
            normal.z /= length;
        }

        mesh->normals[index1*3] += normal.x;
        mesh->normals[index1*3+1] += normal.y;
        mesh->normals[index1*3+2] += normal.z;
        mesh->normals[index2*3] += normal.x;
        mesh->normals[index2*3+1] += normal.y;
        mesh->normals[index2*3+2] += normal.z;
        mesh->normals[index3*3] += normal.x;
        mesh->normals[index3*3+1] += normal.y;
        mesh->normals[index3*3+2] += normal.z;
    }
    
    // Normalize all the vertex normals
    for (int i=0; i < mesh->vertexCount; i++) {
        Vector3 normal = {mesh->normals[i*3], mesh->normals[i*3+1], mesh->normals[i*3+2]};
        float length = sqrt(normal.x*normal.x + normal.y*normal.y + normal.z*normal.z);
        if (length != 0) {
            mesh->normals[i*3] /= length;
            mesh->normals[i*3+1] /= length;
            mesh->normals[i*3+2] /= length;
        }
    }
}
#include <raylib.h>
#include <raymath.h>

int main() {
    InitWindow(600, 600, "Triangle");
    
    Mesh mesh = { 0 };

    mesh.vertexCount   = 3;
    mesh.triangleCount = 1;

    mesh.vertices = (float *)MemAlloc(mesh.vertexCount*3*sizeof(float));

    mesh.vertices[0] = 0.0f;
    mesh.vertices[1] = 1.0f;
    mesh.vertices[2] = 0.0f;

    mesh.vertices[3] = -1.0f;
    mesh.vertices[4] = -1.0f;
    mesh.vertices[5] = 0.0f;

    mesh.vertices[6] = 1.0f;
    mesh.vertices[7] = -1.0f;
    mesh.vertices[8] = 0.0f;

    UploadMesh(&mesh, false);  

    Shader shader = LoadShader("./shaders/vertex.glsl", "./shaders/fragment.glsl");
    Material mat = LoadMaterialDefault();
    mat.shader = shader;
    
    Camera3D cam = {0};
    cam.position = (Vector3){ 0, 0, 1 };
    cam.target   = (Vector3){ 0, 0, 0 };
    cam.up       = (Vector3){ 0, 1, 0 };
    cam.fovy     = 45;
    cam.projection = CAMERA_PERSPECTIVE;

    while(!WindowShouldClose()) {
        BeginDrawing();

        BeginMode3D(cam);
        
        DrawMesh(mesh, mat, MatrixIdentity());

        EndMode3D();

        EndDrawing();
    }
}
#ifndef FREE_CAMERA_H
#define FREE_CAMERA_H

#include "raylib.h"
#include "raymath.h"

// Custom camera system for a free-moving camera
class FreeCamera {
public:
    // Constructor
    FreeCamera(Vector3 position = { 0.0f, 10.0f, 10.0f },
               Vector3 target = { 0.0f, 0.0f, 0.0f },
               Vector3 up = { 0.0f, 1.0f, 0.0f },
               float fovy = 45.0f,
               float movementSpeed = 0.1f,
               float mouseSensitivity = 0.05f);

    // Update camera position and orientation based on input
    void Update(float deltaTime);

    // Get the raylib Camera3D object
    Camera3D GetCamera3D() const;

    // Setters for camera parameters
    void SetPosition(Vector3 position);
    void SetTarget(Vector3 target);
    void SetUp(Vector3 up);
    void SetFovY(float fovy);

private:
    Camera3D camera;
    Vector3 front;
    Vector3 up; // Renamed to avoid conflict with `up` (worldUp)
    Vector3 right;
    Vector3 worldUp;

    float yaw;
    float pitch;

    float movementSpeed;
    float mouseSensitivity;

    void updateCameraVectors();
};

#endif // FREE_CAMERA_H

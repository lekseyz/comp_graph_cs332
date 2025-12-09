#include "free_camera.h"
#include <cmath>

FreeCamera::FreeCamera(Vector3 position, Vector3 target, Vector3 up, float fovy,
                       float movementSpeed, float mouseSensitivity)
    : worldUp(up), movementSpeed(movementSpeed),
      mouseSensitivity(mouseSensitivity) {

  camera.position = position;
  camera.target = target; // This will be updated by updateCameraVectors
  up = worldUp;           // Initialize with worldUp
  camera.fovy = fovy;
  camera.projection = CAMERA_PERSPECTIVE;

  Vector3 direction = Vector3Normalize(Vector3Subtract(target, position));
  yaw = atan2f(direction.z, direction.x) * RAD2DEG;
  pitch = asinf(direction.y) * RAD2DEG;

  if (yaw < 0)
    yaw += 360.0f;
  pitch = Clamp(pitch, -89.0f, 89.0f);

  updateCameraVectors();
}

void FreeCamera::Update(float deltaTime) {
  float currentMovementSpeed =
      movementSpeed * deltaTime * 100.0f; // Scale by 100 for better speed

  if (IsKeyDown(KEY_W))
    camera.position =
        Vector3Add(camera.position, Vector3Scale(front, currentMovementSpeed));
  if (IsKeyDown(KEY_S))
    camera.position = Vector3Subtract(
        camera.position, Vector3Scale(front, currentMovementSpeed));
  if (IsKeyDown(KEY_A))
    camera.position = Vector3Subtract(
        camera.position, Vector3Scale(right, currentMovementSpeed));
  if (IsKeyDown(KEY_D))
    camera.position =
        Vector3Add(camera.position, Vector3Scale(right, currentMovementSpeed));
  if (IsKeyDown(KEY_SPACE))
    camera.position =
        Vector3Add(camera.position,
                   Vector3Scale(worldUp, currentMovementSpeed)); // Move up
  if (IsKeyDown(KEY_LEFT_CONTROL))
    camera.position = Vector3Subtract(
        camera.position,
        Vector3Scale(worldUp, currentMovementSpeed)); // Move down

  Vector2 mouseDelta = GetMouseDelta();
  yaw += mouseDelta.x * mouseSensitivity;
  pitch -= mouseDelta.y * mouseSensitivity; // Invert Y for typical FPS controls

  pitch = Clamp(pitch, -89.0f, 89.0f);

  updateCameraVectors();
}

void FreeCamera::updateCameraVectors() {
  front.x = cosf(DEG2RAD * yaw) * cosf(DEG2RAD * pitch);
  front.y = sinf(DEG2RAD * pitch);
  front.z = sinf(DEG2RAD * yaw) * cosf(DEG2RAD * pitch);
  front = Vector3Normalize(front);

  right = Vector3Normalize(Vector3CrossProduct(front, worldUp));
  up = Vector3Normalize(Vector3CrossProduct(right, front));
  camera.target = Vector3Add(camera.position, front);
  camera.up = up;
}

Camera3D FreeCamera::GetCamera3D() const { return camera; }

void FreeCamera::SetPosition(Vector3 position) {
  camera.position = position;
  updateCameraVectors();
}

void FreeCamera::SetTarget(Vector3 target) {
  camera.target = target;
  Vector3 direction =
      Vector3Normalize(Vector3Subtract(target, camera.position));
  yaw = atan2f(direction.z, direction.x) * RAD2DEG;
  pitch = asinf(direction.y) * RAD2DEG;
  if (yaw < 0)
    yaw += 360.0f;
  pitch = Clamp(pitch, -89.0f, 89.0f);
  updateCameraVectors();
}

void FreeCamera::SetUp(Vector3 up) {
  worldUp = up;
  updateCameraVectors();
}

void FreeCamera::SetFovY(float fovy) { camera.fovy = fovy; }

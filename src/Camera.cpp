#include "Camera.h"
#include "PlayerDimensions.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
glm::vec3 Camera::lookDirection() const {
    float yaw = glm::radians(yaw3D), pitch = glm::radians(firstPersonPitch);
    return glm::vec3(-std::sin(yaw) * std::cos(pitch), std::sin(pitch),
                     -std::cos(yaw) * std::cos(pitch));
}
glm::vec3 Camera::focusPosition() const {
    // Raise third-person framing slightly above Toshko, independently of 2D.
    return target + ((mode == CameraMode::ThreeD && !isFirstPerson()) ? glm::vec3(0.0f, 0.6f, 0.0f)
                                                                      : glm::vec3(0.0f));
}
glm::vec3 Camera::getPosition() const {
    if (isFirstPerson())
        return glm::vec3(
            target.x, target.y - PlayerDimensions::CenterAboveFeet + PlayerDimensions::EyeAboveFeet,
            target.z);
    float yaw = glm::radians(mode == CameraMode::TwoD ? 0.0f : yaw3D);
    float pitch = glm::radians(mode == CameraMode::TwoD ? pitch2D : pitch3D);
    float distance = mode == CameraMode::TwoD ? distance2D : distance3D;
    return focusPosition() + distance * glm::vec3(std::cos(pitch) * std::sin(yaw), std::sin(pitch),
                                                  std::cos(pitch) * std::cos(yaw));
}
glm::mat4 Camera::getViewMatrix() const {
    auto eye = getPosition();
    return glm::lookAt(eye, isFirstPerson() ? eye + lookDirection() : focusPosition(),
                       glm::vec3(0, 1, 0));
}
glm::mat4 Camera::getProjectionMatrix(float aspect) const {
    aspect = std::max(aspect, 0.01f);
    if (mode == CameraMode::ThreeD)
        return glm::perspective(glm::radians(isFirstPerson() ? firstPersonFov : 55.0f), aspect,
                                0.05f, 200.0f);
    float halfHeight = distance2D * std::tan(glm::radians(22.5f));
    return glm::ortho(-halfHeight * aspect, halfHeight * aspect, -halfHeight, halfHeight, 0.1f,
                      150.0f);
}
glm::vec3 Camera::getForward() const {
    float yaw = glm::radians(mode == CameraMode::TwoD ? 0.0f : yaw3D);
    return glm::vec3(-std::sin(yaw), 0, -std::cos(yaw));
}
glm::vec3 Camera::getRight() const {
    return glm::cross(getForward(), glm::vec3(0, 1, 0));
}
void Camera::rotate(float dx, float dy) {
    if (mode == CameraMode::TwoD) {
        pitch2D = std::clamp(pitch2D - dy * 0.06f, 30.0f, 40.0f);
        return;
    }
    // Fixed default mouse directions; keep yaw bounded across repeated turns.
    yaw3D = std::remainder(yaw3D - dx * 0.15f, 360.0f);
    if (isFirstPerson())
        firstPersonPitch = std::clamp(firstPersonPitch - dy * 0.15f, -85.0f, 85.0f);
    else
        pitch3D = std::clamp(pitch3D - dy * 0.15f, 5.0f, 25.0f);
}
void Camera::zoom(float amount) {
    if (mode == CameraMode::TwoD)
        distance2D = std::clamp(distance2D - amount, 5.0f, 18.0f);
    else if (isFirstPerson())
        firstPersonFov = std::clamp(firstPersonFov - amount * 2.0f, 55.0f, 80.0f);
    else
        distance3D = std::clamp(distance3D - amount, 2.5f, 15.0f);
}
void Camera::setTarget(const glm::vec3& position) {
    target = position;
    target.y = std::max(target.y, 0.5f);
}

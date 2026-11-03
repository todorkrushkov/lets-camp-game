#pragma once
#include <glm/glm.hpp>
enum class CameraMode { TwoD, ThreeD };
enum class Perspective { ThirdPerson, FirstPerson };
class Camera {
  public:
    Camera() = default;
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;
    glm::vec3 getPosition() const;
    glm::vec3 getForward() const;
    glm::vec3 getRight() const;
    void rotate(float deltaX, float deltaY);
    void zoom(float amount);
    void setTarget(const glm::vec3& position);
    void setMode(CameraMode value) {
        mode = value;
    }
    void setPerspective(Perspective value) {
        perspective = value;
    }
    CameraMode getMode() const {
        return mode;
    }
    Perspective getPerspective() const {
        return perspective;
    }
    bool isFirstPerson() const {
        return mode == CameraMode::ThreeD && perspective == Perspective::FirstPerson;
    }

  private:
    glm::vec3 target{0.0f, 0.5f, 0.0f};
    CameraMode mode = CameraMode::TwoD;
    Perspective perspective = Perspective::ThirdPerson;
    float pitch2D = 35.0f, distance2D = 5.0f;
    float yaw3D = 0.0f, pitch3D = 25.0f, distance3D = 8.0f;
    float firstPersonPitch = 0.0f, firstPersonFov = 70.0f;
    glm::vec3 lookDirection() const;
    glm::vec3 focusPosition() const;
};

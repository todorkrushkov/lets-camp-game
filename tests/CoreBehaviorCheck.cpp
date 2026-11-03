#include "Animation.h"
#include "Camera.h"
#include "PauseMenuLayout.h"
#include "Player.h"
#include "PlayerDimensions.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
bool close(float a, float b) {
    return std::abs(a - b) < 0.0001f;
}
float orbitPitch(const Camera& camera, glm::vec3 focus) {
    auto offset = camera.getPosition() - focus;
    return glm::degrees(std::atan2(offset.y, glm::length(glm::vec2(offset.x, offset.z))));
}
} // namespace

int main() {
    try {
        Camera camera;
        require(close(camera.getProjectionMatrix(16.0f / 9)[3][3], 1),
                "2D must use orthographic projection");
        camera.setTarget({2, .52f, 5});
        camera.setMode(CameraMode::ThreeD);
        const glm::vec3 focus(2, 1.12f, 5);
        require(close(camera.getProjectionMatrix(16.0f / 9)[3][3], 0),
                "3D must use perspective projection");
        camera.rotate(0, -10000);
        require(close(orbitPitch(camera, focus), 25), "Third-person pitch upper limit changed");
        camera.rotate(0, 10000);
        require(close(orbitPitch(camera, focus), 5), "Third-person pitch lower limit changed");
        camera.zoom(10000);
        require(close(glm::length(camera.getPosition() - focus), 2.5f),
                "Closest orbit distance changed");
        camera.zoom(-10000);
        require(close(glm::length(camera.getPosition() - focus), 15),
                "Farthest orbit distance changed");
        camera.setPerspective(Perspective::FirstPerson);
        require(camera.isFirstPerson(), "First-person selection failed");
        require(close(camera.getPosition().y, .02f + PlayerDimensions::EyeAboveFeet),
                "Eye height detached from player size");
        auto forward = camera.getForward();
        camera.rotate(360 / .15f, 0);
        require(glm::length(camera.getForward() - forward) < .0001f,
                "Full mouse turn must preserve movement direction");
        camera.setMode(CameraMode::TwoD);
        require(!camera.isFirstPerson(), "2D must not hide the player as first person");
        require(glm::length(camera.getForward() - glm::vec3(0, 0, -1)) < .0001f,
                "2D movement must remain fixed to map axes");

        Player player;
        auto start = player.getPosition();
        player.move({0, 0, -1}, .25f);
        require(close(player.getPosition().z, start.z - 1),
                "Player speed must remain four units per second");
        require(close(player.getPosition().y, start.y), "Planar movement must preserve height");

        Animator animation;
        auto walk = ToshkoAnimations::walk(Direction::Down);
        animation.play(walk);
        animation.update(.13f);
        animation.play(walk);
        require(animation.getColumn() == 1, "Repeated walk selection must not reset animation");
        animation.update(0);
        require(animation.getColumn() == 1, "Zero delta must freeze animation");
        animation.play(ToshkoAnimations::idle(Direction::Left));
        require(animation.getRow() == 1 && animation.getColumn() == 0,
                "Direction change must select the correct idle frame");

        for (auto size : {glm::ivec2(1280, 720), glm::ivec2(320, 240)}) {
            PauseMenuLayout menu(size.x, size.y);
            for (int row = 0; row < 3; ++row)
                require(menu.hit(menu.x + menu.width * .5f, menu.rowY(row) + 24 * menu.scale) ==
                            static_cast<MenuAction>(row),
                        "Scaled menu hit areas must match rendered rows");
            require(menu.hit(0, 0) == MenuAction::None,
                    "Outside clicks must not activate the menu");
        }
        std::cout << "PASS: camera modes/limits, player movement, animation continuity and scaled "
                     "menu input.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

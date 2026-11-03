#include "Game.h"

void Game::enterInterior() {
    outdoorPosition = player.getPosition();
    outdoorCamera = camera;
    indoors = true;
    player.setPosition(InteriorRoom::start());
    camera = Camera();
    camera.setMode(CameraMode::ThreeD);
    camera.setPerspective(Perspective::FirstPerson);
    mouse.firstMovement = true;
}

void Game::leaveInterior() {
    indoors = false;
    player.setPosition(outdoorPosition);
    camera = outdoorCamera;
    // Require leaving the trigger before another entry can be armed.
    doorArmed = false;
    mouse.firstMovement = true;
}

void Game::movePlayer(const glm::vec3& movement, float movementDelta) {
    const auto previousPosition = player.getPosition();
    player.move(movement, movementDelta);
    if (indoors) {
        player.setPosition(InteriorRoom::move(previousPosition, player.getPosition()));
        if (InteriorRoom::atExit(player.getPosition()))
            leaveInterior();
    } else if (model) {
        player.setPosition(model->resolveMovement(previousPosition, player.getPosition()));
        const bool nearDoor = model->nearDoor(player.getPosition());
        if (!nearDoor)
            doorArmed = true;
        if (nearDoor && doorArmed && moving)
            enterInterior();
    }
}

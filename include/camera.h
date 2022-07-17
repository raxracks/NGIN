#pragma once
#include "raylib.h"
#include "raymath.h"
#include "mouse.h"

#define PLAYER_MOVEMENT_SENSITIVITY 5


Vector3 playerRotation = (Vector3){0, -1025 * DEG2RAD, 0};

float steps = 0;

void UpdateGameCamera(Camera *camera, Vector3 *playerPos, float deltaTime)
{
    bool direction[4] = {IsKeyDown(KEY_W),
                         IsKeyDown(KEY_S),
                         IsKeyDown(KEY_D),
                         IsKeyDown(KEY_A)};

    if(direction[0] || direction[1] || direction[2] ||direction[3]) steps += deltaTime * 10;
    
    playerPos->x += (sinf(playerRotation.x) * direction[1] -
                     sinf(playerRotation.x) * direction[0] -
                     cosf(playerRotation.x) * direction[3] +
                     cosf(playerRotation.x) * direction[2]) *
                    (deltaTime * PLAYER_MOVEMENT_SENSITIVITY);

    playerPos->z += (cosf(playerRotation.x) * direction[1] -
                     cosf(playerRotation.x) * direction[0] +
                     sinf(playerRotation.x) * direction[3] -
                     sinf(playerRotation.x) * direction[2]) *
                    (deltaTime * PLAYER_MOVEMENT_SENSITIVITY);

    camera->position.x = playerPos->x;

    // Note: place camera on top of player's head.
    camera->position.y = playerPos->y;
    camera->position.z = playerPos->z;

    playerRotation.y -= GetMouseMovement().y;
    playerRotation.x -= GetMouseMovement().x;

    if (playerRotation.y < -20.35)
    {
        playerRotation.y = -20.35;
    }

    if (playerRotation.y > -17.30)
    {
        playerRotation.y = -17.30;
    }

    Matrix translation = MatrixTranslate(0, 0, 1);
    Matrix rotation = MatrixRotateXYZ(Vector3{PI * 2 - playerRotation.y, PI * 2 - playerRotation.x, 0});
    Matrix transform = MatrixMultiply(translation, rotation);

    camera->target.x = camera->position.x - transform.m12;
    camera->target.y = camera->position.y - transform.m13;
    camera->target.z = camera->position.z - transform.m14;
}
#pragma once
#include "raylib.h"
#include "raymath.h"

#define SMOOTHNESS 0.001f
#define SENSITIVITY 2.0f

static Vector2 PreviousMousePosition = {0};
static Vector2 MouseMovement = {0};

void UpdateMouseMovement()
{
    Vector2 CurrentMousePosition = GetMousePosition();
    MouseMovement = Vector2Multiply(
        Vector2Subtract(CurrentMousePosition, PreviousMousePosition),
        Vector2{SMOOTHNESS * SENSITIVITY, SMOOTHNESS * SENSITIVITY});

    PreviousMousePosition = CurrentMousePosition;
}

Vector2 GetMouseMovement()
{
    return MouseMovement;
}
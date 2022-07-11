#include "camera.h"
#include <vector>
#include <string>
#include <raylib.h>

// #define DEBUG

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION 330
#else // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB
#define GLSL_VERSION 100
#endif

#define RENDER_DISTANCE 22

struct Wall
{
    float y = 0.0f;
    float height = 0.0f;
    float width = 2.0f;
    float depth = 2.0f;
    int modelIndex;
};

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1366;
    const int screenHeight = 768;
    const int mapWidth = 100;
    const int mapHeight = 100;

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_FULLSCREEN_MODE);
    InitWindow(screenWidth, screenHeight, "NGIN");

    // Define the camera to look into our 3d world (position, target, up vector)
    Camera camera = {0};
    camera.position = Vector3{4.0f, 2.0f, 4.0f};
    camera.target = Vector3{0.0f, 2.0f, 0.0f};
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Generates some random columns
    std::vector<std::vector<Wall>> walls;

    Shader shader = LoadShader(TextFormat("assets/shaders/glsl%i/base_lighting.vs", GLSL_VERSION),
                               TextFormat("assets/shaders/glsl%i/fog.fs", GLSL_VERSION));
    shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader, "matModel");
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

    int ambientLoc = GetShaderLocation(shader, "ambient");
    float ambientLevel[4] = {0.3f, 0.3f, 0.3f, 1.0f};
    SetShaderValue(shader, ambientLoc, ambientLevel, SHADER_UNIFORM_VEC4);

    float fogDensity = 0.06f;
#ifdef DEBUG
    fogDensity = 0;
#endif
    int fogDensityLoc = GetShaderLocation(shader, "fogDensity");
    SetShaderValue(shader, fogDensityLoc, &fogDensity, SHADER_UNIFORM_FLOAT);

    std::vector<Texture2D> textures = {LoadTexture("assets/wall.png"), LoadTexture("assets/wall1.png"), LoadTexture("assets/wall2.png")};
    std::vector<Model> models = {};
    std::vector<Model> lowLODModels = {};
    std::vector<Model> planes = {};

    for (int i = 0; i < textures.size(); i++)
    {
        Model rect = LoadModelFromMesh(GenMeshCube(1, 1, 1));

        rect.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = textures[i];
        rect.materials[0].shader = shader;

        models.push_back(rect);

        Model lowRect = LoadModelFromMesh(GenMeshCube(1, 1, 1));
        lowLODModels.push_back(lowRect);

        Model plane = LoadModelFromMesh(GenMeshPlane(1, 1, 1, 1));

        plane.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = textures[i];
        plane.materials[0].shader = shader;

        planes.push_back(plane);
    }

    for (int x = 0; x < mapWidth; x++)
    {
        std::vector<Wall> line;

        for (int y = 0; y < mapHeight; y++)
        {
            if (GetRandomValue(1, 12) > 11)
            {
                Wall wall = {0, GetRandomValue(1, 100) / 10.0f, 2, 2, GetRandomValue(0, 2)};
                line.push_back(wall);
            }
            else
            {
                Wall wall;
                wall.modelIndex = GetRandomValue(0, 2);
                line.push_back(wall);
            }
        }

        walls.push_back(line);
    }

    // CreateLight(LIGHT_POINT, (Vector3){ 0, 2, 6 }, Vector3Zero(), WHITE, shader);

#ifndef DEBUG
    SetCameraMode(camera, CAMERA_FIRST_PERSON); // Set a first person camera mode
#endif

#ifdef DEBUG
    SetCameraMode(camera, CAMERA_FREE); // Set a first person camera mode
#endif

    Texture2D skybox = LoadTexture("assets/skybox.png");
    Texture2D shotgun = LoadTexture("assets/shotgun.png");

    Vector3 playerPosition = Vector3{4.0f, 2.0f, 4.0f};
    Vector3 playerPosNoCollide = Vector3{0, 0, 0};

    float maxPlayerPosY = playerPosition.y;

    // SetTargetFPS(60);                           // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        playerPosNoCollide = playerPosition;

        // Update
        //----------------------------------------------------------------------------------
        UpdateMouseMovement();
        UpdateGameCamera(&camera, &playerPosition);

        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], &camera.position.x, SHADER_UNIFORM_VEC3);

// if(playerPosition.x > 0) playerPosition.x = -6.28f;
#ifdef DEBUG
        UpdateCamera(&camera);
#endif

        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(DARKGRAY);

        // DrawTexturePro(skybox, Rectangle{0, 0, skybox.width, skybox.height}, Rectangle{0, 0, GetScreenWidth(), GetScreenHeight()}, Vector2{0, 0}, 0, DARKGRAY);

        BeginMode3D(camera);

        // Draw some cubes around
        // for(int x = 0; x < mapWidth; x++) {
        // for(int y = 0; y < mapHeight; y++) {
        for (int i = 0; i < mapWidth * mapHeight; i++)
        {
            const int x = i / mapWidth;
            const int y = i % mapHeight;
            Wall wall = walls[x][y];
            const int offsetX = x * 2;
            const int offsetY = y * 2;
            bool noCollide = false;

            if (abs(offsetX - playerPosition.x) > RENDER_DISTANCE || abs(offsetY - playerPosition.z) > RENDER_DISTANCE)
                continue;

            if (abs(offsetX - playerPosition.x) > RENDER_DISTANCE / 1.8f || abs(offsetY - playerPosition.z) > RENDER_DISTANCE / 1.8f)
            {
                if (wall.height != 0)
                    DrawModelEx(lowLODModels[wall.modelIndex], Vector3{offsetX, (wall.height / 2) + wall.y, offsetY}, Vector3{0, 0, 0}, 0.0f, Vector3{wall.width, wall.height, wall.depth}, DARKGRAY);
                else
                    DrawModelEx(planes[wall.modelIndex], Vector3{offsetX, (wall.height / 2) + wall.y, offsetY}, Vector3{0, 0, 0}, 0.0f, Vector3{wall.width, wall.height, wall.depth}, WHITE);

                noCollide = true;
            }
            else
                DrawModelEx(models[wall.modelIndex], Vector3{offsetX, (wall.height / 2) + wall.y, offsetY}, Vector3{0, 0, 0}, 0.0f, Vector3{wall.width, wall.height, wall.depth}, WHITE);

#ifdef DEBUG
            if (!noCollide)
            {
                DrawBoundingBox(BoundingBox{Vector3{playerPosition.x - 0.2f, playerPosition.y - 2, playerPosition.z - 0.2f}, Vector3{playerPosition.x + 0.2f, playerPosition.y, playerPosition.z + 0.2f}}, RED);
                DrawBoundingBox(BoundingBox{Vector3{offsetX - 1, wall.y, offsetY - 1}, Vector3{offsetX + 1, wall.y + wall.height, offsetY + 1}}, GREEN);
            }
#endif

            if (!noCollide && CheckCollisionBoxes(
                                  BoundingBox{
                                      Vector3{playerPosition.x - 0.2f, playerPosition.y - 2, playerPosition.z - 0.2f},
                                      Vector3{playerPosition.x + 0.2f, playerPosition.y, playerPosition.z + 0.2f}},
                                  BoundingBox{
                                      Vector3{offsetX - 1, wall.y, offsetY - 1},
                                      Vector3{offsetX + 1, wall.y + wall.height, offsetY + 1}}))
            {

                if (wall.height == 0)
                    maxPlayerPosY = wall.y + 2;

                if ((wall.y + wall.height) - playerPosition.y < 0)
                {
                    playerPosition.y = wall.y + wall.height + 2;
                    if (playerPosition.y > maxPlayerPosY)
                        maxPlayerPosY = playerPosition.y;
                    // playerPosNoCollide = playerPosition;
                }
                else
                {
                    playerPosition = playerPosNoCollide;
                    playerPosition.y = maxPlayerPosY;
                }
            }
            else
            {
                playerPosition.y -= 0.00025f;
                // playerPosNoCollide = playerPosition;
            }
        }
        // }

        EndMode3D();

        DrawTexturePro(shotgun, Rectangle{0, 0, shotgun.width, shotgun.height}, Rectangle{0, 0, GetScreenHeight() / 2, GetScreenHeight() / 2}, Vector2{-((GetScreenHeight() / 2) - sin(steps) * 10) - (GetScreenHeight() / 8), -(((GetScreenHeight() / 2) - cos(sin(-steps) * 2) * 10) + 9)}, 0, WHITE);

        DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
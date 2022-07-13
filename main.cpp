#include "camera.h"
#include <vector>
#include <string>
#include <raylib.h>

//#define DEBUG
#define DEV

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#if defined(PLATFORM_DESKTOP)
#define GLSL_VERSION 330
#else // PLATFORM_RPI, PLATFORM_ANDROID, PLATFORM_WEB
#define GLSL_VERSION 100
#endif

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

struct Wall
{
    float y = 0.0f;
    float height = 0.0f;
    float width = 2.0f;
    float depth = 2.0f;
    float roofOffset = 0.0f;
    float roofHeight = 0.0f;
    int modelIndex;
    int roofIndex;
};

int mapSize = 32;
int wallHeight = 5;
int roofHeight = 0;
int roofOffset = 0;
int wallTexture = 0;
int roofTexture = 0;
int RENDER_DISTANCE = 22;

std::vector<Wall> walls;

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1366;
    const int screenHeight = 768;
    int weaponFrame = 0;
    float weaponFrameTimerCount = 0;
    bool playingAnimation = false;
    bool collidedLastFrame = false;
    bool editing = false;

    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "NGIN");

    std::vector<Texture2D> textures = {LoadTexture("assets/wall1.png"), LoadTexture("assets/floor.png")};
    std::vector<Model> models = {};
    std::vector<Model> lowLODModels = {};
    std::vector<Model> planes = {};

    // std::vector<std::vector<Wall>> walls;

    // Define the camera to look into our 3d world (position, target, up vector)
    Camera camera = {0};
    camera.position = Vector3{4.0f, 2.0f, 4.0f};
    camera.target = Vector3{0.0f, 2.0f, 0.0f};
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 90.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    Camera2D editorCamera = {0};
    editorCamera.target = (Vector2){0, 0};
    editorCamera.offset = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f};
    editorCamera.rotation = 0.0f;
    editorCamera.zoom = 15.0f;

    // Add fog
    Shader shader = LoadShader(TextFormat("assets/shaders/glsl%i/base_lighting.vs", GLSL_VERSION),
                               TextFormat("assets/shaders/glsl%i/fog.fs", GLSL_VERSION));
    shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader, "matModel");
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

    int ambientLoc = GetShaderLocation(shader, "ambient");
    float ambientLevel[4] = {0.3f, 0.3f, 0.3f, 1.0f};
    SetShaderValue(shader, ambientLoc, ambientLevel, SHADER_UNIFORM_VEC4);

    float fogDensity = 0.1f;

#ifdef DEBUG
    fogDensity = 0;
#endif

    int fogDensityLoc = GetShaderLocation(shader, "fogDensity");
    SetShaderValue(shader, fogDensityLoc, &fogDensity, SHADER_UNIFORM_FLOAT);

    for (int i = 0; i < textures.size(); i++)
    {
        Model model = LoadModelFromMesh(GenMeshCube(1, 1, 1));

        model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = textures[i];
        model.materials[0].shader = shader;

        models.push_back(model);

        Model lowModel = LoadModelFromMesh(GenMeshCube(1, 1, 1));
        lowLODModels.push_back(lowModel);

        Model plane = LoadModelFromMesh(GenMeshPlane(1, 1, 1, 1));

        plane.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = textures[i];
        plane.materials[0].shader = shader;

        planes.push_back(plane);
    }

    for (int i = 0; i < mapSize * mapSize; i++)
    {
        walls.push_back(Wall{0, 0, 2, 2, 0, 0, 1, 0});
    }

    // CreateLight(LIGHT_POINT, (Vector3){ 0, 2, 6 }, Vector3Zero(), WHITE, shader);

#ifndef DEBUG
    SetCameraMode(camera, CAMERA_FIRST_PERSON); // Set a first person camera mode
#else
    SetCameraMode(camera, CAMERA_FREE); // Set a first person camera mode
#endif

    Texture2D skybox = LoadTexture("assets/skybox.png");
    Texture2D shotgun = LoadTexture("assets/shotgun.png");

    Vector3 playerPosition = Vector3{4.0f, 2.0f, 4.0f};
    Vector3 playerPosNoCollide = Vector3{0, 0, 0};

    float maxPlayerPosY = playerPosition.y;

    // SetTargetFPS(60);                           // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    bool editWallHeight = false;
    bool editRoofHeight = false;
    bool editRoofOffset = false;
    bool editWallTexture = false;
    bool editRoofTexture = false;
    // bool editWallHeight = false;

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        playerPosNoCollide = playerPosition;

        const float deltaTime = GetFrameTime();

        // Update
        //----------------------------------------------------------------------------------
        UpdateMouseMovement();
        UpdateGameCamera(&camera, &playerPosition, deltaTime);

        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], &camera.position.x, SHADER_UNIFORM_VEC3);

        if (IsMouseButtonPressed(0))
            playingAnimation = true;

        const int wx = GetScreenWidth();
        const int wy = GetScreenHeight();

#ifdef DEBUG
        UpdateCamera(&camera);
#endif

        if (IsKeyPressed(KEY_P))
            editing = !editing;

        if (editing)
        {
            editorCamera.target = Vector2{-mapSize, -mapSize};

            EnableCursor();

            if (IsKeyPressed(KEY_EQUAL))
            {
                wallHeight++;
            }

            if (IsKeyPressed(KEY_MINUS))
            {
                wallHeight--;
            }

            if (IsKeyPressed(KEY_LEFT_BRACKET))
            {
                roofOffset--;
            }

            if (IsKeyPressed(KEY_RIGHT_BRACKET))
            {
                roofOffset++;
            }

            if (IsKeyDown(KEY_RIGHT))
                editorCamera.offset.x -= 5;
            else if (IsKeyDown(KEY_LEFT))
                editorCamera.offset.x += 5;
            if (IsKeyDown(KEY_UP))
                editorCamera.offset.y += 5;
            else if (IsKeyDown(KEY_DOWN))
                editorCamera.offset.y -= 5;

            editorCamera.zoom += ((float)GetMouseWheelMove() * 0.5f);

            if (editorCamera.zoom < 0.1f)
                editorCamera.zoom = 0.1f;

            if (IsKeyPressed(KEY_R))
            {
                editorCamera.zoom = 1.0f;
                editorCamera.rotation = 0.0f;
            }
        }
        else
        {
            DisableCursor();
        }

        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        if (editing)
        {
            ClearBackground(BLACK);

            BeginMode2D(editorCamera);

            for (int i = 0; i < mapSize * mapSize; i++)
            {
                Wall wall = walls[i];

                const int x = i / mapSize;
                const int y = i % mapSize;

                const int offsetX = x * 2;
                const int offsetY = y * 2;

                const Texture2D texture = textures[wall.modelIndex];

                if (wall.roofHeight != 0)
                {
                    DrawTexturePro(texture, Rectangle{0, 0, texture.width, texture.height}, Rectangle{0, 0, 1, 2}, Vector2{offsetX, offsetY}, 0.0f, WHITE);

                    const Texture2D roofTexture = textures[wall.roofIndex];
                    DrawTexturePro(roofTexture, Rectangle{0, 0, roofTexture.width, roofTexture.height}, Rectangle{0, 0, 1, 2}, Vector2{offsetX - 1, offsetY}, 0.0f, WHITE);
                }
                else
                    DrawTexturePro(texture, Rectangle{0, 0, texture.width, texture.height}, Rectangle{0, 0, 2, 2}, Vector2{offsetX, offsetY}, 0.0f, WHITE);

                DrawRectangleLinesEx(Rectangle{-offsetX, -offsetY, 2, 2}, 0.035f, BLACK);

                DrawTextEx(GetFontDefault(), (std::to_string((int)wall.height)).c_str(), Vector2{-offsetX + 0.1f, -offsetY + 0.1f}, 0.8f, 0.8f / 10, RAYWHITE);

                if (IsMouseButtonDown(0) && GetMousePosition().x < wx - 130 || IsMouseButtonDown(1) && GetMousePosition().x < wx - 130)
                {
                    Vector2 p = GetScreenToWorld2D(GetMousePosition(), editorCamera);
                    Vector2 m = Vector2{floor((-p.x + 2)), floor((-p.y + 2))};

                    if (m.x > offsetX - 1 && m.x < offsetX + 2 &&
                        m.y > offsetY - 1 && m.y < offsetY + 2)
                    {
                        if (IsMouseButtonDown(0))
                            walls[i] = Wall{0, wallHeight, 2, 2, roofOffset, roofHeight, wallTexture, roofTexture};
                        else if (IsMouseButtonDown(1))
                            walls[i] = Wall{0, 0, 2, 2, 0, 0, 1};
                    }
                }
            }

            DrawCircle(-playerPosition.x, -playerPosition.z, 0.45f, GREEN);

            EndMode2D();

            DrawRectangle(wx - 220, 0, 220, wy, LIGHTGRAY);
            if (GuiValueBox((Rectangle){wx - 110, 10, 100, 30}, "Wall Height", &wallHeight, 0, 100, editWallHeight))
                editWallHeight = !editWallHeight;
            if (GuiValueBox((Rectangle){wx - 110, 40, 100, 30}, "Roof Height", &roofHeight, 0, 100, editRoofHeight))
                editRoofHeight = !editRoofHeight;
            if (GuiValueBox((Rectangle){wx - 110, 70, 100, 30}, "Roof Offset", &roofOffset, 0, 100, editRoofOffset))
                editRoofOffset = !editRoofOffset;
            if (GuiValueBox((Rectangle){wx - 110, 100, 100, 30}, "Wall Texture", &wallTexture, 0, 100, editWallTexture))
                editWallTexture = !editWallTexture;
            if (GuiValueBox((Rectangle){wx - 110, 130, 100, 30}, "Roof Texture", &roofTexture, 0, 100, editRoofTexture))
                editRoofTexture = !editRoofTexture;

            // wallHeight = GuiSliderBar((Rectangle){ wx - 130, 10, 100, 20}, "Wall Height", std::to_string(wallHeight).c_str(), wallHeight, 0, 15);
            // roofHeight = GuiSliderBar((Rectangle){ wx - 130, 30, 100, 20}, "Roof Height", std::to_string(roofHeight).c_str(), roofHeight, 0, 15);
            // roofOffset = GuiSliderBar((Rectangle){ wx - 130, 50, 100, 20}, "Roof Offset", std::to_string(roofOffset).c_str(), roofOffset, 0, 15);
            // wallTexture = GuiSliderBar((Rectangle){ wx - 130, 70, 100, 20}, "Wall Texture", std::to_string(wallTexture).c_str(), wallTexture, 0, textures.size());
            // roofTexture = GuiSliderBar((Rectangle){ wx - 130, 160, 100, 20}, "Roof Texture", std::to_string(roofTexture).c_str(), roofTexture, 0, textures.size());

            // DrawTexturePro(textures[wallTexture], Rectangle{0, 0, textures[wallTexture].width, textures[wallTexture].height}, Rectangle{0, 0, 32, 32}, Vector2{130, 80}, 0.0f, WHITE);
            // DrawTexturePro(textures[roofTexture], Rectangle{0, 0, textures[roofTexture].width, textures[roofTexture].height}, Rectangle{0, 0, 32, 32}, Vector2{130, 170}, 0.0f, WHITE);
        }
        else
        {
            ClearBackground(Color{116, 115, 114, 255});

            BeginMode3D(camera);

            for (int i = 0; i < mapSize * mapSize; i++)
            {
                Wall wall = walls[i];

                const int x = i / mapSize;
                const int y = i % mapSize;

                const int offsetX = x * 2;
                const int offsetY = y * 2;

                bool noCollide = false;

                if (abs(offsetX - playerPosition.x) > RENDER_DISTANCE || abs(offsetY - playerPosition.z) > RENDER_DISTANCE)
                    continue;

                if (abs(offsetX - playerPosition.x) > RENDER_DISTANCE / 1.8f || abs(offsetY - playerPosition.z) > RENDER_DISTANCE / 1.8f)
                {
                    if (wall.height != 0)
                    {
                        DrawModelEx(lowLODModels[wall.modelIndex], Vector3{offsetX, (wall.height / 2) + wall.y, offsetY}, Vector3{0, 0, 0}, 0.0f, Vector3{wall.width, wall.height, wall.depth}, Color{116, 115, 114, 255});
                        if (wall.roofHeight != 0)
                            DrawModelEx(lowLODModels[wall.roofIndex], Vector3{offsetX, wall.y + wall.height + wall.roofOffset, offsetY}, Vector3{0, 0, 0}, 0.0f, Vector3{wall.width, wall.roofHeight, wall.depth}, Color{116, 115, 114, 255});
                    }
                    else
                        DrawModelEx(planes[wall.modelIndex], Vector3{offsetX, (wall.height / 2) + wall.y, offsetY}, Vector3{0, 0, 0}, 0.0f, Vector3{wall.width, wall.height, wall.depth}, WHITE);

                    noCollide = true;
                }
                else
                {
                    DrawModelEx(models[wall.modelIndex], Vector3{offsetX, (wall.height / 2) + wall.y, offsetY}, Vector3{0, 0, 0}, 0.0f, Vector3{wall.width, wall.height, wall.depth}, WHITE);
                    if (wall.roofHeight != 0)
                        DrawModelEx(models[wall.roofIndex], Vector3{offsetX, ((wall.roofHeight / 2) + wall.y) + wall.height + wall.roofOffset, offsetY}, Vector3{0, 0, 0}, 0.0f, Vector3{wall.width, wall.roofHeight, wall.depth}, Color{116, 115, 114, 255});
                }
#ifdef DEBUG
                if (!noCollide)
                {
                    DrawBoundingBox(BoundingBox{Vector3{playerPosition.x - 0.2f, playerPosition.y - 2, playerPosition.z - 0.2f}, Vector3{playerPosition.x + 0.2f, playerPosition.y, playerPosition.z + 0.2f}}, RED);
                    DrawBoundingBox(BoundingBox{Vector3{offsetX - 1, wall.y, offsetY - 1}, Vector3{offsetX + 1, wall.y + wall.height, offsetY + 1}}, GREEN);
                    DrawBoundingBox(BoundingBox{
                                        Vector3{offsetX - 1, wall.y + wall.height + wall.roofOffset, offsetY - 1},
                                        Vector3{offsetX + 1, wall.y + wall.height + wall.roofOffset + wall.roofHeight, offsetY + 1}},
                                    GREEN);
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
                    }
                    else
                    {
                        Vector3 difference = Vector3{0, 0, playerPosNoCollide.z - playerPosition.z};
                        Vector3 testPos = Vector3Add(playerPosition, difference);

                        if (CheckCollisionBoxes(
                                BoundingBox{
                                    Vector3{testPos.x - 0.2f, testPos.y - 2, testPos.z - 0.2f},
                                    Vector3{testPos.x + 0.2f, testPos.y, testPos.z + 0.2f}},
                                BoundingBox{
                                    Vector3{offsetX - 1, wall.y, offsetY - 1},
                                    Vector3{offsetX + 1, wall.y + wall.height, offsetY + 1}}))
                            difference = Vector3{playerPosNoCollide.x - playerPosition.x, 0, 0};

                        playerPosition = Vector3Add(playerPosition, difference);
                    }
                }

                if (!noCollide && wall.roofHeight != 0 && CheckCollisionBoxes(BoundingBox{Vector3{playerPosition.x - 0.2f, playerPosition.y - 2, playerPosition.z - 0.2f}, Vector3{playerPosition.x + 0.2f, playerPosition.y, playerPosition.z + 0.2f}}, BoundingBox{Vector3{offsetX - 1, wall.y + wall.height + wall.roofOffset, offsetY - 1}, Vector3{offsetX + 1, wall.y + wall.height + wall.roofOffset + wall.roofHeight, offsetY + 1}}))
                {
                    if ((wall.y + wall.height + wall.roofOffset + wall.roofHeight) - playerPosition.y < 0)
                    {
                        playerPosition.y = wall.y + wall.height + wall.roofOffset + wall.roofHeight + 2;
                    }
                    else
                    {
                        Vector3 difference = Vector3{0, 0, playerPosNoCollide.z - playerPosition.z};
                        Vector3 testPos = Vector3Add(playerPosition, difference);

                        if (CheckCollisionBoxes(
                                BoundingBox{
                                    Vector3{testPos.x - 0.2f, testPos.y - 2, testPos.z - 0.2f},
                                    Vector3{testPos.x + 0.2f, testPos.y, testPos.z + 0.2f}},
                                BoundingBox{
                                    Vector3{offsetX - 1, wall.y + wall.height + wall.roofOffset, offsetY - 1},
                                    Vector3{offsetX + 1, wall.y + wall.height + wall.roofOffset + wall.roofHeight, offsetY + 1}}))
                            difference = Vector3{playerPosNoCollide.x - playerPosition.x, 0, 0};

                        playerPosition = Vector3Add(playerPosition, difference);
                    }
                }
                else
                {
                    playerPosition.y -= 0.0195f / mapSize;
                }
            }

            EndMode3D();

            if (playingAnimation)
            {
                weaponFrameTimerCount += deltaTime;

                if (weaponFrameTimerCount > 0.05f)
                {
                    weaponFrameTimerCount = 0.0f;
                    weaponFrame++;

                    if (weaponFrame > shotgun.width / shotgun.height)
                    {
                        weaponFrame = 0;
                        playingAnimation = false;
                        weaponFrameTimerCount = 0;
                    }
                }
            }

#ifndef DEBUG
            DrawTexturePro(shotgun, Rectangle{shotgun.height * weaponFrame, 0, shotgun.height, shotgun.height}, Rectangle{0, 0, wy / 2, wy / 2}, Vector2{-((wy / 2) - sin(steps) * 10) - (wy / 8), -(((wy / 2) - cos(sin(-steps) * 2) * 10) + 9)}, 0, WHITE);
#endif
        }

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
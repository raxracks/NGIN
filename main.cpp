#include <raylib.h>
#include <raymath.h>
#include <vector>
#include <time.h>
#include <stdlib.h>

struct Wall {
    Model plane;
    Vector3 pos;
};

struct WallData {
    Vector3 pos;
    Vector2 size;
    Vector2 rotation;
};

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 600;

    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "NGIN");

    Camera3D camera = { 0 };                        // Initialize all values with 0
    camera.position = Vector3{0.0f, 2.5f, 10.0f};   // Camera position
    camera.target = Vector3{0.0f, 0.0f, 0.0f};      // Camera looking at point
    camera.up = Vector3{0.0f, 1.0f, 0.0f};          // Camera up vector (rotation towards target)
    camera.fovy = 90.0f;                            // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;         // Camera mode type

    SetCameraMode(camera, CAMERA_FIRST_PERSON);

    // textures
    Texture wallTexture = LoadTexture("assets/wall.png");
    Texture zombieTexture = LoadTexture("assets/zombie.png");
    Texture shotgunTexture = LoadTexture("assets/shotgun.png");

    // map data
    std::vector<WallData> map = {
        WallData{Vector3{0, 0, 0}, Vector2{10, 4}, Vector2{0, 0}}, 
        WallData{Vector3{0, 0, 4}, Vector2{10, 4}, Vector2{180, 0}}, 
        WallData{Vector3{-5, 0, 9}, Vector2{10, 4}, Vector2{90, 0}}, 
        WallData{Vector3{-5, 0, -5}, Vector2{10, 4}, Vector2{90, 0}},
        WallData{Vector3{0, 0, 2}, Vector2{2, 0.5f}, Vector2{90, 0}},
        WallData{Vector3{0.75f, .5f, 2}, Vector2{2, 0.5f}, Vector2{90, 0}},
        WallData{Vector3{1.5f, 1.0f, 2}, Vector2{2, 0.5f}, Vector2{90, 0}},
        WallData{Vector3{2.25f, 1.5f, 2}, Vector2{2, 0.5f}, Vector2{90, 0}},
    };
            
    std::vector<Wall> walls;
    
    // create walls from map data
    for(int i = 0; i < map.size(); i++) {
         // use 0 pixel width cube instead of plane
        Model PlaneModel = LoadModelFromMesh(GenMeshCube(map[i].size.x, map[i].size.y, 0));
        // Model PlaneModel = LoadModelFromMesh(GenMeshPlane(map[i].size.x, map[i].size.y, 1, 1));
        
        
        PlaneModel.transform = MatrixRotateXYZ(Vector3{/*-90 * DEG2RAD*/0, map[i].rotation.x * DEG2RAD, map[i].rotation.y * DEG2RAD});
        PlaneModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = wallTexture;
        
        Wall wall = {PlaneModel, Vector3{map[i].pos.x, map[i].pos.y + (map[i].size.y / 2), map[i].pos.z}};
        walls.push_back(wall);
    }

    while (!WindowShouldClose())
    {
        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&camera);
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);
            
            BeginMode3D(camera);
                
                for(Wall wall : walls) {
                    DrawModel(wall.plane, wall.pos, 1, WHITE);
                }
                
                 DrawBillboard(camera, zombieTexture, Vector3{-2, 1.5f, 3}, 5, WHITE);
                
                // DrawGrid(10, 1.0f);
                
            EndMode3D();
            
            DrawTextureEx(shotgunTexture, Vector2{(GetScreenWidth() / 2) - (shotgunTexture.width / 4), GetScreenHeight() - (shotgunTexture.height / 2)}, 0, 0.5f, WHITE);
            
            DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

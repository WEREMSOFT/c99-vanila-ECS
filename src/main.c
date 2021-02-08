#include <stdio.h>
#include <raylib.h>
#include "game_object_acyclic_graph.h"

#ifdef OS_WEB
#include <emscripten/emscripten.h>
#endif

#define WIDTH 800
#define HEIGHT 600

Camera3D camera = {0};
GameObjectDAG* world;

inline void gameObjectDAGUpdateWorldPosition(GameObjectDAG* dag) {
    for (int i = 0; i < dag->header.length; i++){
        GameObject* go = &world->gameObjects[i];
        if(go->parent == -1){
            go->worldPosition = go->position;
        } else {
            go->worldPosition = Vector3Add(go->position, dag->gameObjects[go->parent].worldPosition);
        }
    }
}

void update_frame()
{
    gameObjectDAGUpdateWorldPosition(world);

    BeginDrawing();
    {

        ClearBackground(WHITE);
        DrawFPS(10, 10);

        BeginMode3D(camera);
        {
            for(int i = 0; i < world->header.length; i++){
                GameObject* go = &world->gameObjects[i];
                go->draw(go);
            }
            DrawGrid(10, 1);
        }
        EndMode3D();

        for(int i = 0; i < world->header.length; i++) {
            world->gameObjects[i].update(&world->gameObjects[i]);
        }

        
    }
    EndDrawing();

    if (IsKeyDown(KEY_KP_ADD))
        camera.fovy += 1.0f;
    if (IsKeyDown(KEY_KP_SUBTRACT))
        camera.fovy -= 1.0f;
}

int main(void)
{

    InitWindow(WIDTH, HEIGHT, "this is a DAG test");
    SetTargetFPS(60);

    world = GameObjectDAGInit(10);

    GameObjectDAGInsertGameObject(world, gameObjectCreate());
    GameObjectDAGInsertGameObject(world, gameObjectCreate());

    world->gameObjects[0].update = gameObjectUpdateKeyboard;

    world->gameObjects[1].position.x = 1.f;
    world->gameObjects[1].position.z = 1.f;
    
    GameObjectDAGAddChild(world, 0, 1);

    camera.fovy = 45.0f;
    camera.target = (Vector3){.0f, .0f, .0f};
    camera.position = (Vector3){0.0f, 10.0f, 10.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.type = CAMERA_PERSPECTIVE;

#ifdef OS_WEB
    emscripten_set_main_loop(update_frame, 0, 1);
#else
    while (!WindowShouldClose())
    {
        update_frame();
    }
#endif
    CloseWindow();
    GameObjectDAGFini(world);
    return 0;
}
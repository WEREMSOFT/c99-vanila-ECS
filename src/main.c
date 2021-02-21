#include <stdio.h>
#include <raylib.h>
#include "game_object_acyclic_graph.h"
#include <stdlib.h>
#include <pthread.h>
#include <error.h>

#ifdef OS_WEB
#include <emscripten/emscripten.h>
#endif

#define WIDTH 800
#define HEIGHT 600
#define ARRAY_SIZE 1000
#define NUM_THREADS 5
Camera3D camera = {0};
GameObjectDAG* world;

int count = 1000;

typedef struct ThreadParam {
    int start;
    int end;
} ThreadParam;

/**
 * maxObjects is the number of objects that we think is in the array. If we have 1 object(like in parent)
 * the sort will stop once it fount it
 */ 
static inline void binarySort(GameObjectDAG* world, Tag tag, int maxObjects) {
    int currentPlace = 0;
    for(int i = 0; i < world->header.length && currentPlace < maxObjects; i++) {
        if(world->gameObjects[i].tag == tag) {
            GameObject pivot = world->gameObjects[currentPlace];
            world->gameObjects[currentPlace] = world->gameObjects[i];
            world->gameObjects[i] = pivot;
            currentPlace++;
        }
    }
}

static inline void* gameObjectDAGUpdateWorldPosition(void* threadParam);
static void update_frame();

static inline int resetOrder(GameObject* a, GameObject* b) {
    return a->id - b->id;
}

static inline void* gameObjectDAGUpdateWorldPosition(void* threadParam) {
    ThreadParam* param = (ThreadParam*)threadParam;
    // printf("start %d - end %d\n", param->start, param->end);
    for (int i = param->start; i < param->end; i++){
        GameObject* go = &world->gameObjects[i];
        if(go->parent == -1){
            go->worldPosition = go->position;
        } else {
            go->worldPosition = Vector3Add(go->position, world->gameObjects[go->parent].worldPosition);
        }
    }
}

static inline void* gameObjectDAGUpdateCircular(void* threadParam) {
    ThreadParam* param = (ThreadParam*)threadParam;
    for(int i = param->start; i < param->end && world->gameObjects[i].tag == CHILD_CIRCLE; i++) {
        gameObjectUpdateCircular(&world->gameObjects[i]);
    }
}

inline void update_frame()
{
    pthread_t thread_handlers[NUM_THREADS] = {0};
    ThreadParam thread_params[NUM_THREADS] = {0};
    /**
     * The idea behind this logic is, instead of calling objects methods, we have the array
     * we sort and process, sort and process. The sorting put all the objects we want to 
     * process in the begining of the array, so we iterate the array as long as
     * the object tag is the one we want to work with. The last phase is restore the 
     * array to it's original order(by id)
     */
    
    binarySort(world, PARENT, 1);
    for(int i = 0; i < world->header.length && world->gameObjects[i].tag == PARENT; i++) {
        gameObjectUpdateKeyboard(&world->gameObjects[i]);
    }
    
    binarySort(world, CHILD_CIRCLE, INT_MAX);
    int cuarterLength = world->header.length / NUM_THREADS;
    for(int i = 0; i < NUM_THREADS; i++){
        thread_params[i].start = cuarterLength * i;
        thread_params[i].end = thread_params[i].start + cuarterLength;
        int erno = 0;
        if(erno = pthread_create(&thread_handlers[i], NULL, gameObjectDAGUpdateCircular, &thread_params[i])){
            perror("error creating thread");
            printf("Error creating thread %d\n", erno);
            exit (1);
        }
    }

    for(int i = 0; i < NUM_THREADS; i++){
        int erno = 0;
        if(erno = pthread_join(thread_handlers[i], NULL)){
            perror("error joining thread");
            printf("Error joining thread %d\n", erno);
            exit(-1);
        }
    }


    // binarySort(world, CHILD, INT_MAX);
    // for(int i = 0; i < world->header.length && world->gameObjects[i].tag == CHILD; i++) {
    //     gameObjectUpdate(&world->gameObjects[i]);
    // }
    
    qsort(world->gameObjects, world->header.length, sizeof(GameObject), resetOrder);
    for(int i = 0; i < NUM_THREADS; i++){
        thread_params[i].start = cuarterLength * i;
        thread_params[i].end = thread_params[i].start + cuarterLength;
        int erno = 0;
        if(erno = pthread_create(&thread_handlers[i], NULL, gameObjectDAGUpdateWorldPosition, &thread_params[i])){
            perror("error creating thread");
            printf("Error creating thread %d\n", erno);
            exit (1);
        }
    }

    for(int i = 0; i < NUM_THREADS; i++){
        int erno = 0;
        if(erno = pthread_join(thread_handlers[i], NULL)){
            perror("error joining thread");
            printf("Error joining thread %d\n", erno);
            exit(-1);
        }
    }

    BeginDrawing();
    {

        ClearBackground(WHITE);
        DrawFPS(10, 10);

        {
            char text[100] = {0};
            snprintf(text, 100, "Remaining cyclkes %d", count);
            DrawText(text, 100, 10, 20, RED);
        }

        BeginMode3D(camera);
        {
            // for(int i = 0; i < world->header.length; i++){
            //     gameObjectDraw(&world->gameObjects[i]);
            // }
            DrawGrid(10, 1);
        }
        EndMode3D();
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
    SetTargetFPS(1000);

    world = GameObjectDAGInit(10);

    GameObjectDAGInsertGameObject(&world, gameObjectCreate());
    world->gameObjects[0].tag = PARENT;

    Color colors[] = {RED, GREEN, PURPLE, BLACK, YELLOW};

    for(int i=0; i < ARRAY_SIZE; i++){
        for(int j=0; j < ARRAY_SIZE; j++){
            GameObjectDAGInsertGameObject(&world, gameObjectCreate());
            int arrayPosition = i * ARRAY_SIZE + j + 1;
            world->gameObjects[arrayPosition].tag = i != j ? CHILD : CHILD_CIRCLE;
            world->gameObjects[arrayPosition].position.x = j;
            world->gameObjects[arrayPosition].position.z = i;
            world->gameObjects[arrayPosition].color = colors[(i * j) % 5];
            GameObjectDAGAddChild(world, 0, arrayPosition);
        }
    }

    camera.fovy = 45.0f;
    camera.target = (Vector3){.0f, .0f, .0f};
    camera.position = (Vector3){0.0f, 10.0f, 10.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.type = CAMERA_PERSPECTIVE;

#ifdef OS_WEB
    emscripten_set_main_loop(update_frame, 0, 1);
#else
// we iterate only 1000 frames, to measure cache misses with valgrind 
// valgrind --tool=cachegrind ./bin/main.bin 10000000
    while (!WindowShouldClose() && count--)
    {
        update_frame();
    }
#endif
    CloseWindow();
    GameObjectDAGFini(world);
    return 0;
}
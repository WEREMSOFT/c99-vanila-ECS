#ifndef __GAME_OBJECT_DIRECTED_ACYCLIC_GRAPH_H__
#define __GAME_OBJECT_DIRECTED_ACYCLIC_GRAPH_H__

#include <stdlib.h>
#include <memory.h>
#include "util/array_header.h"
#include <raylib.h>
#include <raymath.h>

struct GameObject;

typedef enum Primitive {
    CUBE,
    CILINDER
} Primitive;

typedef struct GameObject {
    int parent;
    void(*update)(struct GameObject*);
    void(*draw)(struct GameObject*);
    Vector3 worldPosition;
    Vector3 position;
    Primitive type;
} GameObject;

typedef struct GameObjectDAG {
    ArrayHeader header;
    GameObject gameObjects[1];
} GameObjectDAG;

void gameObjectUpdateKeyboard(GameObject* self){
    if (IsKeyPressed(KEY_LEFT))
        self->position.x -= 1.0f;
    if (IsKeyPressed(KEY_RIGHT))
        self->position.x += 1.0f;
    if (IsKeyPressed(KEY_UP))
        self->position.z -= 1.0f;
    if (IsKeyPressed(KEY_DOWN))
        self->position.z += 1.0f;
}

void gameObjectUpdate(GameObject* self){
}

void gameObjectDraw(GameObject* self){
    DrawCube(self->worldPosition, 1, 1, 1, RED);
    DrawCubeWires(self->worldPosition, 1, 1, 1, BLUE);
}


GameObject gameObjectCreate(){
    GameObject self = {0};
    self.parent = -1;
    self.update = gameObjectUpdate;
    self.draw = gameObjectDraw;
    return self;
}

// recomended initial capacity 10
GameObjectDAG* GameObjectDAGInit(int initialCapacity){
    size_t size = sizeof(GameObject) * initialCapacity + sizeof(ArrayHeader);
    GameObjectDAG* dag = (GameObjectDAG*)malloc(size);
    
    if(!dag){
        printf("Error allocation memory for GameObjectDAG %s::%d\n", __FILE__, __LINE__);
        exit(-1);
    }

    memset(dag, 0, size);

    dag->header.capacity = initialCapacity;
    dag->header.elementSize = sizeof(GameObject);
    dag->header.length = 0;
    return dag;
}

void GameObjectDAGInsertGameObject(GameObjectDAG* self, GameObject element){
    if(self->header.length + 1 == self->header.capacity){
        self = realloc(self, self->header.capacity * self->header.elementSize * 2 + sizeof(ArrayHeader));
        if(self == NULL) {
            printf("Error reallocating GameObjectDAG\n");
            exit(-1);
        } else {
            self->header.capacity *= 2;
        }
    }
    if(self->header.length == 0) element.parent = -1;
    self->gameObjects[self->header.length++] = element;
}

int GameObjectDAGAddChild(GameObjectDAG* self, uint64_t parent, uint64_t child){
    if(parent <= child){
        self->gameObjects[child].parent = parent;
        return 0;
    } else {
        return -1;
    }
}

void GameObjectDAGFini(GameObjectDAG* self) {
    free(self);
}

static void gameObjectPrint(GameObject gameObjects[], int gameObjectCount, int gameObject, int indent) {
    if(gameObject >= gameObjectCount) return;

    for(int i = 0; i < indent; i++) printf("\t");
    
    for(int i = gameObject + 1; i < gameObjectCount; i++) {
        if(gameObjects[i].parent == gameObject) gameObjectPrint(gameObjects, gameObjectCount, i, indent + 1);
    }
}

void GameObjectDAGPrint(GameObjectDAG* self) {
    gameObjectPrint(self->gameObjects, self->header.length, 0, 0);
}

#endif
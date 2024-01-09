#ifndef __LRU_H__
#define __LRU_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Tries.h"
#include "macros.h"

typedef struct Node {
    LLHeadPtr lh;
    char* filePath;
    struct Node* prev;
    struct Node* next;
} Node;

typedef struct LRUCache {
    int capacity;
    int size;
    Node* head;
    Node* tail;
    Node** map; 
} LRUCache;

LRUCache* initLRUCache(int capacity);
Node* createCacheNode(char* filePath, LLHeadPtr lh);
void addToCache(LRUCache* cache, Node* node) ;
void evictLRUNode(LRUCache* cache);

LLHeadPtr get(LRUCache* cache, char* filePath);
void put(LRUCache* cache, char* filePath, LLHeadPtr lh);
unsigned long hashfunction(char* filepath);
LLHeadPtr removeForDelete(LRUCache *cache, char *filePath);
void RemoveFromHashMap(LRUCache *cache,Node* node,int hash);

#endif
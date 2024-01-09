#include "LRU.h"

Node *createCacheNode(char *filePath, LLHeadPtr lh)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->filePath = strdup(filePath);
    newNode->lh = lh;
    newNode->prev = NULL;
    newNode->next = NULL;
    return newNode;
}

LRUCache *initLRUCache(int capacity)
{
    LRUCache *cache = (LRUCache *)malloc(sizeof(LRUCache));
    cache->capacity = capacity;
    cache->size = 0;
    cache->head = NULL;
    cache->tail = NULL;

    cache->map = (Node **)calloc(31, sizeof(Node *));

    return cache;
}

void addToCache(LRUCache *cache, Node *node)
{
    if (cache->tail != NULL)
    {
        cache->tail->next = node;
    }

    node->prev = cache->tail;
    node->next = NULL;
    cache->tail = node;

    if (cache->head == NULL)
    {
        cache->head = node;
    }
}
void evictLRUNode(LRUCache *cache)
{
    Node *temp = cache->head;
    cache->head = temp->next;

    if (cache->head != NULL)
    {
        cache->head->prev = NULL;
    }

    int hash = hashfunction(temp->filePath) % 31;
    cache->map[hash] = NULL;

    free(temp->filePath);
    free(temp);
}

void removeFromCache(LRUCache *cache, Node *node)
{
    if (node->prev != NULL)
    {
        node->prev->next = node->next;
    }
    else
    {
        cache->head = node->next;
    }

    if (node->next != NULL)
    {
        node->next->prev = node->prev;
    }
    else
    {
        cache->tail = node->prev;
    }
}

LLHeadPtr get(LRUCache *cache, char *filePath)
{
    unsigned long hashinit = hashfunction(filePath);
    int hash = hashinit % 31;
    Node *node = cache->map[hash];
    while (node != NULL)
    {

        if (strcmp(node->filePath, filePath) == 0)
        {
            if (node != cache->tail)
            {
                removeFromCache(cache, node);
                addToCache(cache, node);
            }
            printf(GREEN "CACHE HIT\n" DEFAULT);
            return node->lh;
        }
        node = node->next;
    }
    printf(ORANGE "CACHE MISS\n" DEFAULT);
    return NULL;
}

// void CheckCache(LRUCache* cache,TriesHeadPtr THP,char* path,LLHeadPtr lh){
//     lh=get(cache,path);
//     if(lh==NULL){
//         searchWords(THP, path,lh);
//     }
// }

void put(LRUCache *cache, char *filePath, LLHeadPtr lh)
{
    //printf("put in cache\n");
    unsigned long hashinit = hashfunction(filePath);
    int hash = hashinit % 31;
    Node *node = cache->map[hash];
    Node *newNode = createCacheNode(filePath, lh);
    if (node==NULL){
        cache->map[hash] = newNode;
    }
    else{
        Node* prev = node;
        while (node != NULL)
        {
            if (strcmp(node->filePath, filePath) == 0)
            {
                node->lh = lh;
                if (node != cache->tail)
                {
                    removeFromCache(cache, node);
                    addToCache(cache, node);
                }
                return;
            }
            prev=node;
            node = node->next;
        }
        prev->next=newNode;
    }  
    if (cache->size >= cache->capacity)
    {
        evictLRUNode(cache);
        cache->size--;
    }

    addToCache(cache, newNode);
    cache->size++;
}

// djb2 hashfunction
unsigned long hashfunction(char *filepath)
{
    unsigned long hash = 5381;
    int c;

    while ((c = *filepath++))
    {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}
void RemoveFromHashMap(LRUCache *cache,Node* node,int hash){
     

    if (node->prev != NULL)
    {
        node->prev->next = node->next;
    }
    else
    {
        cache->map[hash] = node->next;
    }

    if (node->next != NULL)
    {
        node->next->prev = node->prev;
    }

}
LLHeadPtr removeForDelete(LRUCache *cache, char *filePath)
{
    unsigned long hashinit = hashfunction(filePath);
    int hash = hashinit % 31;
    Node *node = cache->map[hash];
    while (node != NULL)
    {

        if (strcmp(node->filePath, filePath) == 0)
        {   //printf("%p\n",cache->head);
            removeFromCache(cache, node);
            RemoveFromHashMap(cache,node,hash);
            cache->size--;
            printf(WHITE "DELETED FROM CACHE\n" DEFAULT);
            //printf("%p\n",cache->head);
            return node->lh;

        }
        node=node->next;

    }
    printf(WHITE "DELETED FILE NOT IN CACHE\n" DEFAULT);
    return NULL;
}
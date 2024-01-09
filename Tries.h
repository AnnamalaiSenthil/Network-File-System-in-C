#ifndef __TRIES_H__
#define __TRIES_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct LLNode *LLNodePtr;
typedef struct LLHead *LLHeadPtr;

struct LLNode
{
    int val;
    LLNodePtr next;
};

struct LLHead
{
    long long int currSize;
    LLNodePtr first;
};

typedef struct Tries *TriesPtr;
typedef struct TriesHead *TriesHeadPtr;

struct Tries
{
    TriesPtr *alpha;
    LLHeadPtr ports;
    int count;
    short is_end;
};

struct TriesHead
{
    TriesPtr head;
};


LLHeadPtr initList();
LLNodePtr createNode(int val);
void printList(LLHeadPtr lh);

TriesPtr initTrieNode();
TriesHeadPtr initTries();
int checkForPortExistance(LLHeadPtr ll,int port);
void insertTries3(TriesHeadPtr thp, char *val,int ss1,int ss2,int ss3);
void insertTries1(TriesHeadPtr thp, char *val,int ss1);
void searchAll(TriesPtr tp, LLHeadPtr lh); // dont use
int normalCompare(const void *stringOne, const void *stringTwo);
void searchWords(TriesHeadPtr thp, char *val, LLHeadPtr lh);  // returns the ports in the linked list provided
void printTries(TriesPtr tp, int j);
TriesHeadPtr loadTrieContentsFromFile(const char *filename);
void saveTrieToFile(TriesHeadPtr thp, const char *filename);
void saveTrieContentsToFile(TriesPtr tp, char *word, FILE *file);
void DeleteNode(TriesHeadPtr thp,char * val);
void Delete_Trie(TriesHeadPtr thp); // empty function   code to be written
void DeleteNode(TriesHeadPtr thp,char * val);
char * nextentry(TriesHeadPtr thp,char * val);
char* rec(TriesPtr tp,char * val,int len);
void insertback(LLHeadPtr qn, LLNodePtr elem);
void printTriesFile(TriesPtr tp, int j,FILE *file,char *val);
int searchSubstring(TriesPtr tp, const char *substring);


struct LL_node_trie_content
{
    char * val;
    int ss_index;
    struct LL_node_trie_content * next;
};

struct Trie_content_Head
{
    struct LL_node_trie_content * first;
};

typedef struct LL_node_trie_content * LL_NODE_TRIE_CONTENT;
typedef struct Trie_content_Head * TRIE_CONTENT_HEAD;

LL_NODE_TRIE_CONTENT init_LL_Trie();
TRIE_CONTENT_HEAD init_LL_Trie_Head();
void InsertLLnode_trie(TRIE_CONTENT_HEAD LLH,char * val,int port);
TRIE_CONTENT_HEAD LL_Tries_Ports_fn(TriesHeadPtr THP,char * val);
void Tries_Ports_fn(TRIE_CONTENT_HEAD LLH, TriesPtr tp, int j,char *val);

#endif
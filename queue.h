#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct QUEUE* QUEUE_PTR;
typedef struct QUEUE_HEAD* QUEUE_HEAD_PTR;
typedef struct COMMAND* COMMAND_PTR;

struct COMMAND
{
    char operation[32];
    char path1[512];
    char path2[512];
    int port;
};

struct QUEUE
{
    COMMAND_PTR curr;
    QUEUE_PTR next;
};


struct QUEUE_HEAD
{
    int count;
    QUEUE_PTR head;
    QUEUE_PTR tail;
};

// struct queue_functionaliy_thread_object
// {
//     COMMAND_PTR C;

// };


QUEUE_HEAD_PTR init_Queue();
QUEUE_PTR init_Queue_Element(COMMAND_PTR new_command);
void push(QUEUE_HEAD_PTR qhp, COMMAND_PTR new_command);
COMMAND_PTR pop(QUEUE_HEAD_PTR qhp);
void print_Customers(QUEUE_HEAD_PTR qhp);
int is_empty(QUEUE_HEAD_PTR qhp);

#endif
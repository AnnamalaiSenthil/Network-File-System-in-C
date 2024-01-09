#include "queue.h"

QUEUE_HEAD_PTR init_Queue()
{
    QUEUE_HEAD_PTR qhp=(QUEUE_HEAD_PTR)malloc(sizeof(struct QUEUE_HEAD));
    qhp->count=0;
    qhp->head=NULL;
    qhp->tail=NULL;
    return qhp;
}
QUEUE_PTR init_Queue_Element(COMMAND_PTR new_command)
{
    QUEUE_PTR qp=(QUEUE_PTR)malloc(sizeof(struct QUEUE));
    qp->curr=new_command;
    qp->next=NULL;
    
    return qp;
}
void push(QUEUE_HEAD_PTR qhp, COMMAND_PTR new_command)
{
    if(qhp->count==0)
    {
        QUEUE_PTR temp=init_Queue_Element(new_command);
        qhp->count=qhp->count+1;
        qhp->head=temp;
        qhp->tail=temp;
    }
    else
    {
        QUEUE_PTR temp=init_Queue_Element(new_command);
        qhp->count=qhp->count+1;
        qhp->tail->next=temp;
        qhp->tail=temp;
    }
}
COMMAND_PTR pop(QUEUE_HEAD_PTR qhp)
{
    if(qhp->count==0)
    {
        return NULL;
    }
    if(qhp->count==1)
    {
        qhp->count=0;
        COMMAND_PTR temp=qhp->head->curr;
        qhp->head=NULL;
        qhp->tail=NULL;
        return temp;
    }
    else
    {
        qhp->count=qhp->count-1;
        COMMAND_PTR temp=qhp->head->curr;
        qhp->head=qhp->head->next;
        return temp;
    }
}

void print_Customers(QUEUE_HEAD_PTR qhp)
{
    QUEUE_PTR temp=qhp->head;
    while(temp!=NULL)
    {
        COMMAND_PTR command=temp->curr;
        printf("%s %s %s\n",command->operation,command->path1,command->path2);
        temp=temp->next;
    }
    printf("\n");
}

int is_empty(QUEUE_HEAD_PTR qhp)
{
    if(qhp->count==0)
    {
        return 1;
    }
    return 0;
}
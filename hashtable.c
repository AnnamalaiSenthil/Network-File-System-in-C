#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#define int long long
#define mod 1000000007

struct Hashtable
{
    int **table_storage;
    int currently_filled;
    int capacity;
};

int Encrypt(char *str, int len)
{
    int sum = 0;
    int key = 31;   
    for (int i = 0; i < len; i++)
    {
        sum += str[i];
        sum %= mod;
        sum *= key;
        sum %= mod;
    }
    return sum;
}

struct Hashtable *HashInit()
{
    struct Hashtable *HT = (struct Hashtable *)(malloc(sizeof(struct Hashtable)));
    HT->currently_filled = 0;
    HT->table_storage = (int **)malloc(sizeof(int *) * 1024);
    HT->capacity=1024;
    for (int i = 0; i < 1024; i++)
    {
        HT->table_storage[i] = (int *)malloc(sizeof(int) * 4);
        HT->table_storage[i][0]=-1;
        HT->table_storage[i][1]=-1;
        HT->table_storage[i][2]=-1;
        HT->table_storage[i][3]=-1;
    }
    return HT;
}

void Insert_Hashtable(struct Hashtable * HT,char *str,int len,int ss1,int ss2,int ss3)
{
    int key=Encrypt(str,len);
    for(int i=0;i<HT->capacity;i++)
    {
        if(HT->table_storage[i][0]==-1)
        {
            HT->table_storage[i][0]=key;
            HT->table_storage[i][1]=ss1;
            HT->table_storage[i][2]=ss2;
            HT->table_storage[i][3]=ss3;
        }
    }
    HT->currently_filled++;
}

int * return_file_location_linear(struct Hashtable * HT,char * str,int len)
{
    int key=Encrypt(str,len);
    for(int i=0;i<HT->capacity;i++)
    {
        if(HT->table_storage[i][0]!=key)
            continue;
        int * storage_servers=(int *)malloc(sizeof(int)*3);
        storage_servers[0]=HT->table_storage[i][1];
        storage_servers[0]=HT->table_storage[i][2];
        storage_servers[0]=HT->table_storage[i][3];
        return storage_servers;
    }
    return NULL;
}   // Returns a int * pointer of 3 length conatining the 3 storage servers which contain the corresponding path, if not NULL

void Delete_file(struct Hashtable * HT,char * str,int len)
{
    int key=Encrypt(str,len);
    for(int i=0;i<HT->capacity;i++)
    {
        if(HT->table_storage[i][0]!=key)
            continue;
        HT->table_storage[i][0]=-1;
        HT->currently_filled--;
    }
}

void Delete_Hashtable(struct Hashtable * HT)
{
    for(int i=0;i<HT->capacity;i++)
        free(HT->table_storage[i]);
    free(HT->table_storage);
    free(HT);
}

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

//this struct is to send data from SS to NM, where it sends its details about the port at which it wants to run, and its ip address
//use case: when SS wants to connect to Naming server
struct SS_NM
{
    int port;
    char ip[128];
    char result[4096];
    int list_port;
};

struct ls_data
{
    int size;
    int no_packets;
    char buffer[512];
};

typedef struct SS_NM* SS_NM_PTR;
typedef struct ls_data* LS_DATA_PTR;


LS_DATA_PTR init_ls_data();
SS_NM_PTR init_ss_nm(int port,char* ip);
SS_NM_PTR init();

//this struct is to send an Acknowledgement from SS to NM, where its say yes or no, whether the command has been a success or not
struct ACK_SS_NM
{
    char ticket[32];
};
typedef struct ACK_SS_NM* ACK_SS_NM_PTR;

ACK_SS_NM_PTR fill_ACK_SS_NM(char* val);
ACK_SS_NM_PTR init_ACK_SS_NM();

//this struct is to send an Acknowledgement from NM to Client, where its say yes or no, whether the command has been a success or not
struct ACK_NM_C
{
    char ticket[32];
};

struct ACK_NM_C_LS
{
    char ticket[32];
    char filename[512];
};

typedef struct ACK_NM_C* ACK_NM_C_PTR;
typedef struct ACK_NM_C_LS* ACK_NM_C_LS_PTR;

ACK_NM_C_PTR fill_ACK_NM_C(char* val);
ACK_NM_C_LS_PTR fill_ACK_NM_LS_C(char* val,char * file);
ACK_NM_C_PTR init_ACK_NM_C();
ACK_NM_C_LS_PTR init_ACK_NM_LS_C();


//this struct is to send IP and index of the port(53xx series) to the client
struct NM_C_IP
{
    char ip[128];
    char extra[512];
    int index;
};
typedef struct NM_C_IP* NM_C_IP_PTR;

NM_C_IP_PTR fill_NM_C_IP(char* ip,int index);
NM_C_IP_PTR init_NM_C_IP();

#endif
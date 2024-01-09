#include "structs.h"

SS_NM_PTR init_ss_nm(int port,char* ip)
{
    SS_NM_PTR snp=(SS_NM_PTR)malloc(sizeof(struct SS_NM));
    snp->port=port;
    strcpy(snp->ip,ip);
    return snp;
}
SS_NM_PTR init()
{
    SS_NM_PTR snp=(SS_NM_PTR)malloc(sizeof(struct SS_NM));
    return snp;
}


ACK_SS_NM_PTR fill_ACK_SS_NM(char* val)
{
    ACK_SS_NM_PTR temp=(ACK_SS_NM_PTR)malloc(sizeof(struct ACK_SS_NM));
    strcpy(temp->ticket,val);
    return temp;
}
ACK_SS_NM_PTR init_ACK_SS_NM()
{
    ACK_SS_NM_PTR temp=(ACK_SS_NM_PTR)malloc(sizeof(struct ACK_SS_NM));
    strcpy(temp->ticket,"\0");
    return temp;
}

ACK_NM_C_PTR fill_ACK_NM_C(char* val)
{
    ACK_NM_C_PTR temp=(ACK_NM_C_PTR)malloc(sizeof(struct ACK_NM_C));
    strcpy(temp->ticket,val);
    return temp;
}
ACK_NM_C_LS_PTR fill_ACK_NM_LS_C(char* val,char *file)
{
    ACK_NM_C_LS_PTR temp=(ACK_NM_C_LS_PTR)malloc(sizeof(struct ACK_NM_C_LS));
    // temp->filename=(char*)malloc(sizeof(char)*512);
    strcpy(temp->ticket,val);
    // printf("here\n");
    strcpy(temp->filename,file);
    // printf("or here\n");
    return temp;
}
ACK_NM_C_PTR init_ACK_NM_C()
{
    ACK_NM_C_PTR temp=(ACK_NM_C_PTR)malloc(sizeof(struct ACK_NM_C));
    strcpy(temp->ticket,"\0");
    return temp;
}

ACK_NM_C_LS_PTR init_ACK_NM_LS_C()
{
    ACK_NM_C_LS_PTR temp=(ACK_NM_C_LS_PTR)malloc(sizeof(struct ACK_NM_C_LS));
    // temp->ticket=malloc(sizeof(char)*10);
    strcpy(temp->ticket,"\0");
    // temp->filename=(char*)malloc(sizeof(char)*512);
    strcpy(temp->filename,"-1");
    return temp;
}

NM_C_IP_PTR fill_NM_C_IP(char* ip,int index)
{
    NM_C_IP_PTR temp=(NM_C_IP_PTR)malloc(sizeof(struct NM_C_IP));
    strcpy(temp->ip,ip);
    temp->index=index;
    strcpy(temp->extra,"\0");
    return temp;
}
NM_C_IP_PTR init_NM_C_IP()
{
    NM_C_IP_PTR temp=(NM_C_IP_PTR)malloc(sizeof(struct NM_C_IP));
    strcpy(temp->ip,"\0");
    strcpy(temp->extra,"\0");
    temp->index=-1;
    return temp;
}

LS_DATA_PTR init_ls_data()
{
    LS_DATA_PTR temp=(LS_DATA_PTR)malloc(sizeof(struct ls_data));
    strcpy(temp->buffer,"");
    temp->no_packets=0;
    temp->size=0;
    return temp;
}
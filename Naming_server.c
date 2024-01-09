#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include "queue.h"
#include <signal.h>
#include "Tries.h"
#include "server.h"
#include "structs.h"
#include "log.h"
#include "SS_functions.h"
#include <time.h>
#include "macros.h"
#include "LRU.h"
#include <math.h>
#include "errorcodes.h"
int random1(int n)
{
    srand(time(NULL));
    return rand() % (n + 1);
}

// my idea is to have one master port and one master port dedicated only to connect to different clients and server, which then redirects
// the clients to connect to which ports if available

// we will start listening on the master ports for both, we will also disconnect when there is a connection
// we will start client ports from 5100 t0 5199 and storage server ports from 5200 to 5299
// 5300 to 5399 is to listen port for storage server
// 5400 to 5499 will be for sending the disconnected msg

#define CLIENT_PORT 6
#define SERVER_PORT 6
#define MASTER_CLIENT_PORT 5000
#define MASTER_SERVER_PORT 5001

LRUCache *cache;

char buffer[1024];
TriesHeadPtr THP;
int master_server_sock, master_client_sock;
// this is for clients to connect
int client_sock[CLIENT_PORT];                // to have the NS side of client socket
int client_reciever_sock[CLIENT_PORT];       // to have the server side of client socket
struct sockaddr_in client_addr[CLIENT_PORT]; // to store the addresses of the clients
int client_available[CLIENT_PORT];           // to check if client port is available or not, 0 means available, 1 means not available
sem_t block_client[CLIENT_PORT];

// this is for storage servers to connect
int server_sock[SERVER_PORT];                // to have the NS side of server socket
int server_reciever_sock[SERVER_PORT];       // to have the server side of server socket
struct sockaddr_in server_addr[SERVER_PORT]; // to store the addresses of the servers
int server_available[SERVER_PORT];           // to check if server port is available or not, 0 means available, 1 means not available
char *server_ip[SERVER_PORT];                // this is to store the server ip
sem_t block_server[SERVER_PORT];
// additional server socks
int server_sock2[SERVER_PORT];                // to have the NS side of server socket
int server_reciever_sock2[SERVER_PORT];       // to have the server side of server socket
struct sockaddr_in server_addr2[SERVER_PORT]; // to store the addresses of the servers

// all the threads
pthread_t *client_master_thread;
pthread_t *server_master_thread;
pthread_t *queue_processor;
pthread_t *server_threads;
pthread_t *client_threads;
pthread_t *server_redundancy;

// all the locks
sem_t master_client_lock, client_lock, master_server_lock, lock4, server_lock;
sem_t red_lock;

// other required variables
int temp_server_sock;
int temp_client_sock;
struct sockaddr_in temp_client_addr;
char *ip = "127.0.0.1";

// now there are two queues which need to be initialized
// currently we start with one
QUEUE_HEAD_PTR requests_queue;

// this is to handle the signals that will be coming from ctrl-C
// we close all the connections properly and write Tries data back
void handle_sigint_cntrlC(int sig)
{

    for (int i = 0; i < CLIENT_PORT; i++)
    {
        close(client_sock[i]);
        if (client_available[i] == 1)
        {

            close(client_reciever_sock[i]);
        }
        // close(client_reciever_sock);
    }
    for (int i = 0; i < SERVER_PORT; i++)
    {
        close(server_sock[i]);
        close(server_sock2[i]);
        if (server_available[i] == 1)
        {

            close(server_reciever_sock[i]);
            close(server_reciever_sock2[i]);
        }
    }

    close(master_client_sock);
    close(master_server_sock);
    printf(BLUE "\nHAVE A NICE DAY\n" DEFAULT);
    exit(0);
}

// this is to initialize all the values to -1
void server_initializer()
{
    for (int i = 0; i < CLIENT_PORT; i++)
    {
        client_sock[i] = -1; // Initialize to an invalid value
        client_reciever_sock[i] = -1;
        client_available[i] = 0;
        sem_init(&block_client[i], 0, 1);
        create_tcp_socket(&client_sock[i]);
        connect_to_client(ip, i + 5100, &client_sock[i]);
    }
    for (int i = 0; i < SERVER_PORT; i++)
    {
        server_sock[i] = -1; // Initialize to an invalid value
        server_reciever_sock[i] = -1;
        server_sock2[i] = -1; // Initialize to an invalid value
        server_reciever_sock2[i] = -1;
        server_available[i] = 0;
        server_ip[i] = (char *)malloc(128);
        sem_init(&block_server[i], 0, 1);
        create_tcp_socket(&server_sock[i]);
        connect_to_client(ip, i + 5200, &server_sock[i]);
        create_tcp_socket(&server_sock2[i]);
        connect_to_client(ip, i + 5400, &server_sock2[i]);
    }
    sem_init(&master_client_lock, 0, 1);
    sem_init(&client_lock, 0, 1);
    sem_init(&master_server_lock, 0, 1);
    sem_init(&lock4, 0, 1);
    sem_init(&server_lock, 0, 1);
    sem_init(&red_lock, 0, 1);

    // to initialize the queues and tires data sturcture
    requests_queue = init_Queue();
    THP = initTries();
    cache = initLRUCache(10); // LRU initalization
}

void send_data_to_client(int client_sock, char *txt) // this is to send data to a client
{
    bzero(buffer, 1024);
    strcpy(buffer, txt);
    // printf("Server: %s\n", buffer);
    int s_error = send(client_sock, buffer, strlen(buffer), 0);
    if (s_error < 0)
    {
        perror("[-]Send error");
        // close_all_clients(client_sock);
        exit(1);
    }
}
void recieve_data_from_client(int client_sock) // this is to recieve data from the client
{
    bzero(buffer, 1024);
    int r_error = recv(client_sock, buffer, sizeof(buffer), 0);
    if (r_error < 0)
    {
        perror("[-]Recieve error 1");
        // close_all_clients(client_sock);
        // exit(1);
    }
    printf("Recieved data is %s\n", buffer);
}

int find_a_free_port_for_client(int client_socket) // this function would find a free port and make the client to that port for a connection
{
    sem_wait(&client_lock);
    int free_port = -1;
    for (int i = 0; i < CLIENT_PORT; i++)
    {
        if (client_available[i] == 0)
        {
            free_port = i;
            client_available[i] = 1;
            break;
        }
    }
    sem_post(&client_lock);
    // now we have ou free port, we now need to send it to the client that we have a free port, where they can join
    if (free_port != -1)
    {
        char temp[32];
        sprintf(temp, "%d", free_port + 5100);
        send_data_to_client(client_socket, temp);
        return free_port;
    }
    else
    {
        printf("Sorry no free port available!!!\n");
        return -1;
    }
}

int find_storage_server_port() // this is to find an running server port
{
    printf(ORANGE "IN STORAGE SERVER FINDER, waiting for lock to be available......\n" DEFAULT);
    sem_wait(&server_lock);
    printf(ORANGE "unlocked...Finding an available server...\n" DEFAULT);
    int num = random1(SERVER_PORT - 1);
    printf("NUM:%d\n", num);
    if (server_available[num] == 1)
    {
        printf(GREEN "FOUND...%d\n" DEFAULT, num);
        sem_post(&server_lock);
        return num;
    }
    for (int i = 0; i < SERVER_PORT; i++)
    {
        if (server_available[i] == 1)
        {
            printf(GREEN "FOUND...\n" DEFAULT);
            sem_post(&server_lock);
            return i;
        }
    }
    printf(RED "%d:NOT FOUND...\n" DEFAULT, PORT_NOT_FOUND);
    sem_post(&server_lock);
    return -1;
}
void Createfunctionality(COMMAND_PTR C) // look into the header again
{
    // all steps need to be done
    // either all are done or none are done
    // first we find if it already exists or not
    // if it doesnt exist then we find an availble storage server
    // we then send a request to the storage server to do the task
    // we recieve an acknowledgement from it
    // if it is a successful command we send that acknowlegdement back to the client
    LLHeadPtr ports_exist = (LLHeadPtr)malloc(sizeof(struct LLHead));
    ports_exist->currSize = 0;
    ports_exist->first = NULL;
    // searchWords(THP, C->path1, ports_exist);
    LLHeadPtr ports_found = get(cache, C->path1);
    if (ports_found == NULL)
    {
        searchWords(THP, C->path1, ports_exist);
    }
    else
    {
        ports_exist = ports_found;
    }
    if (ports_exist->currSize > 0) // this is when the file already exists
    {
        printf(YELLOW "Path already exists, No need to create it again!!!\n" DEFAULT);
        ACK_NM_C_PTR temp2 = fill_ACK_NM_C("ALREADY");
        int s = send(client_reciever_sock[C->port - 5100], temp2, sizeof(struct ACK_NM_C), 0);
        if (s < 0)
        {
            printf(RED "%d:Error sending ACK to client\n" DEFAULT, ACK_SEND_ERROR);
            log_command("\0", C->port, -1, C->operation, "ERROR");
        }
        else
        {
            printf(GREEN "SUCCESS\n" DEFAULT);
            log_command("\0", C->port, -1, C->operation, "FILE/FOLDER already exists");
        }
        return;
    }
    int index = find_storage_server_port(); // should actually return 3 ports -- To be done
    int index1 = index;
    if (index >= 0)
    {
        // we will now send struct C forward to SS
        sem_wait(&block_server[index]);
        int s = send(server_reciever_sock[index], C, sizeof(struct COMMAND), 0);
        if (s < 0)
        {
            printf(RED "%d:Error sending ACK to client\n" DEFAULT, ACK_SEND_ERROR);
            log_command(server_ip[index], C->port, index, C->operation, "Error");
            sem_post(&block_server[index]);
            return;
        }
        else if (s == 0)
        {
            printf("Looks like SS %d has shut down!!!", index);
            log_command(server_ip[index], C->port, index, C->operation, "Error");
            sem_post(&block_server[index]);
        }
        else
        {
            // we will now wait for an ACK from SS
            ACK_SS_NM_PTR temp = init_ACK_SS_NM();
            int r = recv(server_reciever_sock[index], temp, sizeof(struct ACK_SS_NM), 0);
            if (r < 0)
            {
                printf("ACK struct not recieved\n");
                log_command(server_ip[index], C->port, index, C->operation, "Error");
                sem_post(&block_server[index]);
                return;
            }
            else if (r == 0)
            {
                printf("Looks like SS %d has shut down!!!", index);
                log_command(server_ip[index], C->port, index, C->operation, "Error");
                sem_post(&block_server[index]);
            }
            else
            {
                if (strcmp(temp->ticket, "YES") != 0)
                {
                    printf(RED "%d:SS couldnt create the file/folder\n" DEFAULT, SS_CREATE_ERROR);
                    ACK_NM_C_PTR temp2 = fill_ACK_NM_C("NO");
                    s = send(client_reciever_sock[C->port - 5100], temp2, sizeof(struct ACK_NM_C), 0);
                    if (s < 0)
                    {
                        printf(RED "%d:Error sending ACK struct\n" DEFAULT, ACK_SEND_ERROR);
                        sem_post(&block_server[index]);
                        return;
                    }
                    else
                    {
                        printf(GREEN "ACK send\n" DEFAULT);
                    }
                    return;
                }
                // insertTries3(THP, C->path1, index1, index1, index1);
                char path[256] = "";
                char *token = strtok(C->path1, "/");
                while (token != NULL)
                {
                    strcat(path, token);

                    printf("%s\n", path); // Print or save to a temporary location as needed
                    token = strtok(NULL, "/");
                    insertTries3(THP, path, index1, index1, index1);
                    strcat(path, "/");
                }
                log_command(server_ip[index], C->port, index + 5200, C->operation, "SUCCESS");
                ACK_NM_C_PTR temp2 = fill_ACK_NM_C("YES");
                s = send(client_reciever_sock[C->port - 5100], temp2, sizeof(struct ACK_NM_C), 0);
                if (s < 0)
                {
                    printf(RED "%d:Error sending ACK struct\n" DEFAULT, ACK_SEND_ERROR);
                    sem_post(&block_server[index]);
                    return;
                }
                else
                {
                    printf(GREEN "ACK send\n" DEFAULT);
                }
            }
            sem_post(&block_server[index]);
            // insert into cache
            LLNodePtr n = createNode(index1);
            ports_exist->first = n;
            ports_exist->currSize = 1;
            put(cache, C->path1, ports_exist);
        }
    }
    printTries(THP->head, 0);
}

void Deletedirfunctionality(COMMAND_PTR C)
{
    printTries(THP->head, 0);
    // printf("In delete directory now\n");
    LLHeadPtr ports_exist = (LLHeadPtr)malloc(sizeof(struct LLHead));
    LLHeadPtr ports_exist2 = (LLHeadPtr)malloc(sizeof(struct LLHead));
    ports_exist->currSize = 0;
    ports_exist->first = NULL;
    ports_exist2->currSize = 0;
    ports_exist2->first = NULL;
    searchWords(THP, C->path1, ports_exist);
    if (ports_exist->currSize <= 0)
    {
        printf(RED "%d:There is no path with the same name, cannot delete something that doesnt exist!!!\n" DEFAULT, NOT_FOUND_DELETE);
        ACK_NM_C_PTR temp2 = fill_ACK_NM_C("YES");
        int s = send(client_reciever_sock[C->port - 5100], temp2, sizeof(struct ACK_NM_C), 0);
        if (s < 0)
        {
            printf(RED "%d:Error sending ACK struct\n" DEFAULT, ACK_SEND_ERROR);
        }
        else
        {
            printf(GREEN "SUCCESS\n" DEFAULT);
        }
        return;
    }
    int index = -1;
    LLNodePtr temp = ports_exist->first;
    index = temp->val;
    ACK_SS_NM_PTR temp2 = init_ACK_SS_NM();
    while (temp != NULL)
    {
        index = temp->val;
        sem_wait(&block_server[index]);
        int s = send(server_reciever_sock[index], C, sizeof(struct COMMAND), 0);
        if (s < 0)
        {
            printf(RED "Error sending struct\n" DEFAULT);
        }
        else
        {
        }
        temp = temp->next;

        int r = recv(server_reciever_sock[index], temp2, sizeof(struct ACK_SS_NM), 0);
        if (r < 0)
        {
            printf(RED "%d:ACK struct not recieved\n" DEFAULT, STRUCT_RECEIVE_ERROR);
        }
        sem_post(&block_server[index]);
    }
    char *junk = (char *)malloc(sizeof(char) * 512);
    strcpy(junk, C->path1);
    junk[strlen(junk) + 1] = '\0';
    junk[strlen(junk)] = '/';
    searchWords(THP, junk, ports_exist);
    char *dummy = (char *)malloc(sizeof(char) * 512);
    strcpy(dummy, nextentry(THP, C->path1));
    if (1)
    {
        // we will now send struct C forward to SS
        // have to iterate through the tries and find out all the files which have the given path as a prefix
        TriesPtr tp = THP->head;
        int length = strlen(C->path1);
        for (int i = 0; i < length; i++)
        {
            // tp->is_end = 0;
            if (i != length)
            {
                int pos = (int)C->path1[i];
                tp = tp->alpha[pos];
            }
        }
        // DeleteNode(THP,C->path1);
        if (tp->alpha['/'] != NULL)
        {
            tp = tp->alpha['/'];
            // tp->is_end=0;
            // printf("%d %d--\n",THP->head->alpha['d']->alpha['i']->alpha['r']->count,THP->head->alpha['d']->alpha['i']->alpha['r']->is_end);
            while (tp->count > 1)
            {
                // printf("%d %d--\n",THP->head->alpha['d']->alpha['i']->alpha['r']->count,THP->head->alpha['d']->alpha['i']->alpha['r']->is_end);
                strcpy(dummy, nextentry(THP, junk));
                // printf("%s&&\n", dummy);
                removeForDelete(cache, dummy);
                DeleteNode(THP, dummy);
                // printf("%d %d--\n",THP->head->alpha['d']->alpha['i']->alpha['r']->count,THP->head->alpha['d']->alpha['i']->alpha['r']->is_end);
            }
            strcpy(dummy, nextentry(THP, junk));
            // printf("%s&&\n", dummy);
            removeForDelete(cache, dummy);
            DeleteNode(THP, dummy);
        }
        removeForDelete(cache, C->path1);
        DeleteNode(THP, C->path1);

        if (1)
        {
            ACK_NM_C_PTR temp2 = fill_ACK_NM_C("YES");
            int s = send(client_reciever_sock[C->port - 5100], temp2, sizeof(struct ACK_NM_C), 0);
            if (s < 0)
            {
                printf(RED "%d:SS couldnt create the file/folder\n" DEFAULT, SS_CREATE_ERROR);
            }
            else
            {
                printf(GREEN "SUCCESS\n" DEFAULT);
            }
        }
    }
}

void Deletefilefunctionality(COMMAND_PTR C)
{
    // all steps need to be done
    // either all are done or none are done
    // first we find if it already exists
    // we then send a request to the storage server to do the task
    // we recieve an acknowledgement from it
    // if it is a successful command we send that acknowlegdement back to the client
    int res = strcmp("deletedir", C->operation);
    if (res == 0)
    {
        Deletedirfunctionality(C);
        return;
    }
    LLHeadPtr ports_exist = (LLHeadPtr)malloc(sizeof(struct LLHead));
    ports_exist->currSize = 0;
    ports_exist->first = NULL;
    LLHeadPtr ports_found = removeForDelete(cache, C->path1);
    if (ports_found == NULL)
    {
        searchWords(THP, C->path1, ports_exist);
    }
    else
    {
        ports_exist = ports_found;
    }

    if (ports_exist->currSize == -1)
        ports_exist->currSize++;
    if (ports_exist->currSize == 0)
    {
        printf(RED "There is no path with the same name, cannot delete something that doesnt exist!!!\n" DEFAULT);
        ACK_NM_C_PTR temp2 = fill_ACK_NM_C("YES");
        int s = send(client_reciever_sock[C->port - 5100], temp2, sizeof(struct ACK_NM_C), 0);
        if (s < 0)
        {
            printf(RED "%d:Error sending ACK struct\n" DEFAULT, ACK_SEND_ERROR);
        }
        else
        {
            printf(GREEN "SUCCESS\n" DEFAULT);
        }
        return;
    }
    int index = -1;
    LLNodePtr temp = ports_exist->first;
    while (temp != NULL && index == -1)
    {
        index = temp->val;
        temp = temp->next;
    }
    // so we need to delete it from every place it exists in
    // needs to be done fr all indices index1,index2,index3
    if (index >= 0)
    {
        removeForDelete(cache, C->path1);
        sem_wait(&block_server[index]);
        // we will now send struct C forward to SS
        // have to iterate through the tries and find out all the files which have the given path as a prefix
        TriesPtr tp = THP->head;
        int length = strlen(C->path1);
        for (int i = 0; i < length; i++)
        {
            // tp->is_end = 0;
            if (i != length)
            {
                int pos = (int)C->path1[i];
                tp = tp->alpha[pos];
            }
        }
        tp->is_end = 0;
        char *T = malloc(sizeof(char) * 512);
        strcpy(T, C->path1);
        // removeFromCache(cache,T);
        // printf("%d\n", tp->count);
        // printf("%s\n", T);
        strcpy(C->path1, nextentry(THP, T));
        int s = send(server_reciever_sock[index], C, sizeof(struct COMMAND), 0);
        if (s < 0)
        {
            printf(RED "Error sending struct\n" DEFAULT);
        }
        else
        {
            // printf("Successful\n");
        }
        DeleteNode(THP, nextentry(THP, T));
        ACK_SS_NM_PTR temp = init_ACK_SS_NM();
        int r = recv(server_reciever_sock[index], temp, sizeof(struct ACK_SS_NM), 0);
        if (r < 0)
        {
            printf(RED "%d:ACK struct not recieved\n" DEFAULT, STRUCT_RECEIVE_ERROR);
        }
        else
        {
            ACK_NM_C_PTR temp2 = fill_ACK_NM_C("YES");
            s = send(client_reciever_sock[C->port - 5100], temp2, sizeof(struct ACK_NM_C), 0);
            if (s < 0)
            {
                printf(RED "%d:Error sending ACK struct\n" DEFAULT, ACK_SEND_ERROR);
            }
            else
            {
                printf(GREEN "SUCCESS\n" DEFAULT);
            }
        }
        sem_post(&block_server[index]);
    }
}

void Readfunctionality(COMMAND_PTR C)
{
    // so we need to send the index of the SS and ip to the client
    // the client will then make the connection with the SS and will wait for the connection
    // then after reading data, as soon as the client issues stop, then it will stop the reading connection to the SS
    // before anything we need to see if the file exists or not
    LLHeadPtr ports_exist = (LLHeadPtr)malloc(sizeof(struct LLHead));
    ports_exist->currSize = 0;
    ports_exist->first = NULL;
    // searchCache(C->path);
    // ports
    int index = -1;
    // searchWords(THP, C->path1, ports_exist);
    LLHeadPtr ports_found = get(cache, C->path1);
    if (ports_found == NULL)
    {
        searchWords(THP, C->path1, ports_exist);
    }
    else
    {
        ports_exist = ports_found;
    }
    if (ports_exist->currSize == -1)
        ports_exist->currSize++;
    // search words gives an existance, then you insert it here
    if (ports_exist->currSize == 0)
    {
        printf(RED "%d:File doesnt exist!!!\n" DEFAULT, FILE_DOESNT_EXIST);
        NM_C_IP_PTR ptr = fill_NM_C_IP("-1", index);
        int s = send(client_reciever_sock[C->port - 5100], ptr, sizeof(struct NM_C_IP), 0);
        if (s < 0)
        {
            printf(RED "%d:Error sending ACK struct\n" DEFAULT, ACK_SEND_ERROR);
            log_command(server_ip[index], C->port, index + 5200, C->operation, "FAILURE");
        }
        else
        {
            printf(GREEN "SUCCESS\n" DEFAULT);
            log_command(server_ip[index], C->port, index + 5200, C->operation, "FILE DOESNT EXIST");
        }
        return;
    }
    // now that we have found a port where file exist
    put(cache, C->path1, ports_exist);
    LLNodePtr temp = ports_exist->first;
    while (temp != NULL && index == -1)
    {
        index = temp->val;
        temp = temp->next;
    }
    // now we have got the index where the port exists
    // so we sends this to the client
    // before sending we need to make server busy so that no other client can use it
    char *path = (char *)calloc(512, sizeof(char));
    strcpy(path, "main");
    if (server_available[index] == 0) // this server is not available, so now we need to read from its copy
    {
        int mod = index % 3;
        int index1, index2;
        char p1[6] = {'c', 'o', 'p', 'y', '\0', '\0'}; // names of the folder of the copy
        char p2[6] = {'c', 'o', 'p', 'y', '\0', '\0'};
        if (mod == 0)
        {
            index1 = index + 1;
            index2 = index + 2;
            p1[4] = '1';
            p2[4] = '1';
        }
        else if (mod == 1)
        {
            index1 = index - 1;
            index2 = index + 1;
            p1[4] = '1';
            p2[4] = '2';
        }
        else
        {
            index1 = index - 2;
            index2 = index - 1;
            p1[4] = '2';
            p2[4] = '2';
        }
        // now i need to read from copy servers rather than reading from the actual one
        if (server_available[index1] == 1)
        {
            index = index1;
            strcpy(path, p1);
        }
        else if (server_available[index2] == 1)
        {
            index = index2;
            strcpy(path, p2);
        }
        else
        {
            // do nothing and let it read and print an error
        }
    }
    sem_wait(&block_server[index]);
    NM_C_IP_PTR ptr = fill_NM_C_IP(server_ip[index], index);
    strcpy(ptr->extra, path);
    // printf("%s | %s\n",path,ptr->extra);
    int s = send(client_reciever_sock[C->port - 5100], ptr, sizeof(struct NM_C_IP), 0);
    if (s < 0)
    {
        printf(RED "%d:Error sending ACK struct\n" DEFAULT, ACK_SEND_ERROR);
        log_command(server_ip[index], C->port, index + 5200, C->operation, "FAILURE");
        return;
    }
    else
    {
        printf(GREEN "SUCCESS\n" DEFAULT);
    }
    struct COMMAND temp3;
    int bytesRead = recv(client_reciever_sock[C->port - 5100], &temp3, sizeof(struct COMMAND), 0);
    if (strcmp(temp3.operation, "stop") == 0)
    {
        printf(GREEN "CONNECTION STOPPED\n" DEFAULT);
    }
    else
    {
        printf(RED "%d:Some error while reading" DEFAULT, READ_ERROR);
        sem_post(&block_server[index]);
        log_command(server_ip[index], C->port, index + 5200, C->operation, "FAILURE");
        return;
    }
    sem_post(&block_server[index]);
    log_command(server_ip[index], C->port, index + 5200, C->operation, "SUCCESS");
}
void Writefunctionality(COMMAND_PTR C)
{
    LLHeadPtr ports_exist = (LLHeadPtr)malloc(sizeof(struct LLHead));
    ports_exist->currSize = 0;
    ports_exist->first = NULL;
    // searchCache(C->path);
    // ports
    int index = -1;
    // searchWords(THP, C->path1, ports_exist);
    LLHeadPtr ports_found = get(cache, C->path1);
    if (ports_found == NULL)
    {
        searchWords(THP, C->path1, ports_exist);
        put(cache, C->path1, ports_exist);
    }
    else
    {
        ports_exist = ports_found;
    }
    if (ports_exist->currSize == -1)
        ports_exist->currSize++;
    // search words gives an existance, then you insert it here
    if (ports_exist->currSize == 0)
    {
        printf(RED "%d:File doesnt exist!!!\n" DEFAULT, FILE_DOESNT_EXIST);
        NM_C_IP_PTR ptr = fill_NM_C_IP("-1", index);
        int s = send(client_reciever_sock[C->port - 5100], ptr, sizeof(struct NM_C_IP), 0);
        if (s < 0)
        {
            printf(RED "%d:Error sending ACK struct\n" DEFAULT, ACK_SEND_ERROR);
            log_command(server_ip[index], C->port, index + 5200, C->operation, "FAILURE");
        }
        else
        {
            printf(GREEN "SUCCESS\n" DEFAULT);
            log_command(server_ip[index], C->port, index + 5200, C->operation, "FILE DOESNT EXIST");
        }
        return;
    }
    // now that we have found a port where file exist

    LLNodePtr temp = ports_exist->first;
    while (temp != NULL && index == -1)
    {
        index = temp->val;
        temp = temp->next;
    }
    // now we have got the index where the port exists
    // so we sends this to the client
    // before sending we need to make server busy so that no other client can use it
    sem_wait(&block_server[index]);
    NM_C_IP_PTR ptr = fill_NM_C_IP(server_ip[index], index);
    int s = send(client_reciever_sock[C->port - 5100], ptr, sizeof(struct NM_C_IP), 0);
    if (s < 0)
    {
        printf(RED "%d:Error sending ACK struct\n" DEFAULT, ACK_SEND_ERROR);
        log_command(server_ip[index], C->port, index + 5200, C->operation, "FAILURE");
        return;
    }
    else
    {
        printf(GREEN "SUCCESS\n" DEFAULT);
    }
    struct COMMAND temp3;
    int bytesRead = recv(client_reciever_sock[C->port - 5100], &temp3, sizeof(struct COMMAND), 0);
    if (strcmp(temp3.operation, "stop") == 0)
    {
        printf(GREEN "CONNECTION STOPPED\n" DEFAULT);
    }
    else
    {
        printf(RED "%d:Some error while writing" DEFAULT, WRITE_ERROR);
        sem_post(&block_server[index]);
        log_command(server_ip[index], C->port, index + 5200, C->operation, "FAILURE");
        return;
    }
    sem_post(&block_server[index]);
    log_command(server_ip[index], C->port, index + 5200, C->operation, "SUCCESS");
}

void Infofunctionality(COMMAND_PTR C)
{
    LLHeadPtr ports_exist = (LLHeadPtr)malloc(sizeof(struct LLHead));
    ports_exist->currSize = 0;
    ports_exist->first = NULL;
    // searchCache(C->path);
    // ports
    int index = -1;
    // searchWords(THP, C->path1, ports_exist);
    LLHeadPtr ports_found = get(cache, C->path1);
    if (ports_found == NULL)
    {
        searchWords(THP, C->path1, ports_exist);
        put(cache, C->path1, ports_exist);
    }
    else
    {
        ports_exist = ports_found;
    }
    if (ports_exist->currSize == -1)
        ports_exist->currSize++;
    // search words gives an existance, then you insert it here
    if (ports_exist->currSize == 0)
    {
        printf(RED "%d:File doesnt exist!!!\n" DEFAULT, FILE_DOESNT_EXIST);
        NM_C_IP_PTR ptr = fill_NM_C_IP("-1", index);
        int s = send(client_reciever_sock[C->port - 5100], ptr, sizeof(struct NM_C_IP), 0);
        if (s < 0)
        {
            printf(RED "%d:Error sending ACK struct\n" DEFAULT, ACK_SEND_ERROR);
            log_command(server_ip[index], C->port, index + 5200, C->operation, "FAILURE");
        }
        else
        {
            printf(GREEN "SUCCESS\n" DEFAULT);
            log_command(server_ip[index], C->port, index + 5200, C->operation, "FILE DOESNT EXIST");
        }
        return;
    }
    // now that we have found a port where file exist

    LLNodePtr temp = ports_exist->first;
    while (temp != NULL && index == -1)
    {
        index = temp->val;
        temp = temp->next;
    }
    // now we have got the index where the port exists
    // so we sends this to the client
    // before sending we need to make server busy so that no other client can use it
    sem_wait(&block_server[index]);
    NM_C_IP_PTR ptr = fill_NM_C_IP(server_ip[index], index);
    int s = send(client_reciever_sock[C->port - 5100], ptr, sizeof(struct NM_C_IP), 0);
    if (s < 0)
    {
        printf(RED "%d:Error sending ACK struct\n" DEFAULT, ACK_SEND_ERROR);
        log_command(server_ip[index], C->port, index + 5200, C->operation, "FAILURE");
        return;
    }
    else
    {
        printf(GREEN "SUCCESS\n" DEFAULT);
    }
    struct COMMAND temp3;
    int bytesRead = recv(client_reciever_sock[C->port - 5100], &temp3, sizeof(struct COMMAND), 0);
    if (strcmp(temp3.operation, "stop") == 0)
    {
        printf(GREEN "CONNECTION STOPPED\n" DEFAULT);
    }
    else
    {
        printf(RED "%d:Some error while getting info" DEFAULT, INFO_ERROR);
        sem_post(&block_server[index]);
        log_command(server_ip[index], C->port, index + 5200, C->operation, "FAILURE");
        return;
    }
    sem_post(&block_server[index]);
    log_command(server_ip[index], C->port, index + 5200, C->operation, "SUCCESS");
}

void JustLS(COMMAND_PTR C)
{
    char *file = "ls";
    FILE *fptr = fopen(file, "w");
    char *dummy = (char *)malloc(sizeof(char) * 512);
    for (int i = 0; i < 512; i++)
        dummy[i] = '\0';
    printTriesFile(THP->head, strlen(dummy), fptr, dummy);
    fclose(fptr);
    LS_DATA_PTR st = init_ls_data();
    fptr = fopen(file, "r");
    fseek(fptr, 0, SEEK_END);
    int fileSize = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    int iterations = fileSize / 512;
    iterations++;
    // printf("%d\n", iterations);
    for (int i = 0; i < iterations; i++)
    {
        st->size = fread(st->buffer, 1, 512, fptr);
        st->no_packets = iterations;
        int s = send(client_reciever_sock[C->port - 5100], st, sizeof(struct ls_data), 0);
        if (s < 0)
        {
        }
        // printf("WHY\n");
    }
    fclose(fptr);
    remove(file);
    ACK_NM_C_PTR temp2;
    temp2 = fill_ACK_NM_C("YES");
    int s = send(client_reciever_sock[C->port - 5100], temp2, sizeof(struct ACK_NM_C), 0);
    if (s < 0)
    {
        printf(RED "%d:Error sending ACK struct\n" DEFAULT, ACK_SEND_ERROR);
    }
    else
    {
        printf(GREEN "SUCCESS\n" DEFAULT);
    }
}

void Lsfunctionality(COMMAND_PTR C)
{
    // printf("IN LS\n");
    printf("%s\n", C->path1);
    TriesPtr tp = (TriesPtr)malloc(sizeof(struct Tries));
    tp = THP->head;
    int len = strlen(C->path1);
    int flag = 0;
    if (strcmp(C->path1, "\0") == 0)
    {
        JustLS(C);
        return;
    }
    for (int i = 0; i < len; i++)
    {
        if (tp->alpha[C->path1[i]] == NULL)
        {
            flag = 1;
            break;
        }
        tp = tp->alpha[C->path1[i]];
    }
    // printf("This works till here\n");
    if (flag == 0 && tp->alpha['/'] == NULL)
        flag = 1;
    char *file = (char *)malloc(sizeof(char) * 512);
    strcpy(file, C->path1);
    // printf("%d Flag\n", flag);
    if (flag == 0)
    {
        char *dump = (char *)malloc(sizeof(char) * 512);
        strcpy(dump, C->path1);
        dump[strlen(dump) + 1] = '\0';
        dump[strlen(dump)] = '/';
        FILE *fptr = fopen(file, "w");
        printTriesFile(tp->alpha['/'], strlen(dump), fptr, dump);
        fclose(fptr);
        // printf("JOY\n");
        LS_DATA_PTR st = init_ls_data();
        fptr = fopen(file, "r");
        fseek(fptr, 0, SEEK_END);
        int fileSize = ftell(fptr);
        fseek(fptr, 0, SEEK_SET);
        int iterations = fileSize / 512;
        iterations++;
        // printf("%d\n", iterations);
        for (int i = 0; i < iterations; i++)
        {
            st->size = fread(st->buffer, 1, 512, fptr);
            st->no_packets = iterations;
            int s = send(client_reciever_sock[C->port - 5100], st, sizeof(struct ls_data), 0);
            if (s < 0)
            {
            }
            // printf("WHY\n");
        }
        fclose(fptr);
        remove(file);
    }
    else
        strcpy(file, "-1");
    // printf("ANGUISH\n");
    // printf("%s\n",file);

    ACK_NM_C_PTR temp2;
    if (flag == 0)
        temp2 = fill_ACK_NM_C("YES");
    else
        temp2 = fill_ACK_NM_C("NO");
    int s = send(client_reciever_sock[C->port - 5100], temp2, sizeof(struct ACK_NM_C), 0);
    if (s < 0)
    {
        printf(RED "%d:Error sending ACK struct\n" DEFAULT, ACK_SEND_ERROR);
    }
    else
    {
        printf(GREEN "SUCCESS\n" DEFAULT);
    }
}

void copyfunctionality(COMMAND_PTR C)
{
    // first search if the first file exists
    LLHeadPtr ports_exist = (LLHeadPtr)malloc(sizeof(struct LLHead));
    ports_exist->currSize = 0;
    ports_exist->first = NULL;
    int index = -1;
    LLHeadPtr ports_found = get(cache, C->path1);
    if (ports_found == NULL)
    {
        searchWords(THP, C->path1, ports_exist);
    }
    else
    {
        ports_exist = ports_found;
    }
    // then check if the second file exists
    LLHeadPtr ports_exist2 = (LLHeadPtr)malloc(sizeof(struct LLHead));
    ports_exist2->currSize = 0;
    ports_exist2->first = NULL;
    int index2 = -1;
    LLHeadPtr ports_found2 = get(cache, C->path2);
    if (ports_found2 == NULL)
    {
        searchWords(THP, C->path2, ports_exist2);
    }
    else
    {
        ports_exist2 = ports_found2;
    }
    // now we need to know if both of them are there are not
    if (ports_exist2->currSize == 0 || ports_exist->currSize == 0)
    {
        // so here we find out that one of the files dont exist
        ACK_NM_C_PTR temp2 = fill_ACK_NM_C("DONT");
        int s = send(client_reciever_sock[C->port - 5100], temp2, sizeof(struct ACK_NM_C), 0);
        if (s < 0)
        {
            printf(RED "%d:Error sending ACK to client\n" DEFAULT, ACK_SEND_ERROR);
            log_command("\0", C->port, -1, C->operation, "ERROR");
        }
        else
        {
            printf(GREEN "SUCCESS\n" DEFAULT);
            log_command("\0", C->port, -1, C->operation, "FILE/FOLDER already exists");
        }
        return;
    }
    // so now we know that the files exist in some storage server
    // so now we will read it from the SS
    // putting the current files into a cache
    put(cache, C->path1, ports_exist);
    put(cache, C->path2, ports_exist2);
    index = ports_exist->first->val;
    index2 = ports_exist2->first->val;
    // now we need to communicate with the relevant storage server to read the data
    COMMAND_PTR temp1 = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
    strcpy(temp1->operation, "copyfileread");
    strcpy(temp1->path1, C->path1);
    sem_wait(&block_server[index]);
    int s = send(server_reciever_sock[index], temp1, sizeof(struct COMMAND), 0);
    if (s < 0)
    {
        printf(RED "READ SEND ERROR\n" DEFAULT);
    }
    // now we will recieve data into a character buffer
    char siz[32];
    FILE *file = fopen("temp.txt", "ab");
    char *text = (char *)calloc(8192, sizeof(char));
    int r = recv(server_reciever_sock[index], siz, 32, 0);
    int size_f = atoi(siz);
    if (r < 0)
    {
        printf(RED "%d:Recieve Error\n" DEFAULT, RECIEVE_ERROR);
    }
    else
    {
        // now onto recieving the file
        size_f = ceil(((float)size_f) / 8192);
        for (int i = 0; i < size_f; i++)
        {
            int r = recv(server_reciever_sock[index], text, 8192, 0);
            if (r < 0)
            {
                printf(RED "%d:Recieve Error\n" DEFAULT, RECIEVE_ERROR);
            }
            text[strlen(text)] = '\0';
            fwrite(text, sizeof(char), r, file);
            memset(text, '\0', 8192);
            //printf("%d %d\n", i, size_f);
        }
    }
    fclose(file);
    sem_post(&block_server[index]);
    // now here i have to write the content to the other file
    char str[32];
    sprintf(str, "%d", size_f);
    strcpy(temp1->operation, "copyfilewrite");
    strcpy(temp1->path1, C->path2);
    strcpy(temp1->path2, str);
    sem_wait(&block_server[index2]);
    s = send(server_reciever_sock[index2], temp1, sizeof(struct COMMAND), 0);
    if (s < 0)
    {
        printf("%d:READ SEND ERROR\n", READ_SEND_ERROR);
    }
    // now i send the data to the other file
    FILE *file2 = fopen("temp.txt", "rb");
    char *buffer = (char *)calloc(8192, sizeof(char));
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, 8192, file2)) > 0)
    {
        int s = send(server_reciever_sock[index2], buffer, 8192, 0);
        if (s < 0)
        {
            printf("error\n");
        }
        //printf("data send\n");
        memset(buffer, '\0', 8192);
    }
    fclose(file2);
    remove("temp.txt");
    sem_post(&block_server[index2]);

    ACK_SS_NM_PTR write_ack = init_ACK_SS_NM();
    r = recv(server_reciever_sock[index2], write_ack, sizeof(struct ACK_SS_NM), 0);
    if (r < 0)
    {
        printf(RED "%d:Error receiving ack from server\n" DEFAULT, ACK_RECIEVE_ERROR_SS);
    }
    printf(GREEN "SS said HI" DEFAULT);

    // here to send the acknowledgement
    ACK_NM_C_PTR temp2 = fill_ACK_NM_C("YES");
    s = send(client_reciever_sock[C->port - 5100], temp2, sizeof(struct ACK_NM_C), 0);
    if (s < 0)
    {
        printf(RED "%d:Error sending ACK to client\n" DEFAULT, ACK_SEND_ERROR);
        log_command("\0", C->port, -1, C->operation, "ERROR");
    }
    else
    {
        printf(GREEN "SUCCESS\n" DEFAULT);
        log_command("\0", C->port, -1, C->operation, "FILE/FOLDER already exists");
    }
    return;
}

void copydirfunctionality(COMMAND_PTR C)
{
    // first i need to search whether both the given directories exist in my tries
    int c1 = searchSubstring(THP->head, C->path1);
    int c2 = searchSubstring(THP->head, C->path2);
    // here to send the acknowledgement
    if (c1 != 1 || c2 != 1)
    {
        // printf("wow\n");
        ACK_NM_C_PTR temp2 = fill_ACK_NM_C("DONT");
        int s = send(client_reciever_sock[C->port - 5100], temp2, sizeof(struct ACK_NM_C), 0);
        if (s < 0)
        {
            printf(RED "%d:Error sending ACK to client\n" DEFAULT, ACK_SEND_ERROR);
            log_command("\0", C->port, -1, C->operation, "ERROR");
        }
        else
        {
            printf(GREEN "SUCCESS\n" DEFAULT);
            log_command("\0", C->port, -1, C->operation, "FILE/FOLDER already exists");
        }
        return;
    }
    // now i need to go through the tries and find out the number of endpoint it has
    // get the filename and the port at which they are stored
    // now i iterate through all of them
    // i now create new filepaths C->path2/FILEPATH
    // now i read from C->path1/FILEPATH
    // write it into C->path2/FILEPATH
    TRIE_CONTENT_HEAD tch = LL_Tries_Ports_fn(THP, C->path1);
    LL_NODE_TRIE_CONTENT lnt = tch->first;
    while (lnt != NULL)
    {
         //printf("%s %d\n", lnt->val, lnt->ss_index);
        //  read lnt->val
        int checker=0;
        int index = lnt->ss_index;
        COMMAND_PTR temp1 = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
        strcpy(temp1->operation, "copyfileread");
        strcpy(temp1->path1, lnt->val);
        sem_wait(&block_server[index]);
        int s = send(server_reciever_sock[index], temp1, sizeof(struct COMMAND), 0);
        if (s < 0)
        {
            printf("%d:READ SEND ERROR\n", READ_SEND_ERROR);
        }
        FILE *file = fopen("temp.txt", "ab");
        // now we will recieve data into a character buffer
        char *siz = (char *)calloc(32, sizeof(char));
        char *text = (char *)calloc(8192, sizeof(char));
        int r = recv(server_reciever_sock[index], siz, 32, 0);
        int size_f = atoi(siz);
        if (r < 0)
        {
            printf(RED "%d:Recieve Error\n" DEFAULT, RECIEVE_ERROR);
        }
        else
        {
            // now onto recieving the file
            size_f = ceil(((float)size_f) / 8192);
            for (int i = 0; i < size_f; i++)
            {
                int r = recv(server_reciever_sock[index], text, 8192, 0);
                if (r < 0)
                {
                    printf(RED "%d:Recieve Error\n" DEFAULT, RECIEVE_ERROR);
                }
                if(strlen(text)>0)
                {
                    checker++;
                }
                text[strlen(text)] = '\0';
                fwrite(text, sizeof(char), r, file);
                memset(text, '\0', 8192);
                //printf("%d %d\n", i, size_f);
            }
        }
        fclose(file);
        // printf("TEXT IS: |%s|\n", text);
        sem_post(&block_server[index]);
        // createfiles with the C=>path2+lnt->val
        // printf("C-PATH IS |%s|\n", C->path2);
        char *newpath = (char *)calloc(512, sizeof(char));
        strcat(newpath, C->path2);
        strcat(newpath, "/");
        strcat(newpath, lnt->val);
        // printf("NEW PATH IS %s\n", newpath);
        if (strcmp(text, "DIR") == 0)
        {
            // then create a directory
            // and dont write anything
            COMMAND_PTR temp_C = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
            strcpy(temp_C->operation, "createdir");
            strcpy(temp_C->path1, newpath);
            strcpy(temp_C->path2, "\0");
            temp_C->port = C->port;
            int index1 = find_storage_server_port(); // should actually return 3 ports -- To be done
            // printf("IN CREATE DIR\n");
            if (index1 >= 0)
            {
                // printf("Index is %d\n", index1);
                //  we will now send struct C forward to SS
                sem_wait(&block_server[index1]);
                int s = send(server_reciever_sock[index1], temp_C, sizeof(struct COMMAND), 0);
                // printf("s is %d\n", s);
                if (s < 0)
                {
                    printf(RED "%d:Error sending instruction forward to SS\n" DEFAULT, SS_INSTRUCTION_FORWARD_ERROR);
                    log_command(server_ip[index1], C->port, index1, C->operation, "Error");
                    sem_post(&block_server[index1]);
                    return;
                }
                else if (s == 0)
                {
                    printf("Looks like SS %d has shut down!!!", index1);
                    log_command(server_ip[index1], C->port, index1, C->operation, "Error");
                    sem_post(&block_server[index1]);
                }
                else
                {
                    printf("WAITING for ACK now\n");
                    // we will now wait for an ACK from SS
                    ACK_SS_NM_PTR temp = init_ACK_SS_NM();
                    int r = recv(server_reciever_sock[index1], temp, sizeof(struct ACK_SS_NM), 0);
                    if (r < 0)
                    {
                        printf("ACK struct not recieved\n");
                        log_command(server_ip[index1], C->port, index, C->operation, "Error");
                        sem_post(&block_server[index1]);
                        return;
                    }
                    else if (r == 0)
                    {
                        printf("Looks like SS %d has shut down!!!", index1);
                        log_command(server_ip[index1], C->port, index1, C->operation, "Error");
                        sem_post(&block_server[index1]);
                    }
                    else
                    {
                        if (strcmp(temp->ticket, "YES") != 0)
                        {
                            printf(RED "%d:SS couldnt create the file/folder\n" DEFAULT, SS_CREATE_ERROR);
                            ACK_NM_C_PTR temp2 = fill_ACK_NM_C("NO");
                            s = send(client_reciever_sock[C->port - 5100], temp2, sizeof(struct ACK_NM_C), 0);
                            if (s < 0)
                            {
                                printf(RED "%d:Error sending ACK struct\n" DEFAULT, ACK_SEND_ERROR);
                                sem_post(&block_server[index1]);
                                return;
                            }
                            else
                            {
                                printf(GREEN "ACK send\n" DEFAULT);
                            }
                            sem_post(&block_server[index1]);
                            return;
                        }
                    }
                    // insertTries3(THP, newpath, index1, index1, index1);
                    char path[256] = "";
                    char *token = strtok(newpath, "/");
                    while (token != NULL)
                    {
                        strcat(path, token);

                        printf("%s\n", path); // Print or save to a temporary location as needed
                        token = strtok(NULL, "/");
                        insertTries3(THP, path, index1, index1, index1);
                        strcat(path, "/");
                    }
                    log_command(server_ip[index1], C->port, index1 + 5200, C->operation, "SUCCESS");
                    sem_post(&block_server[index1]);
                }
            }
        }
        else
        {
            // create a file and do whatever was normally being done
            COMMAND_PTR temp_C = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
            strcpy(temp_C->operation, "createfile");
            strcpy(temp_C->path1, newpath);
            strcpy(temp_C->path2, "\0");
            temp_C->port = C->port;
            int index1 = find_storage_server_port(); // should actually return 3 ports -- To be done
            // printf("IN CREATE FILE\n");
            if (index1 >= 0)
            {
                // printf("Index is %d\n", index1);
                //  we will now send struct C forward to SS
                sem_wait(&block_server[index1]);
                int s = send(server_reciever_sock[index1], temp_C, sizeof(struct COMMAND), 0);
                // printf("s is %d\n", s);
                if (s < 0)
                {
                    printf(RED "%d:Error sending instruction forward to SS\n" DEFAULT, SS_INSTRUCTION_FORWARD_ERROR);
                    log_command(server_ip[index1], C->port, index1, C->operation, "Error");
                    sem_post(&block_server[index1]);
                    return;
                }
                else if (s == 0)
                {
                    printf("Looks like SS %d has shut down!!!", index1);
                    log_command(server_ip[index1], C->port, index1, C->operation, "Error");
                    sem_post(&block_server[index1]);
                }
                else
                {
                    printf("WAITING for ACK now\n");
                    // we will now wait for an ACK from SS
                    ACK_SS_NM_PTR temp = init_ACK_SS_NM();
                    int r = recv(server_reciever_sock[index1], temp, sizeof(struct ACK_SS_NM), 0);
                    if (r < 0)
                    {
                        printf("ACK struct not recieved\n");
                        log_command(server_ip[index1], C->port, index, C->operation, "Error");
                        sem_post(&block_server[index1]);
                        return;
                    }
                    else if (r == 0)
                    {
                        printf("Looks like SS %d has shut down!!!", index1);
                        log_command(server_ip[index1], C->port, index1, C->operation, "Error");
                        sem_post(&block_server[index1]);
                    }
                    else
                    {
                        if (strcmp(temp->ticket, "YES") != 0)
                        {
                            printf(RED "%d:SS couldnt create the file/folder\n" DEFAULT, SS_CREATE_ERROR);
                            ACK_NM_C_PTR temp2 = fill_ACK_NM_C("NO");
                            s = send(client_reciever_sock[C->port - 5100], temp2, sizeof(struct ACK_NM_C), 0);
                            if (s < 0)
                            {
                                printf(RED "%d:Error sending ACK struct\n" DEFAULT, ACK_SEND_ERROR);
                                sem_post(&block_server[index1]);
                                return;
                            }
                            else
                            {
                                printf(GREEN "ACK send\n" DEFAULT);
                            }
                            sem_post(&block_server[index1]);
                            return;
                        }
                    }
                    // insertTries3(THP, newpath, index1, index1, index1);
                    char path[256] = "";
                    char copy_newpath[256];
                    strcpy(copy_newpath, newpath);
                    char *token = strtok(copy_newpath, "/");
                    while (token != NULL)
                    {
                        strcat(path, token);

                        printf("%s\n", path); // Print or save to a temporary location as needed
                        token = strtok(NULL, "/");
                        insertTries3(THP, path, index1, index1, index1);
                        strcat(path, "/");
                    }
                    log_command(server_ip[index1], C->port, index1 + 5200, C->operation, "SUCCESS");
                    sem_post(&block_server[index1]);
                    if (checker <= 0)
                    {
                        // dont write
                    }
                    else
                    {
                        // write onto C->port+lnt->val
                        // now here i have to write the content to the other file
                         printf("came till here\n");
                        char str[32];
                        sprintf(str, "%d", size_f);
                        COMMAND_PTR temp3 = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
                        strcpy(temp3->operation, "copyfilewrite");
                        strcpy(temp3->path1, newpath);
                        strcpy(temp3->path2, str);
                        temp3->port = C->port;
                        sem_wait(&block_server[index1]);
                        s = send(server_reciever_sock[index1], temp3, sizeof(struct COMMAND), 0);
                        // printf("s is %d\n", s);
                        if (s < 0)
                        {
                            printf("%d:READ SEND ERROR\n", READ_SEND_ERROR);
                        }

                        // now i send the data to the other file
                        FILE *file2 = fopen("temp.txt", "rb");
                        char *buffer = (char *)calloc(8192, sizeof(char));
                        size_t bytesRead;
                        while ((bytesRead = fread(buffer, 1, 8192, file2)) > 0)
                        {
                            int s = send(server_reciever_sock[index1], buffer, strlen(buffer), 0);
                            if (s < 0)
                            {
                                printf("error\n");
                            }
                            printf("data send %ld\n",bytesRead);
                            memset(buffer, '\0', 8192);
                        }
                        fclose(file2);
                        remove("temp.txt");
                        // printf("WAITING FOR WRITE ACK\n");
                        ACK_SS_NM_PTR write_ack = init_ACK_SS_NM();
                        r = recv(server_reciever_sock[index1], write_ack, sizeof(struct ACK_SS_NM), 0);
                        // printf("write r is %d\n", r);
                        if (r < 0)
                        {
                            printf(RED "%d:Error receiving ack from server\n" DEFAULT, ACK_RECIEVE_ERROR_SS);
                        }
                    }
                    printf(GREEN "SS said HI" DEFAULT);
                    sem_post(&block_server[index1]);
                }
            }
        }

        free(newpath);
        free(text);
        free(siz);
        // move onto the next entry
        lnt = lnt->next;
    }
    // this is to send the final acknowledgement
    ACK_NM_C_PTR temp2 = fill_ACK_NM_C("YES");
    int s = send(client_reciever_sock[C->port - 5100], temp2, sizeof(struct ACK_NM_C), 0);
    if (s < 0)
    {
        printf(RED "%d:Error sending ACK to client\n" DEFAULT, ACK_SEND_ERROR);
        log_command("\0", C->port, -1, C->operation, "ERROR");
    }
    else
    {
        printf(GREEN "SUCCESS\n" DEFAULT);
        log_command("\0", C->port, -1, C->operation, "FILE/FOLDER already exists");
    }
    return;
}
void command_proccessor(COMMAND_PTR C, int clientport)
{
    while (1)
    {
        sem_wait(&block_client[clientport]);
        sem_trywait(&red_lock);
        // sem_wait(&block_server[serverport]);
        //  first we will make it work for compare
        if ((strcmp("createfile", C->operation) == 0) || (strcmp("createdir", C->operation) == 0))
        {
            Createfunctionality(C);
        }
        else if (strcmp("deletefile", C->operation) == 0 || strcmp("deletedir", C->operation) == 0)
        {
            Deletefilefunctionality(C);
        }
        else if (strcmp("read", C->operation) == 0)
        {
            Readfunctionality(C);
        }
        else if (strcmp("write", C->operation) == 0)
        {
            Writefunctionality(C);
        }
        else if (strcmp("info", C->operation) == 0)
        {
            Infofunctionality(C);
        }
        else if (strcmp("ls", C->operation) == 0)
        {
            Lsfunctionality(C);
        }
        else if (strcmp("copyfile", C->operation) == 0)
        {
            copyfunctionality(C);
        }
        else if (strcmp("copydir", C->operation) == 0)
        {
            copydirfunctionality(C);
        }
        sem_post(&red_lock);
        sem_post(&block_client[clientport]);

        // sem_post(&block_server[serverport]);
        break;
    }
}

// this is to run each and every client and recieve its data
void *clientcommandtemporary(void *args)
{
    int index = *(int *)args;
    while (1)
    {
        printf(YELLOW "Waiting for clients %d inputs...\n" DEFAULT, index + 5100);
        struct COMMAND C;
        int bytesRead = recv(client_reciever_sock[index], &C, sizeof(struct COMMAND), 0);

        if (bytesRead < 0)
        {
            printf(RED "Error in recieving data from the Client...Shutting off Client %d\n", index + 5100);
            sem_wait(&client_lock);
            client_available[index] = 0;
            close(client_reciever_sock[index]);
            client_reciever_sock[index] = -1;
            sem_post(&client_lock);
            break;
        }
        else if (bytesRead == 0)
        {
            printf(BLUE "LOOKS LIKE CLIENT With port %d has disconnected\n" DEFAULT, index + 5100);
            sem_wait(&client_lock);
            client_available[index] = 0;
            close(client_reciever_sock[index]);
            client_reciever_sock[index] = -1;
            sem_post(&client_lock);
            break;
        }
        else if (bytesRead < sizeof(struct COMMAND))
        {
            printf(RED "Incomplete data received ending the client connection\n" DEFAULT);
            sem_wait(&client_lock);
            client_available[index] = 0;
            close(client_reciever_sock[index]);
            client_reciever_sock[index] = -1;
            sem_post(&client_lock);
            break;
        }
        else
        {
            COMMAND_PTR temp = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
            strcpy(temp->operation, C.operation);
            strcpy(temp->path1, C.path1);
            strcpy(temp->path2, C.path2);
            temp->port = C.port;
            command_proccessor(temp, temp->port - 5100);
        }

        // print_Customers(requests_queue);
        // Clear the structure for the next iteration

        sleep(1);
    }
}
void *empty_func(void *args)
{
    int x = *(int *)args;
    while (1)
    {
        char siz[3];
        int r = recv(server_reciever_sock2[x - 5200], siz, 3, 0);
        if (r < 0)
        {
            // do nothing
        }
        else
        {
            sem_wait(&block_server[x - 5200]);
            server_available[x - 5200] = 0;
            printf(ORANGE "Server at Port %d has disconnected\n" DEFAULT, x);
            close(server_reciever_sock[x - 5200]);
            close(server_reciever_sock2[x - 5200]);
            sem_post(&block_server[x - 5200]);
            break;
        }
    }
}
void *client_master_starter(void *args)
{
    // this is for the master port to get activated
    while (1)
    {
        sem_wait(&master_client_lock);
        socklen_t addr_size = sizeof(temp_client_addr);
        temp_client_sock = accept(master_client_sock, (struct sockaddr *)&temp_client_addr, &addr_size);
        if (temp_client_sock < 0)
        {
            printf(RED "%d:A client tried to connect but could not connect\n" DEFAULT, UNSUCCESSFUL_CLIENT_CONNECT);
            continue;
            // exit(1);
        }
        int index = find_a_free_port_for_client(temp_client_sock);
        if (index == -1)
        {
            printf("NO\n");
            continue;
        }
        sem_wait(&client_lock);
        addr_size = sizeof(client_addr[index]);
        client_reciever_sock[index] = accept(client_sock[index], (struct sockaddr *)&client_addr[index], &addr_size);
        if (client_reciever_sock[index] < 0)
        {
            printf(RED "%d:Client couldnt be assigned a port \n" DEFAULT, CLIENT_PORT_ASSIGN_ERROR);
            client_available[index] = 0;
            client_sock[index] = 0;
            continue;
            // exit(1);
        }
        sem_post(&client_lock);
        printf(GREEN "made connection with port %d\n" DEFAULT, index + 5100);
        int x = index;
        pthread_create(&client_threads[index], NULL, clientcommandtemporary, &x);
        sem_post(&master_client_lock);
    }
}
void tokenizeAndPrint(const char *input, const char *delimiter, int port)
{
    // Make a copy of the input string as strtok modifies the original string
    char *inputCopy = strdup(input);
    if (!inputCopy)
    {
        perror("strdup");
        return;
    }

    // Tokenize the input string
    char *token = strtok(inputCopy, delimiter);
    while (token != NULL)
    {
        // Print the token
        // printf("TOKEN: %s\n", token);
        insertTries3(THP, token, port, port, port);

        // Get the next token
        token = strtok(NULL, delimiter);
    }

    // Free the copy of the input string
    free(inputCopy);
}
TRIE_CONTENT_HEAD Copied_fn(TriesHeadPtr THP, char *val)
{
    TRIE_CONTENT_HEAD LLH = init_LL_Trie_Head();
    TriesPtr T = THP->head;
    for (int i = 0; i < strlen(val); i++)
    {
        if (T->alpha[val[i]] == NULL)
        {
            return LLH;
        }
        T = T->alpha[val[i]];
    }
    int j = strlen(val);
    Tries_Ports_fn(LLH, T, j, val);
    return LLH;
}

void *redundant_copy(void *args)
{
    // now this is to make the servers redundant
    // so basic logic is
    // delete the copy_folder and copy the main folder directly into it
    int index = *((int *)args); // index of the main server
    index = index - 5200;
    // now i need to find the index of the servers where i will store a copy

    int mod = index % 3;
    int index1, index2;
    char p1[6] = {'c', 'o', 'p', 'y', '\0', '\0'}; // names of the folder of the copy
    char p2[6] = {'c', 'o', 'p', 'y', '\0', '\0'};
    if (mod == 0)
    {
        index1 = index + 1;
        index2 = index + 2;
        p1[4] = '1';
        p2[4] = '1';
    }
    else if (mod == 1)
    {
        index1 = index - 1;
        index2 = index + 1;
        p1[4] = '1';
        p2[4] = '2';
    }
    else
    {
        index1 = index - 2;
        index2 = index - 1;
        p1[4] = '2';
        p2[4] = '2';
    }
    while (1)
    {
        sleep(120); // this will be done every 60 seconds
        if (server_available[index] == 0)
        {
            // this means that this server has closed
            return NULL;
        }
        sem_wait(&red_lock);
        sem_wait(&block_server[index1]);
        sem_wait(&block_server[index2]);
        sem_wait(&block_server[index]);
        printf("PORT %d has acquired all 3 locks\n", index);
        // so now i have got all the indexes
        // so first i need to send a command to both the storage servers copy folders to delete them
        // this will send it to the first server first
        if (server_available[index1] == 1)
        {

            COMMAND_PTR C = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
            strcpy(C->operation, "deletereddir");
            strcpy(C->path1, p1);
            strcpy(C->path2, "\0");
            C->port = -1;

            int s = send(server_reciever_sock[index1], C, sizeof(struct COMMAND), 0);
            if (s < 0)
            {
                printf("ERROR SENDING STUFF\n");
            }
        }
        if (server_available[index2] == 1)
        {
            COMMAND_PTR C = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
            strcpy(C->operation, "deletereddir");
            strcpy(C->path1, p2);
            strcpy(C->path2, "\0");
            C->port = -1;

            int s = send(server_reciever_sock[index2], C, sizeof(struct COMMAND), 0);
            if (s < 0)
            {
                printf("ERROR SENDING STUFF\n");
            }
        }

        // okay so now delete is done
        // now i need to send a copy of my folder to them
        // now we need to get all the file/directory paths in our folder
        TriesHeadPtr temp = initTries();
        if (server_available[index] == 1)
        {
            COMMAND_PTR C = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
            strcpy(C->operation, "getMain");
            strcpy(C->path1, "\0");
            strcpy(C->path2, "\0");
            C->port = -1;
            int s = send(server_reciever_sock[index], C, sizeof(struct COMMAND), 0);
            if (s < 0)
            {
                printf("ERROR SENDING STUFF\n");
            }
            char result[4096];
            int r = recv(server_reciever_sock[index], result, 4096, 0);
            if (r < 0)
            {
                printf("ERROR recieving STUFF\n");
            }
            // now i need to put it into a tries data structure

            char *inputCopy = strdup(result);
            if (!inputCopy)
            {
                perror("strdup");
                return NULL;
            }
            char *token = strtok(inputCopy, "\n");
            while (token != NULL)
            {
                insertTries3(temp, token, index, index, index);
                token = strtok(NULL, "\n");
            }
            free(inputCopy);

            // so now i have got the list of files with their ports
        }
        char *random = (char *)calloc(512, sizeof(char));
        strcpy(random, "");
        TRIE_CONTENT_HEAD tch = Copied_fn(temp, random);
        LL_NODE_TRIE_CONTENT lnt = tch->first;
        while (lnt != NULL)
        {
            // printf("%s %d\n", lnt->val, lnt->ss_index);
            //  to read the files
            COMMAND_PTR temp1 = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
            strcpy(temp1->operation, "getsizdir");
            strcpy(temp1->path1, lnt->val);
            int s = send(server_reciever_sock[index], temp1, sizeof(struct COMMAND), 0);
            if (s < 0)
            {
                printf("%d:READ SEND ERROR\n", READ_SEND_ERROR);
            }
            // now we will recieve data into a character buffer
            char *siz = (char *)calloc(32, sizeof(char));
            char *text = (char *)calloc(8192, sizeof(char));
            int r = recv(server_reciever_sock[index], siz, 32, 0);
            int size_f = atoi(siz);
            if (r < 0)
            {
                printf(RED "%d:Recieve Error\n" DEFAULT, RECIEVE_ERROR);
            }
            else
            {
                // now onto recieving the file
                size_f = ceil(((float)size_f) / 8192);
                for (int i = 0; i < 1; i++)
                {
                    int r = recv(server_reciever_sock[index], text, 8192, 0);
                    if (r < 0)
                    {
                        printf(RED "%d:Recieve Error\n" DEFAULT, RECIEVE_ERROR);
                    }
                }
            }
            // printf("TEXT IS: |%s|\n", text);

            char *newpath = (char *)calloc(512, sizeof(char));
            strcat(newpath, lnt->val);
            // printf("NEW PATH IS %s\n", newpath);
            if (strcmp(text, "DIR") == 0)
            {
                // printf("in createdir\n");
                //  to create a directory endpoint
                COMMAND_PTR temp_C = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
                strcpy(temp_C->operation, "createREDdir");
                strcpy(temp_C->path1, newpath);
                strcpy(temp_C->path2, p1);
                temp_C->port = -1;
                s = send(server_reciever_sock[index1], temp_C, sizeof(struct COMMAND), 0);
                if (s < 0)
                {
                    printf("send error");
                }
                COMMAND_PTR temp_C2 = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
                strcpy(temp_C2->operation, "createREDdir");
                strcpy(temp_C2->path1, newpath);
                strcpy(temp_C2->path2, p2);
                temp_C2->port = -1;
                s = send(server_reciever_sock[index2], temp_C2, sizeof(struct COMMAND), 0);
                if (s < 0)
                {
                    printf("send error");
                }
            }
            else
            {
                // printf("in createfile\n");
                //  to create a file endpoint
                COMMAND_PTR temp_C = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
                strcpy(temp_C->operation, "createREDfile");
                strcpy(temp_C->path1, newpath);
                strcpy(temp_C->path2, p1);
                temp_C->port = -1;
                s = send(server_reciever_sock[index1], temp_C, sizeof(struct COMMAND), 0);
                if (s < 0)
                {
                    printf("send error");
                }
                COMMAND_PTR temp_C2 = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
                strcpy(temp_C2->operation, "createREDfile");
                strcpy(temp_C2->path1, newpath);
                strcpy(temp_C2->path2, p2);
                temp_C2->port = -1;
                s = send(server_reciever_sock[index2], temp_C2, sizeof(struct COMMAND), 0);
                if (s < 0)
                {
                    printf("send error");
                }
                // if its a file then we should write the text also
                // so commands to write text here
                if (strlen(text) <= 0)
                {
                    // do nothing
                }
                else
                {
                    // write to that file
                    COMMAND_PTR temp1 = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
                    strcpy(temp1->operation, "copyfileread");
                    strcpy(temp1->path1, lnt->val);
                    int s = send(server_reciever_sock[index], temp1, sizeof(struct COMMAND), 0);
                    if (s < 0)
                    {
                        printf("%d:READ SEND ERROR\n", READ_SEND_ERROR);
                    }
                    free(text);
                    // now we will recieve data into a character buffer
                    char *siz = (char *)calloc(32, sizeof(char));
                    char *text = (char *)calloc(8192, sizeof(char));
                    int r = recv(server_reciever_sock[index], siz, 32, 0);
                    int size_f = atoi(siz);
                    if (r < 0)
                    {
                        printf(RED "%d:Recieve Error\n" DEFAULT, RECIEVE_ERROR);
                    }
                    else
                    {

                        // now onto recieving the file

                        size_f = ceil(((float)size_f) / 8192);
                        // printf("SIZE IS %d\n", size_f);
                        for (int i = 0; i < size_f; i++)
                        {
                            text = (char *)calloc(8192, sizeof(char));
                            printf("text is ---\n");
                            printf("%s", text);
                            printf("---------------------------------------\n");
                            int r = recv(server_reciever_sock[index], text, 8192, 0);
                            if (r < 0)
                            {
                                printf(RED "%d:Recieve Error\n" DEFAULT, RECIEVE_ERROR);
                            }
                            printf("text is ---\n");
                            printf("%s", text);
                            printf("---------------------------------------\n");
                            strcpy(temp_C->operation, "copyREDfile");
                            strcpy(temp_C2->operation, "copyREDfile");
                            s = send(server_reciever_sock[index1], temp_C, sizeof(struct COMMAND), 0);
                            // printf("s is %d\n", s);
                            if (s < 0)
                            {
                                printf("%d:READ SEND ERROR\n", READ_SEND_ERROR);
                            }
                            s = send(server_reciever_sock[index2], temp_C, sizeof(struct COMMAND), 0);
                            // printf("s is %d\n", s);
                            if (s < 0)
                            {
                                printf("%d:READ SEND ERROR\n", READ_SEND_ERROR);
                            }
                            s = send(server_reciever_sock[index1], text, strlen(text), 0);
                            // printf("s is %d\n", s);
                            if (s < 0)
                            {
                                printf("%d:READ SEND ERROR\n", READ_SEND_ERROR);
                            }
                            s = send(server_reciever_sock[index2], text, strlen(text), 0);
                            // printf("s is %d\n", s);
                            if (s < 0)
                            {
                                printf("%d:READ SEND ERROR\n", READ_SEND_ERROR);
                            }
                            memset(text, 0, 8192);
                            free(text);
                        }
                    }
                }
            }
            free(newpath);
            free(siz);
            lnt = lnt->next;
            // so recieving the text is done now, now i need to send it to the other things
            // now i need to go to copy of both index1 with p1 and index2 with p2 and create the filepath
        }
        printf("Server %d has released all locks\n", index);
        sem_post(&block_server[index]);
        sem_post(&block_server[index1]);
        sem_post(&block_server[index2]);
        sem_post(&red_lock);
    }

    // whatever ANNA DOES, i just assume i will get a list of them
}
void *server_master_starter(void *args)
{
    // this is for the master port to get activated
    while (1)
    {
        sem_wait(&master_server_lock);
        socklen_t addr_size = sizeof(temp_client_addr);
        temp_server_sock = accept(master_server_sock, (struct sockaddr *)&temp_client_addr, &addr_size);
        if (temp_server_sock < 0)
        {
            printf(RED "%d:A SS tried to connect but could not connect\n" DEFAULT, UNSUCCESSFUL_SS_CONNECT);
            continue;
            // exit(1);
        }
        SS_NM_PTR ss_nm = init();
        int bytesRead = recv(temp_server_sock, ss_nm, sizeof(struct SS_NM), 0);
        int index = ss_nm->port - 5200;
        strcpy(server_ip[index], ss_nm->ip);
        tokenizeAndPrint(ss_nm->result, "\n", index);
        // printTries(THP->head, 0);
        server_available[index] = 1;
        if (index < 0)
        {
            continue;
        }
        sem_wait(&block_server[index]);
        addr_size = sizeof(server_addr[index]);
        server_reciever_sock[index] = accept(server_sock[index], (struct sockaddr *)&server_addr[index], &addr_size);
        if (server_reciever_sock[index] < 0)
        {
            printf(RED "%d:Naming server couldnt be assigned a port \n" DEFAULT, NS_PORT_ASSIGN_ERROR);
            server_available[index] = 0;
            // continue;
            //  exit(1);
        }
        addr_size = sizeof(server_addr2[index]);
        server_reciever_sock2[index] = accept(server_sock2[index], (struct sockaddr *)&server_addr2[index], &addr_size);
        if (server_reciever_sock2[index] < 0)
        {
            printf(RED "%d:Naming server couldnt be assigned a port \n" DEFAULT, NS_PORT_ASSIGN_ERROR);
            server_available[index] = 0;
            // continue;
            //  exit(1);
        }
        printf(GREEN "made connection with port %d\n" DEFAULT, ss_nm->port);
        pthread_create(&server_threads[index], NULL, empty_func, &ss_nm->port);
        pthread_create(&server_redundancy[index], NULL, redundant_copy, &ss_nm->port);
        sem_post(&block_server[index]);

        sem_post(&master_server_lock);
    }
}

int main()
{
    printf(YELLOW "Initializing server....\n" DEFAULT);
    // Register the signal handler for SIGINT (Ctrl+C)
    if (signal(SIGINT, handle_sigint_cntrlC) == SIG_ERR)
    {
        perror(RED "Error initializing server!!!\n" DEFAULT);
        return 1;
    }
    server_initializer();
    printf(GREEN "Server Ready\n" DEFAULT);
    server_master_thread = (pthread_t *)malloc(sizeof(pthread_t));
    server_threads = (pthread_t *)malloc(sizeof(pthread_t) * (SERVER_PORT));
    client_master_thread = (pthread_t *)malloc(sizeof(pthread_t));
    client_threads = (pthread_t *)malloc(sizeof(pthread_t) * (CLIENT_PORT));
    queue_processor = (pthread_t *)malloc(sizeof(pthread_t));
    server_redundancy = (pthread_t *)malloc(sizeof(pthread_t) * (SERVER_PORT));
    // before everything i need to make sure all the storage servers are up and running, or try to get them runnning
    // so what i will do is that i will pre-assign each port to a storage server, that way it would easier to keep
    // track of storage servers if they are up or down
    // first step is to create the master socket for client and server
    create_tcp_socket(&master_server_sock);
    connect_to_client(ip, MASTER_SERVER_PORT, &master_server_sock);
    create_tcp_socket(&master_client_sock);
    connect_to_client(ip, MASTER_CLIENT_PORT, &master_client_sock);

    pthread_create(&client_master_thread[0], NULL, client_master_starter, NULL);
    pthread_create(&server_master_thread[0], NULL, server_master_starter, NULL);
    // pthread_create(queue_processor, NULL, queue_processorfn, NULL);

    // this is to join all the threads which have been created
    pthread_join(client_master_thread[0], NULL);
    pthread_join(server_master_thread[0], NULL);
    for (int i = 0; i < SERVER_PORT; i++)
    {
        if (server_available[i] == 1)
        {
            pthread_join(server_threads[i], NULL);
            pthread_join(server_redundancy[i], NULL);
        }
    }
    for (int i = 0; i < SERVER_PORT; i++)
    {
        if (client_available[i] == 1)
        {
            pthread_join(client_threads[i], NULL);
        }
    }
}
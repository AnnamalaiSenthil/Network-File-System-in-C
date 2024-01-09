#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "queue.h"
#include "structs.h"
#include <ctype.h>
#include <sys/stat.h>
#include <math.h>
#include "macros.h"
#include "errorcodes.h"

#define MASTER_CLIENT_PORT 5000
#define IP_ADDRESS "127.0.0.1"
char buffer[1024];
int port = -1;

// we create the socket which will first interact with the server through the master port
int server_sock;
struct sockaddr_in addr;
// sockets for storage server
int storage_server_sock;
struct sockaddr_in storage_addr;

void getFileInformation(struct stat file_stat)
{

    printf("File Size: %ld bytes\n", file_stat.st_size);
    printf("Owner UID: %d\n", file_stat.st_uid);
    printf("Group GID: %d\n", file_stat.st_gid);
    printf("Access Permissions: %o\n", file_stat.st_mode & 0777);
    printf("Last Access Time: %ld\n", file_stat.st_atime);
    printf("Last Modification Time: %ld\n", file_stat.st_mtime);
    printf("Last Status Change Time: %ld\n", file_stat.st_ctime);
}

void trimString(char *str)
{
    size_t len = strlen(str);
    // Remove trailing whitespaces and newline characters
    while (len > 0 && (isspace((unsigned char)str[len - 1]) || str[len - 1] == '\n'))
    {
        len--;
    }
    // Find the starting position of the non-whitespace part
    size_t start = strspn(str, " \t\r\n");
    // Shift the non-whitespace part to the beginning of the string
    memmove(str, str + start, len - start);
    // Null-terminate the trimmed string
    str[len - start] = '\0';
}

void recieve_data_from_server(int sock)
{
    bzero(buffer, 1024);
    int r_error = recv(sock, buffer, sizeof(buffer), 0);
    if (r_error < 0)
    {
        perror("[-]Recieve error");
        exit(1);
    }
    printf("Server: %s\n", buffer);
}
int connector_port(int sock)
{
    recieve_data_from_server(sock);
    return atoi(buffer);
}
void send_data_to_server(int sock, char *response)
{
    printf("IN HERE\n");
    bzero(buffer, 1024);
    strcpy(buffer, response);
    printf("Client: %s\n", buffer);
    int s_error = send(sock, buffer, strlen(buffer), 0);
    if (s_error < 0)
    {
        perror("[-]Send error");
        exit(1);
    }
}
void create_tcp_socket(int *server_sock, struct sockaddr_in *addr, int port)
{
    *server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (*server_sock < 0)
    {
        perror("[-]Socket error");
        exit(1);
    }

    memset(addr, '\0', sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port); // Convert port to network byte order
    addr->sin_addr.s_addr = inet_addr(IP_ADDRESS);
    if (addr->sin_addr.s_addr == INADDR_NONE)
    {
        perror("[-]Address error");
        exit(1);
    }
}
void write_function(COMMAND_PTR C)
{
    NM_C_IP_PTR temp = init_NM_C_IP();
    int r = recv(server_sock, temp, sizeof(struct NM_C_IP), 0);
    if (r < 0)
    {
        printf("struct not recieved\n");
    }
    else
    {
        // now i need to make a connection with the SS
        if (temp->index == -1)
        {
            printf(RED "%d:File doesnt exist\n" DEFAULT,FILE_DOESNT_EXIST);
            return;
        }
        int c_error = -1;
        create_tcp_socket(&storage_server_sock, &storage_addr, 5300 + temp->index);
        int count = 0;
        while (c_error < 0)
        {
            c_error = connect(storage_server_sock, (struct sockaddr *)&storage_addr, sizeof(storage_addr));

            if (count >= 4)
            {
                printf(RED "COULDNT CONNECT TO STORAGE SERVER\n" DEFAULT);
                COMMAND_PTR Clo = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
                strcpy(Clo->operation, "stop");
                strcpy(Clo->path1, "\0");
                strcpy(Clo->path2, "\0");
                int sn = send(server_sock, Clo, sizeof(struct COMMAND), 0);
                close(storage_server_sock);
                return;
            }
            else if (c_error < 0 && count < 4)
            {
                printf(ORANGE "Looks Like the Storage server is offline :(Retrying in 2 seconds %d\n" DEFAULT, count);
                sleep(2);
                count++;
                // exit(1);
            }
            else
            {
                // first we will send the fileInfofunctionality(C) name
                int s = send(storage_server_sock, C, sizeof(struct COMMAND), 0);
                if (s < 0)
                {
                    printf(RED "%d:Commands couldnt be forwarded to Storage server \n" DEFAULT,SS_INSTRUCTION_FORWARD_ERROR);
                }
                {
                    printf(ORANGE "Enter the Text you want to write $->\n" DEFAULT);
                    char text[8192];
                    while (fgets(text, 8192, stdin) == NULL)
                    {
                    }
                    int s = send(storage_server_sock, text, strlen(text), 0);
                    if (s < 0)
                    {
                        printf(RED "%d:struct not send\n" DEFAULT,STRUCT_SEND_ERROR);
                    }
                    else
                    {
                        // do nothing
                    }
                }
                // now to close the connection
                COMMAND_PTR Clo = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
                strcpy(Clo->operation, "stop");
                strcpy(Clo->path1, "\0");
                strcpy(Clo->path2, "\0");
                s = send(server_sock, Clo, sizeof(struct COMMAND), 0);
                close(storage_server_sock);
                printf(BLUE "Connection with storage server closed\n" DEFAULT);
            }
        }
    }
}

void info_function(COMMAND_PTR C)
{
    NM_C_IP_PTR temp = init_NM_C_IP();
    int r = recv(server_sock, temp, sizeof(struct NM_C_IP), 0);
    if (r < 0)
    {
        printf("struct not recieved\n");
    }
    else
    {
        // now i need to make a connection with the SS
        if (temp->index == -1)
        {
            printf(RED "%d:File doesnt exist\n" DEFAULT,FILE_DOESNT_EXIST);
            return;
        }
        // now i need to make a connection with the SS

        int c_error = -1;
        create_tcp_socket(&storage_server_sock, &storage_addr, 5300 + temp->index);
        int count = 0;
        while (c_error < 0)
        {
            c_error = connect(storage_server_sock, (struct sockaddr *)&storage_addr, sizeof(storage_addr));

            if (count >= 4)
            {
                printf(RED "%d:COULDNT CONNECT TO STORAGE SERVER\n" DEFAULT,UNSUCCESSFUL_CLIENT_TO_SS_CONNECT);
                COMMAND_PTR Clo = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
                strcpy(Clo->operation, "stop");
                strcpy(Clo->path1, "\0");
                strcpy(Clo->path2, "\0");
                int sn = send(server_sock, Clo, sizeof(struct COMMAND), 0);
                close(storage_server_sock);
                return;
            }
            else if (c_error < 0 && count < 4)
            {
                printf(ORANGE "Looks Like the Storage server is offline :(Retrying in 2 seconds %d\n" DEFAULT, count);
                sleep(2);
                count++;
                // exit(1);
            }
            else
            {
                // first we will send the Infofunctionality(C) name
                int s = send(storage_server_sock, C, sizeof(struct COMMAND), 0);
                if (s < 0)
                {
                    printf(RED "%d:Commands couldnt be forwarded to Storage server \n" DEFAULT,SS_INSTRUCTION_FORWARD_ERROR);
                }
                else
                {
                    struct stat file_stat;
                    int r = recv(storage_server_sock, &file_stat, sizeof(struct stat), 0);
                    if (r < 0)
                    {
                        printf(RED "%d:recieve error\n" DEFAULT,RECIEVE_ERROR);
                    }
                    getFileInformation(file_stat);
                }
                // now to close the connection
                COMMAND_PTR Clo = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
                strcpy(Clo->operation, "stop");
                strcpy(Clo->path1, "\0");
                strcpy(Clo->path2, "\0");
                s = send(server_sock, Clo, sizeof(struct COMMAND), 0);
                close(storage_server_sock);
                printf(BLUE "Connection with storage server closed\n" DEFAULT);
            }
        }
    }
}
void read_function(COMMAND_PTR C)
{
    NM_C_IP_PTR temp = init_NM_C_IP();
    int r = recv(server_sock, temp, sizeof(struct NM_C_IP), 0);
    if (r < 0)
    {
        printf("struct not recieved\n");
    }
    else
    {
        // now i need to make a connection with the SS
        if (temp->index == -1)
        {
            printf(RED "%d:File doesnt exist\n" DEFAULT,FILE_DOESNT_EXIST);
            return;
        }
        int c_error = -1;
        create_tcp_socket(&storage_server_sock, &storage_addr, 5300 + temp->index);
        int count = 0;
        while (c_error < 0)
        {
            c_error = connect(storage_server_sock, (struct sockaddr *)&storage_addr, sizeof(storage_addr));

            if (count >= 4)
            {
                printf(RED "%d:COULDNT CONNECT TO STORAGE SERVER\n" DEFAULT,UNSUCCESSFUL_CLIENT_TO_SS_CONNECT);
                COMMAND_PTR Clo = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
                strcpy(Clo->operation, "stop");
                strcpy(Clo->path1, "\0");
                strcpy(Clo->path2, temp->extra);
                int sn = send(server_sock, Clo, sizeof(struct COMMAND), 0);
                close(storage_server_sock);
                return;
            }
            else if (c_error < 0 && count < 4)
            {
                printf(ORANGE "Looks Like the Storage server is offline :(Retrying in 2 seconds %d\n" DEFAULT, count);
                sleep(2);
                count++;
                // exit(1);
            }
            else
            {
                // first we will send the file name
                strcpy(C->path2, temp->extra);
                int s = send(storage_server_sock, C, sizeof(struct COMMAND), 0);
                if (s < 0)
                {
                    printf(RED "%d:Commands couldnt be forwarded to Storage server \n" DEFAULT,SS_INSTRUCTION_FORWARD_ERROR);
                }
                else
                {
                    // now we will recieve the size
                    char siz[32];
                    char text[8192];
                    int r = recv(storage_server_sock, siz, 32, 0);
                    int size_f = atoi(siz);
                    if (r < 0)
                    {
                        printf(RED "%d:recieve error\n" DEFAULT,RECIEVE_ERROR);
                    }
                    else
                    {
                        // now onto recieving the file
                        size_f = ceil(((float)size_f) / 8192);
                        for (int i = 0; i < size_f; i++)
                        {
                            int r = recv(storage_server_sock, text, 8192, 0);
                            if (r < 0)
                            {
                                printf(RED "%d:recieve error\n" DEFAULT,RECIEVE_ERROR);
                            }
                            printf("%s", text);
                        }
                    }
                    // now to close the connection
                    COMMAND_PTR Clo = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
                    strcpy(Clo->operation, "stop");
                    strcpy(Clo->path1, "\0");
                    strcpy(Clo->path2, "\0");
                    s = send(server_sock, Clo, sizeof(struct COMMAND), 0);
                    close(storage_server_sock);
                    printf(BLUE "Connection with storage server closed\n" DEFAULT);
                }
            }
        }
    }
}

void copy_function(COMMAND_PTR C)
{
    ACK_NM_C_PTR temp = init_ACK_NM_C();
    int r = recv(server_sock, temp, sizeof(struct ACK_NM_C), 0);
    if (r < 0)
    {
        printf("Struct not recieved\n");
    }
    else
    {
        if (strcmp(temp->ticket, "DONT") == 0)
        {
            printf(BLUE "FILE PATH DOESNT EXISTS\n" DEFAULT);
        }
        else if (strcmp(temp->ticket, "YES") == 0)
        {
            printf(GREEN "SUCCESS\n" DEFAULT);
        }
        else
        {
            printf(RED "%d:Error in executing command\n" DEFAULT,EXECUTE_COMMAND_ERROR);
        }
    }
}

void copydir_function(COMMAND_PTR C)
{
    ACK_NM_C_PTR temp = init_ACK_NM_C();
    int r = recv(server_sock, temp, sizeof(struct ACK_NM_C), 0);
    if (r < 0)
    {
        printf("Struct not recieved\n");
    }
    else
    {
        if (strcmp(temp->ticket, "DONT") == 0)
        {
            printf(BLUE "DIR PATH DOESNT EXISTS\n" DEFAULT);
        }
        else if (strcmp(temp->ticket, "YES") == 0)
        {
            printf(GREEN "SUCCESS\n" DEFAULT);
        }
        else
        {
            printf(RED "%d:Error in executing command\n" DEFAULT,EXECUTE_COMMAND_ERROR);
        }
    }
}

int main()
{
    printf(BLUE "\n Welcome to Team-55 NFS.\n" DEFAULT);
    printf(YELLOW "THE LIST OF COMMANDS ARE read,write,createfile,createdir,deletefile,deletedir,copyfile,copydir,info\n" DEFAULT);
    // we create the socket for the master port
    create_tcp_socket(&server_sock, &addr, MASTER_CLIENT_PORT);
    // now we wait or the initial connection with the server
    int c_error = -1;
    while (c_error < 0 && port < 0)
    {
        c_error = connect(server_sock, (struct sockaddr *)&addr, sizeof(addr));
        if (c_error < 0)
        {
            printf(RED "%d:Looks Like the server is offline :(Retrying to connect in 5 seconds\n" DEFAULT,NAMING_SERVER_OFFLINE);
            sleep(5);
            continue;
            // exit(1);
        }
        // we recieve the port with which we need to communicate
        port = connector_port(server_sock);
    }

    c_error = -1;
    // we close the connection with the master port
    close(server_sock);
    // now we create a new tcp socket for the new connection
    create_tcp_socket(&server_sock, &addr, port);
    while (c_error < 0)
    {
        c_error = connect(server_sock, (struct sockaddr *)&addr, sizeof(addr));
        if (c_error < 0)
        {
            printf(RED "%d:Looks Like the server is offline :(Retrying in 2 seconds\n" DEFAULT,NAMING_SERVER_OFFLINE);
            sleep(2);
            // exit(1);
        }
    }
    printf(GREEN "Connected to the server.\n" DEFAULT);
    printf(YELLOW "Connected via port: %d\n" DEFAULT, port);
    // A terminal for the client where COMMANDs can be isued like read, write, copy ....
    // Takes input and then receives the port number to communicate with the required storage server
    while (1)
    {
        printf(YELLOW "$------------> " DEFAULT);
        fflush(stdout);
        char inp[4096];
        while (fgets(inp, 4096, stdin) == NULL)
        {
        }
        char args[20][500];
        int count = 0;
        int c = 0;
        int array[20];
        // breaking down the given input COMMAND
        for (int i = 0; i < 4096; i++)
        {
            if (inp[i] == '\n' || inp[i] == '\0')
            {
                if (count == 0)
                    break;
                c++;
                array[c - 1] = count;
                count = 0;
                break;
            }
            if ((inp[i] == ' ' && count != 0) || (inp[i] == '\t' && count != 0))
            {
                c++;
                array[c - 1] = count;
                count = 0;
                continue;
            }
            if (inp[i] != ' ' && inp[i] != '\t')
            {
                args[c][count++] = inp[i];
            }
        }
        COMMAND_PTR C = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
        strcpy(C->operation, args[0]);
        strcpy(C->path1, args[1]);
        C->port = port;
        if (c != 2)
        {
            strcpy(C->path2, args[2]);
        }
        else
        {
            C->path2[0] = '\0';
        }
        C->port = port;
        int fl = 0;
        if (strcmp(C->operation, "copydir") == 0)
        {
            char *p1 = (char *)calloc(512, sizeof(char));
            char *p2 = (char *)calloc(512, sizeof(char));
            strcpy(p1, C->path1);
            strcpy(p2, C->path2);
            int flag = 1;
            if (strcmp(p1, p2) == 0)
            {
                //printf(RED "CANNOT COPY FROM SAME PLACE TO SAME PLACE\n" DEFAULT);
                flag = 0;
            }
            else
            {
                flag = 0;
                p1[strlen(p1) + 1] = '\0';
                p1[strlen(p1)] = '/';
                p2[strlen(p2) + 1] = '\0';
                p2[strlen(p2)] = '/';

                int minimum = strlen(p1);
                if (strlen(p2) < minimum)
                    minimum = strlen(p2);
                for (int j = 0; j < minimum; j++)
                {
                    if (p1[j] != p2[j])
                        flag = 1;
                    if (flag)
                        break;
                }
            }
            if (!flag)
            {
                printf(RED "CANNOT COPY FROM SAME PLACE TO SAME PLACE\n" DEFAULT);
                fl = 1;
            }
        }
        if (strcmp(C->operation, "copyfile") == 0 && strcmp(C->path1, C->path2) == 0)
            fl = 1;
        if (fl)
        {
            memset(args, 0, sizeof(args));
            continue;
        }
        int s = send(server_sock, C, sizeof(struct COMMAND), 0);
        if (s < 0)
        {
            printf(RED "%d:Error in sending command over to the server\n" DEFAULT,SEND_COMMAND_ERROR);
            close(server_sock);
            printf(RED "Closing the connection with the Server, Try to restart the client\n" DEFAULT);
            exit(0);
        }
        if (s == 0)
        {
            printf(YELLOW "LOOKS LIKE NM HAS DISCONNECTEDr\n" DEFAULT);
            close(server_sock);
            printf(RED "Closing the connection with the Server, Try to restart the client\n" DEFAULT);
            exit(0);
        }
        // when command is createfile, createdir, deletefile, deletedir
        else if (strcmp("createfile", C->operation) == 0 || strcmp("createdir", C->operation) == 0 || strcmp("deletefile", C->operation) == 0 || strcmp("deletedir", C->operation) == 0)
        {
            ACK_NM_C_PTR temp = init_ACK_NM_C();
            int r = recv(server_sock, temp, sizeof(struct ACK_NM_C), 0);
            if (r < 0)
            {
                printf("Struct not recieved\n");
            }
            else
            {
                if (strcmp(temp->ticket, "ALREADY") == 0)
                {
                    printf(BLUE "FILE PATH ALREADY EXISTS\n" DEFAULT);
                }
                else if (strcmp(temp->ticket, "YES") == 0)
                {
                    printf(GREEN "SUCCESS\n" DEFAULT);
                }
                else
                {
                    printf(RED "%d:Error in executing command\n" DEFAULT,EXECUTE_COMMAND_ERROR);
                }
            }
        }
        else if (strcmp("read", C->operation) == 0)
        {
            // printf("IN READ PATH IS %s|%s|%s|\n",C->operation,C->path1,C->path2);
            read_function(C);
        }
        else if (strcmp("stop", C->operation) == 0)
        {
            close(storage_server_sock);
            printf("Connection with storage server closed");
        }
        else if (strcmp("write", C->operation) == 0)
        {
            write_function(C);
        }
        else if (strcmp("info", C->operation) == 0)
        {
            info_function(C);
        }
        else if (strcmp("ls", C->operation) == 0)
        {
            LS_DATA_PTR st = init_ls_data();
            int r = recv(server_sock, st, sizeof(struct ls_data), 0);
            for (int j = 1; j < st->no_packets; j++)
            {
                printf("%s", st->buffer);
                r = recv(server_sock, st, sizeof(struct ls_data), 0);
            }
            printf("%s", st->buffer);
            ACK_NM_C_PTR temp = init_ACK_NM_C();
            r = recv(server_sock, temp, sizeof(struct ACK_NM_C), 0);
            // printf("WhY%s \n",temp->tic/ket);
            if (r < 0)
            {
                printf("Struct not recieved\n");
            }
            else
            {
            }
        }
        else if (strcmp("copyfile", C->operation) == 0)
        {
            copy_function(C);
        }
        else if (strcmp("copydir", C->operation) == 0)
        {
            copydir_function(C);
        }
        else if (strcmp("clear", C->operation) == 0)
        {
            system("clear");
        }
        else
        {
            printf(RED "WRONG COMMAND\n" DEFAULT);
        }
        sleep(1);
        free(C);
        memset(args, 0, sizeof(args));
    }
    close(server_sock);
    return 0;
}

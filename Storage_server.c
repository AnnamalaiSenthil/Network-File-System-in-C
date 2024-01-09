#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "server.h"
#include "structs.h"
#include "queue.h"
#include "Tries.h"
#include "SS_functions.h"
#include "pthread.h"
#include "macros.h"
#include <signal.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <math.h>
#include <semaphore.h>
#include "errorcodes.h"

#define MASTER_SERVER_PORT 5001
#define IP_ADDRESS "127.0.0.1"
char buffer[1024];
int port = -1;
int server_sock, server_sock2;
struct sockaddr_in addr;
int port2 = -1;
char currentWorkingDirectory[256];
char result[4096];
int server_listening_sock;
struct sockaddr_in server_listening_addr;
#define RESULT_SIZE 4096
#define CHUNK_SIZE 1024 // Adjust the chunk size as needed
sem_t lock;
sem_t del_lock;

int getLocalIpAddress(char *ipBuffer, size_t bufferSize)
{
    struct ifaddrs *ifap, *ifa;

    if (getifaddrs(&ifap) == -1)
        return -1;

    for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr != NULL && ifa->ifa_addr->sa_family == AF_INET)
        {
            const char *ip = inet_ntoa(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr);
            if (ip != NULL && snprintf(ipBuffer, bufferSize, "%s", ip) > 0)
            {
                freeifaddrs(ifap);
                return 0;
            }
        }
    }

    freeifaddrs(ifap);
    return -1; // No suitable address found
}

void appendToResult(const char *path)
{
    // Check if there is enough space in the result buffer
    if (strlen(result) + strlen(path) + 2 <= RESULT_SIZE)
    {
        // Append the path to the result string without the initial `./`
        strncat(result, path + 2, RESULT_SIZE - strlen(result) - 1);
        strncat(result, "\n", RESULT_SIZE - strlen(result) - 1);
    }
    else
    {
        fprintf(stderr, "Result buffer is too small.\n");
        // Handle the error as needed (e.g., return an error code or exit)
        exit(EXIT_FAILURE);
    }
}

void listFilesRecursively(const char *basePath)
{

    chdir(currentWorkingDirectory);
    chdir("main");
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(basePath)) != NULL)
    {
        while ((entry = readdir(dir)) != NULL)
        {
            char path[256]; // Adjust the size as needed
            snprintf(path, sizeof(path), "%s/%s", basePath, entry->d_name);

            if (entry->d_type == DT_DIR)
            {
                // Ignore directories starting with a dot
                if (entry->d_name[0] != '.')
                {
                    // Append the directory path to the result string
                    appendToResult(path);

                    // Recursively list files in the subdirectory
                    listFilesRecursively(path);
                }
            }
            else
            {
                // It's a file
                // Append the file path to the result string
                appendToResult(path);
            }
        }

        closedir(dir);
    }
    else
    {
        perror("opendir");
    }
    chdir(currentWorkingDirectory);
}

int countFilesInDirectory(const char *path)
{
    chdir(currentWorkingDirectory);
    chdir("main");
    DIR *dir;
    struct dirent *entry;
    struct stat fileStat;
    int count = 0;

    // Open the directory
    dir = opendir(path);

    // Check if the directory can be opened
    if (dir == NULL)
    {
        perror("Error opening directory");
        return -1;
    }

    // Read each entry in the directory
    while ((entry = readdir(dir)) != NULL)
    {
        // Get the file information
        stat(entry->d_name, &fileStat);

        // Check if the entry is a regular file
        if (S_ISREG(fileStat.st_mode))
        {
            count++;
        }
    }

    // Close the directory
    closedir(dir);
    chdir(currentWorkingDirectory);
    return count;
}

void MainCount()
{
    memset(result, 0, 4096);
    listFilesRecursively(".");
}
void getsiz_isdir(const char *file_path, int socket)
{
    // printf("IN PROCESS\n");
    chdir(currentWorkingDirectory);
    chdir("main");
    // i need to check if it is a file or a directory
    struct stat fileStat;
    if (stat(file_path, &fileStat) == 0)
    {
        if (S_ISDIR(fileStat.st_mode))
        {
            // to send if the given stuff is a directory
            long file_size = 1;
            char siz[32];
            sprintf(siz, "%ld", file_size);
            int s = send(socket, siz, 32, 0);
            if (s < 0)
            {
                printf("error\n");
            }
            char *buffer = (char *)calloc(8192, sizeof(char));
            strcpy(buffer, "DIR");
            s = send(socket, buffer, 8192, 0);
            if (s < 0)
            {
                printf("error\n");
            }
            chdir(currentWorkingDirectory);
        }
        else
        {
            // if it is a regular file then lite
            FILE *file = fopen(file_path, "rb");

            if (file == NULL)
            {
                perror("Error opening file");
                return;
            }

            // Seek to the end of the file to get its size
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            rewind(file);

            char siz[32];
            sprintf(siz, "%ld", file_size);
            int s = send(socket, siz, 32, 0);
            if (s < 0)
            {
                printf("error\n");
            }
            // Read and send file in chunks
            char *buffer = (char *)malloc(8192);

            if (buffer == NULL)
            {
                perror("Memory allocation error");
                fclose(file);
                return;
            }

            strcpy(buffer, "FILE");
            s = send(socket, buffer, 8192, 0);
            if (s < 0)
            {
                printf("error\n");
            }
            chdir(currentWorkingDirectory);
            free(buffer);
            fclose(file);
            chdir(currentWorkingDirectory);
        }
    }

    // printf("OUT OF PROCESS\n");
}
void process_file(const char *file_path, int socket, const char *path)
{
    // printf("IN PROCESS\n");
    chdir(currentWorkingDirectory);
    chdir(path);
    // i need to check if it is a file or a directory
    struct stat fileStat;
    if (stat(file_path, &fileStat) == 0)
    {
        if (S_ISDIR(fileStat.st_mode))
        {
            // to send if the given stuff is a directory
            long file_size = 1;
            char siz[32];
            sprintf(siz, "%ld", file_size);
            int s = send(socket, siz, 32, 0);
            if (s < 0)
            {
                printf("error\n");
            }
            char *buffer = (char *)calloc(8192, sizeof(char));
            strcpy(buffer, "DIR");
            s = send(socket, buffer, 8192, 0);
            if (s < 0)
            {
                printf("error\n");
            }
            chdir(currentWorkingDirectory);
        }
        else
        {
            // if it is a regular file then lite
            FILE *file = fopen(file_path, "rb");

            if (file == NULL)
            {
                perror("Error opening file");
                return;
            }

            // Seek to the end of the file to get its size
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            rewind(file);

            char siz[32];
            sprintf(siz, "%ld", file_size);
            int s = send(socket, siz, 32, 0);
            if (s < 0)
            {
                printf("error\n");
            }
            // Read and send file in chunks
            char *buffer = (char *)calloc(8192, sizeof(char));

            size_t bytesRead;
            while ((bytesRead = fread(buffer, 1, 8192, file)) > 0)
            {
                int s = send(socket, buffer, 8192, 0);
                if (s < 0)
                {
                    printf("error\n");
                }
                // printf("buffer is -------\n");
                // printf("%s",buffer);
                memset(buffer, '\0', 8192);
                // printf("buffer is -------\n");
                // printf("%s",buffer);
            }

            free(buffer);
            fclose(file);
            chdir(currentWorkingDirectory);
        }
    }

    // printf("OUT OF PROCESS\n");
}

int receive_file(const char *file_path, int socket, int size)
{
    chdir(currentWorkingDirectory);
    chdir("main");
    // printf("filepath is %s\n", file_path);
    FILE *file = fopen(file_path, "wb");

    if (file == NULL)
    {
        perror("Error opening file");
        return 0;
    }

    // Receive the entire file content in one go
    char buffer[8192];
    // printf("text recieved is |%s| of size %d\n", buffer, sizeof(buffer));

    // Write the received data to the file
    for (int i = 0; i < size; i++)
    {
        printf("%d %d\n", i, size);
        int r = recv(server_sock, buffer, 8192, 0);
        if (r < 0)
        {
            perror("Receive error");
            fclose(file);
            return 0;
        }
        size_t written = fwrite(buffer, 1, r, file);
        if (written != r)
        {
            perror("Write error");
            fclose(file);
            return 0;
        }
    }

    // Flush the file to ensure data is written
    fflush(file);

    // Check for errors during the write operation
    if (ferror(file))
    {
        perror("Error writing to file");
        fclose(file);
        return 0;
    }

    // Close the file
    if (fclose(file) != 0)
    {
        perror("Error closing file");
        return 0;
    }

    printf(GREEN "File received successfully.\n" DEFAULT);
    chdir(currentWorkingDirectory);
    return 1;
}
int receive_file_REDUN(const char *file_path, int socket)
{
    printf("filepath is %s\n", file_path);
    FILE *file = fopen(file_path, "ab");

    if (file == NULL)
    {
        perror("Error opening file");
        return 0;
    }

    // Receive the entire file content in one go
    char *buffer = (char *)calloc(8192, sizeof(char));
    memset(buffer, '\0', 8192);
    int r = recv(socket, buffer, 8192, 0);
    printf("text recieved is |%s| of size %d\n", buffer, sizeof(buffer));
    if (r < 0)
    {
        perror("Receive error");
        fclose(file);
        return 0;
    }
    if (sizeof(buffer) <= 0)
    {
        if (fclose(file) != 0)
        {
            perror("Error closing file");
            return 0;
        }

        printf(GREEN "File received successfully.\n" DEFAULT);
        chdir(currentWorkingDirectory);
        return 1;
    }
    // Write the received data to the file
    size_t written = fwrite(buffer, 1, r, file);

    if (written != r)
    {
        perror("Write error");
        fclose(file);
        return 0;
    }

    // Flush the file to ensure data is written
    fflush(file);

    // Check for errors during the write operation
    if (ferror(file))
    {
        perror("Error writing to file");
        fclose(file);
        return 0;
    }

    // Close the file
    if (fclose(file) != 0)
    {
        perror("Error closing file");
        return 0;
    }

    printf(GREEN "File received successfully.\n" DEFAULT);
    chdir(currentWorkingDirectory);
    return 1;
}

void info_file(const char *file_path, int socket)
{
    chdir(currentWorkingDirectory);
    chdir("main");
    struct stat file_stat;

    if (stat(file_path, &file_stat) != 0)
    {
        perror("Error getting file information");
    }

    // Send the struct stat directly over the socket
    int result = send(socket, &file_stat, sizeof(struct stat), 0);
    if (result < 0)
    {
        perror("Error sending file information");
    }
    printf(GREEN "INFO SEND SUCCESSFULLY\n" DEFAULT);
    chdir(currentWorkingDirectory);
}
int get_port_name() // this gets us the predefined port that we are using
{
    chdir(currentWorkingDirectory);
    char cwd[1024]; // Buffer to store the current working directory
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        char *token = strtok(cwd, "/"); // Tokenize the path using "/"
        char *lastToken = NULL;
        // Traverse the tokens to get the last one (i.e., the current directory)
        while (token != NULL)
        {
            lastToken = token;
            token = strtok(NULL, "/");
        }
        if (lastToken != NULL)
        {
            return atoi(lastToken);
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }
    chdir(currentWorkingDirectory);
    return 0;
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
    printf("Naming Server: %s\n", buffer);
}
int connector_port(int sock)
{
    recieve_data_from_server(sock);
    return atoi(buffer);
}
void send_data_to_server(int sock, char *response)
{
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
void create_tcp_socket_client(int *server_sock, struct sockaddr_in *addr, int port)
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
void connect_to_naming_server()
{
    // we create the socket for the master port
    create_tcp_socket_client(&server_sock, &addr, MASTER_SERVER_PORT);
    // now we wait or the initial connection with the server
    port = get_port_name();
    int c_error = -1;
    while (c_error < 0)
    {
        c_error = connect(server_sock, (struct sockaddr *)&addr, sizeof(addr));
        if (c_error < 0)
        {
            perror(ORANGE "Looks Like the server is offline :(Retrying to connect in 2 seconds\n" DEFAULT);
            sleep(2);
            continue;
            // exit(1);
        }
        int s = -1;
        while (s < 0)
        {
            // we recieve the port with which we need to communicate
            char *ip = (char *)malloc(128);
            char ip1[INET_ADDRSTRLEN];

            if (getLocalIpAddress(ip1, sizeof(ip1)) == 0)
                strcpy(ip, ip1);
            else
            {
                strcpy(ip, IP_ADDRESS);
            }
            SS_NM_PTR temp = init_ss_nm(port, ip);
            temp->list_port = port + 100;
            printf("port is %d\n", temp->port);
            strcpy(temp->result, result);
            // printf("%s", temp->result);
            printf("%s\n", temp->ip);
            s = send(server_sock, temp, sizeof(struct SS_NM), 0);
            if (s < 0)
            {
                printf(RED "%d:Error in connecting\n" DEFAULT,SS_TO_NS_CONNECTION_ERROR);
            }
        }
    }
    printf(YELLOW "Connected to the server and have send the port for verfication.\n" DEFAULT);
    c_error = -1;
    // we close the connection with the master port
    close(server_sock);
    // now we create a new tcp socket for the new connection
    create_tcp_socket_client(&server_sock, &addr, port);
    while (c_error < 0)
    {
        c_error = connect(server_sock, (struct sockaddr *)&addr, sizeof(addr));
        if (c_error < 0)
        {
            perror(ORANGE "Looks Like the server is offline :(Retrying in 2 seconds\n" DEFAULT);
            sleep(2);
            // exit(1);
        }
    }
    c_error = -1;
    // we close the connection with the master port
    close(server_sock2);
    // now we create a new tcp socket for the new connection
    create_tcp_socket_client(&server_sock2, &addr, port + 200);
    while (c_error < 0)
    {
        c_error = connect(server_sock2, (struct sockaddr *)&addr, sizeof(addr));
        if (c_error < 0)
        {
            perror(ORANGE "Looks Like the server is offline :(Retrying in 2 seconds\n" DEFAULT);
            sleep(2);
            // exit(1);
        }
    }
    printf(GREEN "Connected to Storage Server via port %d\n" DEFAULT, port);
}

void acknowledgement(char *cond, char *str)
{
    ACK_SS_NM_PTR temp = fill_ACK_SS_NM(cond);
    int s = send(server_sock, temp, sizeof(struct ACK_SS_NM), 0);
    if (s < 0)
    {
        printf(RED "%d:ACK couldnt be send\n" DEFAULT,ACK_SEND_ERROR);
    }
    else
    {
        printf(GREEN "Success\n" DEFAULT);
    }
}
int deleteFolder(char *folderPath)
{
    pid_t pid = fork();
    if (pid == -1)
        printf("error in forking\n");
    // printf("IN ECEXVP\n");
    if (pid == 0)
    {
        char *argv[] = {"rm", "-rf", folderPath, NULL};
        // Execute the command using execvp
        if (execvp("rm", argv) == -1)
        {
            perror("execvp");
        }
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
    }
    // printf("OUT OF EXECVP\n");
}
void deleteRedFolder(COMMAND_PTR C)
{
    chdir(currentWorkingDirectory);
    pid_t pid = fork();
    if (pid == -1)
        printf("error in forking\n");
    // printf("IN DRF\n");
    // printf("CURRENT %s\n", C->path1);
    if (pid == 0)
    {
        char *argv[] = {"rm", "-rf", C->path1, NULL};
        // Execute the command using execvp
        if (execvp("rm", argv) == -1)
        {
            perror("execvp");
        }
    }
    else
    {
        // do nothing
        int status;
        waitpid(pid, &status, 0);
    }
}
int client_receive_file(const char *file_path, int socket)
{
    chdir(currentWorkingDirectory);
    chdir("main");
    // printf("filepath is %s\n", file_path);
    FILE *file = fopen(file_path, "wb");

    if (file == NULL)
    {
        perror("Error opening file");
        return 0;
    }

    // Receive the entire file content in one go
    char buffer[8192];
    int r = recv(socket, buffer, 8192, 0);
    // printf("text recieved is |%s| of size %d\n", buffer, sizeof(buffer));
    if (r < 0)
    {
        perror("Receive error");
        fclose(file);
        return 0;
    }
    if (sizeof(buffer) <= 0)
    {
        if (fclose(file) != 0)
        {
            perror("Error closing file");
            return 0;
        }

        printf(GREEN "File received successfully.\n" DEFAULT);
        chdir(currentWorkingDirectory);
        return 1;
    }
    // Write the received data to the file
    size_t written = fwrite(buffer, 1, r, file);

    if (written != r)
    {
        perror("Write error");
        fclose(file);
        return 0;
    }

    // Flush the file to ensure data is written
    fflush(file);

    // Check for errors during the write operation
    if (ferror(file))
    {
        perror("Error writing to file");
        fclose(file);
        return 0;
    }

    // Close the file
    if (fclose(file) != 0)
    {
        perror("Error closing file");
        return 0;
    }

    printf(GREEN "File received successfully.\n" DEFAULT);
    chdir(currentWorkingDirectory);
    return 1;
}
void *NM_runner(void *args)
{
    while (1)
    {
        printf("waiting...\n");
        COMMAND_PTR C = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
        // printf("%d\n",server_sock);
        int r = recv(server_sock, C, sizeof(struct COMMAND), 0);
        sem_wait(&lock);
        if (r < 0)
        {
            printf(RED "%d:didnt recieve struct\n" DEFAULT,STRUCT_RECEIVE_ERROR);
            exit(0);
        }
        if (r == 0)
        {
            printf(GREEN "Naming Server has shut down i think\n" DEFAULT);
            exit(0);
        }
        else
        {
            chdir(currentWorkingDirectory);
            chdir("main");
            if (strcmp("createfile", C->operation) == 0)
            {
                //  printf("Here\n");
                int test = createFilePath(C->path1);
                // printf("test is %d\n", test);
                if (test == 1)
                {
                    acknowledgement("YES", "File path created.");
                }
                else
                {
                    acknowledgement("NO", "File path couldnt be created.");
                }
                // printf("HERE2\n");
            }
            else if (strcmp("createdir", C->operation) == 0)
            {
                int test = createDirPath(C->path1);
                if (test == 1)
                {
                    acknowledgement("YES", "Directory path created");
                }
                else
                {
                    acknowledgement("NO", "Directory path couldnt be created");
                }
            }
            else if (strcmp("deletefile", C->operation) == 0)
            {
                int test = removeLastFile(C->path1);
                if (test == 1)
                {
                    acknowledgement("YES", "Directory succssfully deleted");
                }
                else
                {
                    acknowledgement("NO", "Error");
                }
            }
            else if (strcmp("deletedir", C->operation) == 0)
            {
                int test = deleteFolder(C->path1);
                if (test == 1)
                {
                    acknowledgement("YES", "Directory succssfully deleted");
                }
                else
                {
                    acknowledgement("NO", "Error");
                }
            }
            else if (strcmp("copyfileread", C->operation) == 0)
            {
                process_file(C->path1, server_sock, "main");
            }
            else if (strcmp("copyfilewrite", C->operation) == 0)
            {
                printf("in copy file write %s\n", C->path2);
                int test = receive_file(C->path1, server_sock, atoi(C->path2));
                 printf("test is %d\n", test);
                if (test == 1)
                {
                    acknowledgement("YES", "Directory succssfully deleted");
                }
                else
                {
                    acknowledgement("NO", "Error");
                }
                // printf("write ack send\n");
            }
            else if (strcmp("copydirread", C->operation) == 0)
            {
                process_file(C->path1, server_sock, "main");
            }
            else if (strcmp("deletereddir", C->operation) == 0)
            {
                // printf("IN DELETE RED DIRECTORY\n");
                deleteRedFolder(C);
                chdir(currentWorkingDirectory);
                createDirectory(C->path1);
            }
            else if (strcmp("getMain", C->operation) == 0)
            {
                MainCount();
                // now i need to send the data of the result to NS
                int s = send(server_sock, result, 4096, 0);
                if (r < 0)
                {
                    printf("send error\n");
                }
                // printf("Get MAIN DONE\n %s\n", result);
            }
            else if (strcmp("createREDdir", C->operation) == 0)
            {
                chdir(currentWorkingDirectory);
                chdir(C->path2);
                createDirPath(C->path1);
            }
            else if (strcmp("createREDfile", C->operation) == 0)
            {
                // printf("IN HERE %s %s\n", C->path2, C->path1);
                chdir(currentWorkingDirectory);
                chdir(C->path2);
                createFilePath(C->path1);
            }
            else if (strcmp("copyREDfile", C->operation) == 0)
            {
                // printf("CRF %s %s\n", C->path2, C->path1);
                chdir(currentWorkingDirectory);
                chdir(C->path2);
                receive_file_REDUN(C->path1, server_sock);
            }
            else if (strcmp("getsizdir", C->operation) == 0)
            {
                getsiz_isdir(C->path1, server_sock);
            }
            chdir(currentWorkingDirectory);
        }
        free(C);
        sem_post(&lock);
    }
}

void *C_runner(void *args)
{
    while (1)
    {
        printf("listening...\n");
        socklen_t addr_size = sizeof(server_listening_addr);
        int temp_client_sock = accept(server_listening_sock, (struct sockaddr *)&server_listening_addr, &addr_size);
        if (temp_client_sock < 0)
        {
            perror("A client tried to connect but could not connect\n");
            continue;
            // exit(1);
        }
        else
        {
            printf(GREEN "Connection made with Client\n" DEFAULT);
            COMMAND_PTR C = (COMMAND_PTR)malloc(sizeof(struct COMMAND));
            int r = recv(temp_client_sock, C, sizeof(struct COMMAND), 0);
            sem_wait(&lock);
            if (r < 0)
            {
                printf(RED "%d:Command Not Recieved\n" DEFAULT,RECIEVE_COMMAND_ERROR);
            }
            else if (strcmp("read", C->operation) == 0)
            {
                // printf("IN READ PATH IS %s|%s|%s|\n",C->operation,C->path1,C->path2);
                process_file(C->path1, temp_client_sock, C->path2);
            }
            else if (strcmp("write", C->operation) == 0)
            {
                // printf("%s %s %s %d\n", C->operation, C->path1, C->path2, C->port);
                client_receive_file(C->path1, temp_client_sock);
            }
            else if (strcmp("info", C->operation) == 0)
            {
                // printf("%s %s %s %d\n", C->operation, C->path1, C->path2, C->port);
                info_file(C->path1, temp_client_sock);
            }
            sem_post(&lock);
        }
        // now to process the command and send the file to the client
    }
}
void cntrlC(int sig)
{
    int s = send(server_sock2, "BYE", 3, 0);
    close(server_sock);
    close(server_sock2);
    close(server_listening_sock);
    printf("BYE\n");
    exit(0);
}

int main()
{
    // we create the socket which will first interact with the server through the master port
    if (signal(SIGINT, cntrlC) == SIG_ERR)
    {
        printf(RED "%d:Error initializing server!!!\n" DEFAULT,INIT_SERVER_ERROR);
        return 1;
    }

    if (getcwd(currentWorkingDirectory, sizeof(currentWorkingDirectory)) != NULL)
    {
        printf("Current Working Directory: %s\n", currentWorkingDirectory);
    }
    else
    {
        exit(0);
    }
    sem_init(&lock, 0, 1);
    sem_init(&del_lock, 0, 1);
    listFilesRecursively(".");
    // printf("List of accessible files are\n");
    // printf("%s", result);
    connect_to_naming_server();
    port2 = port + 100;
    create_tcp_socket(&server_listening_sock);
    connect_to_client(IP_ADDRESS, port2, &server_listening_sock);

    pthread_t *threads = (pthread_t *)malloc(sizeof(pthread_t) * 2);
    pthread_create(&threads[0], NULL, NM_runner, NULL);
    pthread_create(&threads[0], NULL, C_runner, NULL);

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);

    return 0;
}

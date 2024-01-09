#include "server.h"

void create_tcp_socket(int *server_sock) // this is to create a server socket,
{
    *server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (*server_sock < 0)
    {
        perror("[-]Socket error");
        exit(1);
    }
}
void connect_to_client(char *ip, int port, int *server_sock) // this is to bind and make a server listen for the client
{
    struct sockaddr_in server_addr;

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);
    if (server_addr.sin_addr.s_addr == INADDR_NONE)
    {
        perror("[-]Address error");
        exit(1);
    }

    int n = bind(*server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (n < 0)
    {
        perror("[-]Bind error");
        exit(1);
    }
    // printf("[+]Bind to the port number: %d\n", port);

    int l_error = listen(*server_sock, 5);
    if (l_error < 0)
    {
        perror("[-]Listen error");
        exit(1);
    }
    // printf("Listening...\n");
}
void close_all_clients(int client_sock) // this is to close the connection
{
    // to close connection to client 1
    int c_error = close(client_sock);
    if (c_error < 0)
    {
        perror("[-]Close error");
        exit(1);
    }
    printf("[+]Client 1 disconnected.\n\n");
}

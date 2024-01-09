#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

void create_tcp_socket(int *server_sock); // this is to create a server socket;
void connect_to_client(char *ip, int port, int *server_sock); // this is to bind and make a server listen for the client
void close_all_clients(int client_sock); // this is to close the connection

#endif
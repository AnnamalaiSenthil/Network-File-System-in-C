#ifndef __LOG_H__
#define __LOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>

void log_command(const char *ip, int client_port, int storage_server_port, const char *command, const char *status);

#endif
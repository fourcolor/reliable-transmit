#ifndef SOCKET_API_H
#define SOCKET_API_H
#include "config.h"

#define NET_SOFTERROR -1
#define NET_HARDERROR -2
int client_sock_run(config_t* config);
#endif
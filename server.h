#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <signal.h>
#include <fcntl.h>
#include <stdbool.h>



#define BUFFER_SIZE 2048
#define MAX_CLIENTS 5

struct Client {
    int socket_sd;
    char *display_name; // TODO max cisla
    char *channel_name;
    char *protocol;
    struct sockaddr_in address;
    bool authenticated;
    int id;
};

void server(char ip_addr[],uint16_t port,uint16_t udp_timeout, uint8_t udp_ret);
struct sockaddr_in adress_fill(uint16_t port);
void tcp_accept(int tcp_socket,int client_sockets[]);

#endif

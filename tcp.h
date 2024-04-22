#ifndef TCP_H
#define TCP_H

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

#include "server.h"

#define MAX_CLIENTS 5
#define BUFFER_SIZE 2048



void tcp_accept(int tcp_socket,int client_sockets[],struct sockaddr_in client_address);
void tcp_auth(struct Client user[MAX_CLIENTS],int sd,struct sockaddr_in client_address,int i);
void tcp_join(struct Client user[MAX_CLIENTS],struct sockaddr_in client_address,int i,int sd);
void tcp_msg(struct Client user[MAX_CLIENTS],struct sockaddr_in client_address,int i,int sd,int client_sockets[], int udp_socket,int address_size);
void tcp_err(struct Client user[MAX_CLIENTS],struct sockaddr_in client_address,int sd,int client_sockets[],int i);
void tcp_bye(int client_sockets[],struct sockaddr_in client_address,struct Client user[MAX_CLIENTS],int i,int sd);

#endif
#ifndef UDP_H
#define UDP_H

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

#define BUFFER_SIZE 4096
#define MAX_CLIENTS 256

void handle_udp_packet(int udp_socket,int client_sockets[],struct sockaddr_in client_address,struct Client user[]);
void confirm(char buffer[BUFFER_SIZE],int udp_socket,struct sockaddr_in client_address);
void udp_auth(int udp_socket, struct sockaddr_in client_address,int client_sockets[],struct Client user[],char* disp_tmp);
void udp_message(struct Client user[MAX_CLIENTS],int client_sockets[], struct sockaddr_in client_address);

#endif
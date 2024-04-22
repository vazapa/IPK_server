#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>

#define MAX_CLIENTS 5

extern volatile sig_atomic_t keepRunning;
extern uint16_t message_id;

void handle_udp_packet(int udp_socket,int client_sockets[],struct sockaddr_in client_address,struct Client user[]);
void tcp_accept(int tcp_socket,int client_sockets[],struct sockaddr_in client_address);
void tcp_auth(struct Client user[MAX_CLIENTS],int sd,struct sockaddr_in client_address,int i,int client_sockets[], int udp_socket,int address_size);
void tcp_join(struct Client user[MAX_CLIENTS],struct sockaddr_in client_address,int i,int sd,int client_sockets[],int udp_socket,int address_size);
void tcp_msg(struct Client user[MAX_CLIENTS],struct sockaddr_in client_address,int i,int sd,int client_sockets[], int udp_socket,int address_size);
void tcp_err(struct Client user[MAX_CLIENTS],struct sockaddr_in client_address,int sd,int client_sockets[],int i);
void tcp_bye(int client_sockets[],struct sockaddr_in client_address,struct Client user[MAX_CLIENTS],int i,int sd);

#endif 

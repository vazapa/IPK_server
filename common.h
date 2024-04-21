#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>

extern volatile sig_atomic_t keepRunning;
extern uint16_t message_id;

void handle_udp_packet(int udp_socket,int client_sockets[],struct sockaddr_in client_address,struct Client user[]);

#endif 

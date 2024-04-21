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
void handle_udp_packet(int udp_socket,int client_sockets[],struct sockaddr_in client_address,struct Client user[]);
void tcp_accept(int tcp_socket,int client_sockets[],struct Client user[]);
void confirm(char buffer[BUFFER_SIZE],int udp_socket,struct sockaddr_in client_address);
void udp_auth(int udp_socket,struct Client user[MAX_CLIENTS],int client_sockets[], struct sockaddr_in client_address);
void udp_message(struct Client user[MAX_CLIENTS],int client_sockets[], struct sockaddr_in client_address);
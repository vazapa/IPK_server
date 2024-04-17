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

#define MAX_CLIENTS 3

struct Client {
    int socket_sd;
    char *display_name; // TODO max cisla
    char *channel_name;
    char *protocol;
};

void server(char ip_addr[],uint16_t port,uint16_t udp_timeout, uint8_t udp_ret);
struct sockaddr_in adress_fill(uint16_t port);
void handle_udp_packet(int udp_socket,int client_sockets[],struct sockaddr_in client_address,struct Client user[]);
void tcp_accept(int tcp_socket,int client_sockets[],struct Client user[]);

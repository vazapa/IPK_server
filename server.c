#include "server.h"
#include "common.h"

#define BUFFER_SIZE 4096
#define MAX_CLIENTS 256
#define MAX_CHANNELS 3

char buffer[BUFFER_SIZE];
char reply_buffer[BUFFER_SIZE];
volatile sig_atomic_t keepRunning = 1;
uint16_t message_id = 0;
uint8_t ref_id[3];

void intHandler() {
    keepRunning = 0;
}

/**
 * @brief Completes server address
 * 
 * @param ip_addr 
 * @param port 
 * @return struct sockaddr_in 
 */
struct sockaddr_in adress_fill(char *ip_addr,uint16_t port){
    struct sockaddr_in server_address; 

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET; 
    int a = inet_pton(AF_INET, ip_addr, &server_address.sin_addr);
    if (a <= 0){
        fprintf(stderr,"Error with inet_pton");
    }

    server_address.sin_port = htons(port);
    
    return server_address;
}

/**
 * @brief Handling all sockets
 * 
 * @param ip_addr 
 * @param port 
 * @param udp_timeout 
 * @param udp_ret 
 */
void server(char *ip_addr,uint16_t port,uint16_t udp_timeout, uint8_t udp_ret){
    struct Client user[MAX_CLIENTS] = {0};
    
    fd_set readfds;
    int max_sd = 0;

    struct sockaddr_in client_address;
    socklen_t client_address_size = sizeof(client_address);
    struct sockaddr_in server_address = adress_fill(ip_addr,port);
    struct sockaddr *address = (struct sockaddr *) &server_address; 
    int address_size = sizeof(server_address);

    int max_waiting_connections = 50; 
    int enable = 1; 

    int udp_socket;
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (udp_socket < 0) {
            fprintf(stderr,"ERR: with UDP socket creation");
        }
    
    int client_sockets[MAX_CLIENTS] = {0};
    signal(SIGINT, intHandler); 

    /* ------- SERVER/SOCKET INFO -------*/
    
    int welcome_socket = socket(AF_INET, SOCK_STREAM, 0); 
    if (welcome_socket <= 0)
    {
        fprintf(stderr,"ERR: with creating TCP socket");
    }
    
    setsockopt(welcome_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    if (bind(udp_socket, address, address_size) < 0) {
        fprintf(stderr,"ERR: with UDP bind");
    }

    if (bind(welcome_socket, address, address_size) < 0) {
        fprintf(stderr,"ERR: with TCP bind");
    }

    if (listen(welcome_socket, max_waiting_connections) < 0) {
        fprintf(stderr,"ERR: with TCP listen");
    }

    int flags = fcntl(welcome_socket, F_GETFL, 0);
    fcntl(welcome_socket, F_SETFL, flags | O_NONBLOCK);

    int flags2 = fcntl(udp_socket, F_GETFL, 0);
    fcntl(udp_socket, F_SETFL, flags2 | O_NONBLOCK);

    
    /* ------- SERVER/SOCKET INFO -------*/
    
    while (keepRunning) {

        FD_ZERO(&readfds);

        FD_SET(welcome_socket, &readfds);
        FD_SET(udp_socket, &readfds);

        // Add sockets
        max_sd = welcome_socket > udp_socket ? welcome_socket : udp_socket;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            //add to read 
            if (sd > 0) {
                FD_SET(sd, &readfds);
                max_sd = sd > max_sd ? sd : max_sd;
            }
        }

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("select error");
        }

        /* ---------- ACCEPT ---------- */
        // Waiting for TCP activity
        if (FD_ISSET(welcome_socket, &readfds)) {
            
            tcp_accept(welcome_socket,client_sockets,client_address);
         
        }
        /* ---------- ACCEPT ---------- */
        // Waiting for UDP activity
        if (FD_ISSET(udp_socket, &readfds)) {
            handle_udp_packet(udp_socket,client_sockets,client_address,user);
            
        }
        // Loop for clients, handling only TCP users
        for (int i = 0; i < MAX_CLIENTS; i++) {
            
            int sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                
                //Read incoming message
                getpeername(sd, (struct sockaddr *) &client_address, &client_address_size); 
                memset(buffer, 0, BUFFER_SIZE);
                int valread = recv(sd, buffer, BUFFER_SIZE, 0);
                if (valread == 0) {
                    shutdown(sd, SHUT_RDWR); 
                    shutdown(client_sockets[i], SHUT_RDWR); 
                    close(sd);
                    close(client_sockets[i]);
                    client_sockets[i] = 0;
                } else {
                    
                    if(strncmp(buffer,"AUTH",4) == 0){ 

                        tcp_auth(user,sd,client_address,i,client_sockets,udp_socket,address_size);
                        
                        
                    }else if(strncmp(buffer,"JOIN",4) == 0){             
                        
                        tcp_join(user,client_address,i,sd,client_sockets,udp_socket,address_size);

                    }else if(strncmp(buffer,"MSG",3) == 0){

                        tcp_msg(user,client_address,i,sd,client_sockets,udp_socket,address_size);
                        
                    }else if(strncmp(buffer,"ERR",3) == 0){

                        tcp_err(user,client_address,sd,client_sockets,i);

                    }else if(strncmp(buffer,"BYE",3) == 0){

                            tcp_bye(client_sockets,client_address,user,i,sd,udp_socket,address_size);

                    }
                    memset(buffer, 0, BUFFER_SIZE);
                }
            }
        }
    }

    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        user->socket_sd = client_sockets[i];
        if (user->socket_sd > 0) {

            getpeername(user->socket_sd, (struct sockaddr *) &client_address, &client_address_size); 
            printf("SENT %s:%d | BYE\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
            snprintf(buffer, BUFFER_SIZE, "BYE\r\n");
            send(user->socket_sd, buffer, strlen(buffer), 0);
        
            
            shutdown(client_sockets[i], SHUT_RDWR); 
            close(client_sockets[i]);
        }
    }
    shutdown(welcome_socket, SHUT_RDWR); 
    close(welcome_socket); 
}
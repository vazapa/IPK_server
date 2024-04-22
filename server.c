#include "server.h"


#include "common.h"


#define BUFFER_SIZE 2048
#define MAX_CLIENTS 5
#define MAX_CHANNELS 3

/* TODO
- asi kontrolovani spravnosti loginu atd
- server shall never use the Username of the authenticated user as their DisplayName
- argumenty
- An example of a valid channel join failure is when the server is unable to internally create the corresponding channel or add the connection user to that channel.
- Assuming a single connection per unique user account (username) at the most.
- jdou posilat zpravy i kdyz uzivatel neni authnuty
- otestovat na nixu
---- TCP ----
- vyresit stream
- dodelat zpravy
---- UDP ----
- dynamicky port!!! -> Call sendto without calling bind first, the socket will be bound automatically (to a free port).
- dodelat zpravy
- timeout a retries

- err
*/

char buffer[BUFFER_SIZE];
char reply_buffer[BUFFER_SIZE];
volatile sig_atomic_t keepRunning = 1;
uint16_t message_id = 0;

uint8_t ref_id[3];

void intHandler() {
    keepRunning = 0;
}

struct sockaddr_in adress_fill(uint16_t port){
    struct sockaddr_in server_address; 

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET; 
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_address.sin_port = htons(port);
    
    return server_address;
}

void server(char ip_addr[],uint16_t port,uint16_t udp_timeout, uint8_t udp_ret){
    printf("%d%d%d%s",port,udp_ret,udp_timeout,ip_addr);
    struct Client user[MAX_CLIENTS] = {0};
    
    
    
    fd_set readfds;
    int max_sd = 0;

    struct sockaddr_in client_address;
    socklen_t client_address_size = sizeof(client_address);
    struct sockaddr_in server_address = adress_fill(port);
    struct sockaddr *address = (struct sockaddr *) &server_address; 
    int address_size = sizeof(server_address);

    int max_waiting_connections = 3; // TODO zvysit
    int enable = 1; // TOOD umoznuje rychle znovu nastaveni portu, idk jestli to mame pouzit

    int udp_socket;
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
        if (udp_socket < 0) {
            perror("UDP socket creation failed");
            exit(EXIT_FAILURE);
        }
    // Client socket descriptors
    int client_sockets[MAX_CLIENTS] = {0};
    signal(SIGINT, intHandler); //ctrl+c and ctrl+c

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
    // Main loop
    while (keepRunning) {

        FD_ZERO(&readfds);

        FD_SET(welcome_socket, &readfds);
        FD_SET(udp_socket, &readfds);

        // Add child sockets to set and update max_sd
        max_sd = welcome_socket > udp_socket ? welcome_socket : udp_socket;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            // If valid socket descriptor, add to read list and update max_sd
            if (sd > 0) {
                FD_SET(sd, &readfds);
                max_sd = sd > max_sd ? sd : max_sd;
            }
        }

        // Wait for activity on any of the sockets
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("select error");
        }

        /* ---------- ACCEPT ---------- */
        // If something happened on the master socket, then it's an incoming connection
        if (FD_ISSET(welcome_socket, &readfds)) {
            
            tcp_accept(welcome_socket,client_sockets,client_address);
         
        }
        /* ---------- ACCEPT ---------- */
        
        if (FD_ISSET(udp_socket, &readfds)) {
            handle_udp_packet(udp_socket,client_sockets,client_address,user);
            
        }
        
        /* ---------- COMM LOOP ---------- */
        // Else it's some IO operation on some other socket
        for (int i = 0; i < MAX_CLIENTS; i++) {
            
            int sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                    
                // Check if it was for closing, and also read the incoming message
                getpeername(sd, (struct sockaddr *) &client_address, &client_address_size); //TODO mozna pouzit neco jineho
                memset(buffer, 0, BUFFER_SIZE);
                int valread = recv(sd, buffer, BUFFER_SIZE, 0);
                if (valread == 0) {
                    // Somebody disconnected, get his details and print
                    // printf("Host disconnected, ip %s, port %d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

                    // Close the socket and mark as 0 in list for reuse
                    shutdown(sd, SHUT_RDWR); 
                    shutdown(client_sockets[i], SHUT_RDWR); 
                    close(sd);
                    close(client_sockets[i]);
                    client_sockets[i] = 0;
                } else {
                    
                    if(strncmp(buffer,"AUTH",4) == 0){ 

                        tcp_auth(user,sd,client_address,i,client_sockets,udp_socket,address_size);
                        
                        
                    }else if(strncmp(buffer,"JOIN",4) == 0){ //TODO handle reply                        
                        
                        tcp_join(user,client_address,i,sd,client_sockets,udp_socket,address_size);

                    }else if(strncmp(buffer,"MSG",3) == 0){

                        tcp_msg(user,client_address,i,sd,client_sockets,udp_socket,address_size);
                        
                    }else if(strncmp(buffer,"ERR",3) == 0){

                        tcp_err(user,client_address,sd,client_sockets,i);

                    }else if(strncmp(buffer,"BYE",3) == 0){

                            tcp_bye(client_sockets,client_address,user,i,sd);;

                    }
                    memset(buffer, 0, BUFFER_SIZE);
                    
                }
            }
        }
    }


    /* ---------- COMM LOOP ---------- */
    // close(udp_socket);

    /* ---------- LOOP END ---------- */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        user->socket_sd = client_sockets[i];
        if (user->socket_sd > 0) {
            // Send BYE message
            getpeername(user->socket_sd, (struct sockaddr *) &client_address, &client_address_size); //TODO mozna pouzit neco jineho
            printf("SENT %s:%d | BYE\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
            snprintf(buffer, BUFFER_SIZE, "BYE\r\n");
            send(user->socket_sd, buffer, strlen(buffer), 0);
            // Close socket
            
            shutdown(client_sockets[i], SHUT_RDWR); 
            close(client_sockets[i]);

            
            // Mark socket as 0 in the list
            client_sockets[i] = 0;
        }
    }
    shutdown(welcome_socket, SHUT_RDWR); 
    close(welcome_socket); 
    /* ---------- LOOP END ---------- */
}
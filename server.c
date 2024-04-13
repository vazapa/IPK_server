#include "server.h"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 3
#define MAX_CHANNELS 3

/* TODO
- asi kontrolovani spravnosti loginu atd
-It is required that, by default, 
any combination of a valid username (Username), display name (DisplayName) and secret (Secret) will be authenticated successfully by the server. 
Assuming a single connection per unique user account (username) at the most.
- server shall never use the Username of the authenticated user as their DisplayName
*/


struct Client {
    int socket_sd;
    char *display_name; // TODO max cisla
    char *channel_name;
};


char *MessageContent;
char buffer[BUFFER_SIZE];
volatile sig_atomic_t keepRunning = 1;

void intHandler(int sig) {
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
    
    // Set of socket descriptors
    fd_set readfds;
    int max_sd; // Maximum socket descriptor

    struct sockaddr_in client_address;
    socklen_t client_address_size = sizeof(client_address);
    struct sockaddr_in server_address = adress_fill(port);
    struct sockaddr *address = (struct sockaddr *) &server_address; 
    int address_size = sizeof(server_address);

    int max_waiting_connections = 3; // TODO zvysit
    int enable = 1; // TOOD umoznuje rychle znovu nastaveni portu, idk jestli to mame pouzit

    // Client socket descriptors
    int client_sockets[MAX_CLIENTS] = {0};
    signal(SIGINT, intHandler); //ctrl+c and ctrl+c

    /* ------- SERVER/SOCKET INFO -------*/
    //TCP socket creation
    int welcome_socket = socket(AF_INET, SOCK_STREAM, 0); 
    if (welcome_socket <= 0)
    {
        fprintf(stderr,"ERR: with creating TCP socket");
    }
    
    setsockopt(welcome_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    if (bind(welcome_socket, address, address_size) < 0) { //TOOD: &address nebo address ?
        fprintf(stderr,"ERR: with TCP bind");
    }

    if (listen(welcome_socket, max_waiting_connections) < 0) {
        fprintf(stderr,"ERR: with TCP listen");
    }

    int flags = fcntl(welcome_socket, F_GETFL, 0);
    fcntl(welcome_socket, F_SETFL, flags | O_NONBLOCK);

    
    /* ------- SERVER/SOCKET INFO -------*/


    // Main loop
    while (keepRunning) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add the master socket to the set
        FD_SET(welcome_socket, &readfds);
        max_sd = welcome_socket;

        // Add child sockets to set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];

            // If valid socket descriptor, add to read list
            if (sd > 0)
                FD_SET(sd, &readfds);

            // Get the highest file descriptor number
            if (sd > max_sd)
                max_sd = sd;
        }

        // Wait for activity on any of the sockets
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("select error");
        }

        /* ---------- ACCEPT ---------- */
        // If something happened on the master socket, then it's an incoming connection
        if (FD_ISSET(welcome_socket, &readfds)) {
            struct sockaddr_in client_address;
            socklen_t client_address_size = sizeof(client_address);
            int new_socket = accept(welcome_socket, (struct sockaddr *) &client_address, &client_address_size);
            if (new_socket < 0) {
                // perror("ERR: accepting connection"); // TODO pri ctrl+c to dela bordel
                // exit(EXIT_FAILURE);
                break;
            }
            // Add new socket to array of sockets
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }
        /* ---------- ACCEPT ---------- */
        
        /* ---------- COMM LOOP ---------- */
        // Else it's some IO operation on some other socket
        for (int i = 0; i < MAX_CLIENTS; i++) {
            // user->socket_sd = client_sockets[i];
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
                    if(strncmp(buffer,"AUTH",4) == 0){ // TODO checknout spravnost
                        printf("RECV %s:%d | AUTH\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

                        /* TODO UDELAT JEDNOSUSSI*/
                        char *login_message = buffer + 5; // Move past "AUTH "
                        char *display_name_start = strstr(login_message, "AS") + 3;
                        char *display_name_end = strstr(display_name_start, " ");
                        *display_name_end = '\0'; // Null-terminate the display name
                        char *display_name = strdup(display_name_start);

                        // Store the display name of the client
                        user[i].display_name = display_name;
                        /* TODO UDELAT JEDNOSUSSI*/



                        memset(buffer, 0, BUFFER_SIZE);
                        snprintf(buffer, BUFFER_SIZE, "REPLY OK IS Sucesful auth\r\n"); 
                        send(sd, buffer, strlen(buffer), 0);
                        printf("SENT %s:%d | REPLY\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

                        user[i].channel_name = "general";

                        for (int j = 0; j < MAX_CLIENTS; j++) {
                                if (client_sockets[j] > 0 && client_sockets[j] != sd && strcmp(user[i].channel_name, user[j].channel_name) == 0) {
                                    // Send notification to other clients
                                    memset(buffer, 0, BUFFER_SIZE);
                                    snprintf(buffer, BUFFER_SIZE, "MSG FROM Server IS %s joined the %s\r\n", user[i].display_name,user[i].channel_name);
                                    send(client_sockets[j], buffer, strlen(buffer), 0);
                                }
                        }
                        
                        continue;
                    }else if(strncmp(buffer,"JOIN",4) == 0){ //TODO handle reply
                        //JOIN channel_name AS Display_name
                        printf("RECV %s:%d | JOIN\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                        char *token =  buffer + 4;
                        token = strtok(token," ");
                        user[i].channel_name = strdup(token);
                        memset(buffer, 0, BUFFER_SIZE);
                        snprintf(buffer, BUFFER_SIZE, "REPLY OK IS Sucesful join\r\n");//TODO tweak na REPLY!
                        send(sd, buffer, strlen(buffer), 0); 
                        printf("SENT %s:%d | REPLY\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                                if (client_sockets[j] > 0 && client_sockets[j] != sd && strcmp(user[i].channel_name, user[j].channel_name) == 0) {
                                    // Send notification to other clients
                                    memset(buffer, 0, BUFFER_SIZE);
                                    snprintf(buffer, BUFFER_SIZE, "MSG FROM Server IS %s joined the %s\r\n", user[i].display_name,user[i].channel_name);
                                    send(client_sockets[j], buffer, strlen(buffer), 0);
                                }
                        }

                    }else if(strncmp(buffer,"MSG",3) == 0){
                        //MSG FROM displej IS zprava
                        printf("RECV %s:%d | MSG\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                        char buffer_copy[BUFFER_SIZE];
                        strcpy(buffer_copy, buffer);
                        MessageContent = strstr(buffer_copy, "IS"); //strcasestr
                        
                        if (MessageContent != NULL) {
                            MessageContent += 3;
                            
                            memset(buffer, 0, BUFFER_SIZE);
                            snprintf(buffer, BUFFER_SIZE, "MSG FROM %s IS %s",user[i].display_name,MessageContent);
                            for (int j = 0; j < MAX_CLIENTS; j++) { //Sending messages from one client to others
                                int dest_socket = client_sockets[j];
                                if (dest_socket != sd && dest_socket > 0 && strcmp(user[i].channel_name, user[j].channel_name) == 0 ) {
                                    send(dest_socket, buffer, strlen(buffer), 0);
                                }
                                
                            }
                        }
                    }else if(strncmp(buffer,"ERR",3) == 0){
                        printf("RECV %s:%d | ERR\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                            // Iterate over all client sockets
                            for (int j = 0; j < MAX_CLIENTS; j++) {
                                if (client_sockets[j] > 0 && client_sockets[j] != sd) {
                                    // Send notification to other clients
                                    memset(buffer, 0, BUFFER_SIZE);
                                    snprintf(buffer, BUFFER_SIZE, "MSG FROM Server IS %s left the %s\r\n", user[i].display_name,user[i].channel_name);
                                    send(client_sockets[j], buffer, strlen(buffer), 0);
                                    
                                }
                            }
                            shutdown(sd, SHUT_RDWR); 
                            shutdown(client_sockets[i], SHUT_RDWR); 

                            close(sd);
                            close(client_sockets[i]);
                            client_sockets[i] = 0;
                            continue;
                    }else if(strncmp(buffer,"BYE",3) == 0){
                            printf("RECV %s:%d | BYE\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                            // Iterate over all client sockets
                            for (int j = 0; j < MAX_CLIENTS; j++) {
                                if (client_sockets[j] > 0 && client_sockets[j] != sd && strcmp(user[i].channel_name, user[j].channel_name) == 0) {
                                    // Send notification to other clients
                                    memset(buffer, 0, BUFFER_SIZE);
                                    printf("channel jmeno: %s\n",user[i].channel_name);
                                    snprintf(buffer, BUFFER_SIZE, "MSG FROM Server IS %s left the %s\r\n", user[i].display_name, user[i].channel_name);
                                    send(client_sockets[j], buffer, strlen(buffer), 0);
                                    
                                }
                            }
                            shutdown(sd, SHUT_RDWR); 
                            shutdown(client_sockets[i], SHUT_RDWR); 
                            close(sd);
                            close(client_sockets[i]);

                            free(user[i].channel_name);
                            free(user[i].display_name);
                            
                            client_sockets[i] = 0;
                    }
                    memset(buffer, 0, BUFFER_SIZE);
                    
                }
            }
        }
    }
    /* ---------- COMM LOOP ---------- */

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
#include "server.h"
#define BUFFER_SIZE 2048
#define MAX_CLIENTS 5
#define MAX_CHANNELS 3

/* TODO
- asi kontrolovani spravnosti loginu atd
- server shall never use the Username of the authenticated user as their DisplayName
- argumenty
- An example of a valid channel join failure is when the server is unable to internally create the corresponding channel or add the connection user to that channel.
- Assuming a single connection per unique user account (username) at the most.
- co nejmin global variables, at to muzu rozdelit do souboru
- otestovat na nixu
---- TCP ----
- vyresit stream
---- UDP ----
- dynamicky port!!! -> Call sendto without calling bind first, the socket will be bound automatically (to a free port).

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

void tcp_accept(int tcp_socket,int client_sockets[]){
    struct sockaddr_in client_address;
    socklen_t client_address_size = sizeof(client_address);
    int new_socket = accept(tcp_socket, (struct sockaddr *) &client_address, &client_address_size);
    if (new_socket < 0) {
        // perror("ERR: accepting connection"); // TODO pri ctrl+c to dela bordel
        // exit(EXIT_FAILURE);
        // break;
    }
    // Add new socket to array of sockets
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == 0) {
            client_sockets[i] = new_socket;
            break;
        }
    }
}

void confirm(char buffer[BUFFER_SIZE],int udp_socket,struct sockaddr_in client_address){
        
        socklen_t addr_len = sizeof(client_address);
        uint16_t ref_messageID = *(uint16_t*)(buffer + 1);
        ref_id[0] = (uint8_t)(ref_messageID & 0xFF);
        ref_id[1] = (uint8_t)((ref_messageID >> 8) & 0xFF);

        memset(reply_buffer, 0, BUFFER_SIZE); 

        reply_buffer[0] = 0x00;
        memcpy(reply_buffer + 1, &ref_id,sizeof(u_int16_t));
        sendto(udp_socket, reply_buffer, 3, 0, (struct sockaddr *)&client_address, addr_len);

        printf("SENT %s:%d | CONFIRM\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        memset(reply_buffer, 0, BUFFER_SIZE); 
}

void udp_auth(int udp_socket, struct sockaddr_in client_address){

        socklen_t addr_len = sizeof(client_address);        
        reply_buffer[0] = 0x01; // Message type (REPLY)
        message_id++; 
        uint8_t server_id[3];
        server_id[0] = (uint8_t)(message_id & 0xFF);
        server_id[1] = (uint8_t)((message_id >> 8) & 0xFF);

        
        memcpy(reply_buffer + 1, &server_id, sizeof(uint16_t));
        reply_buffer[3] = 1; 
        
        memcpy(reply_buffer + 4, &ref_id, sizeof(uint8_t)); 

        char* message_content = "Success";
        memcpy(reply_buffer + 6, message_content, strlen(message_content)); 

        // Append the null terminator at the end of the MessageContents
        reply_buffer[6 + strlen(message_content)] = '\0';

        
        size_t reply_length = strlen(reply_buffer) + 1; // Include the null terminator

        sendto(udp_socket, reply_buffer, reply_length + 11, 0, (struct sockaddr *)&client_address, addr_len);

        printf("SENT %s:%d | REPLY\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
}

void udp_message(struct Client user[MAX_CLIENTS],int client_sockets[], struct sockaddr_in client_address){
        

        

        //socklen_t len = sizeof(user->address);
        // getpeername(client_sockets, (struct sockaddr *) &client_address, &len); //TODO mozna pouzit neco jineho
        printf("RECV %s:%d | MSG\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
        // printf("message from %s\n",user->channel_name);
        message_id++;
        ref_id[0] = (uint8_t)(message_id & 0xFF);
        ref_id[1] = (uint8_t)((message_id >> 8) & 0xFF);
        memcpy(buffer + 1, &message_id,sizeof(u_int16_t));

        
        int name_len = strlen(buffer + 3) + 1; 
        int message_len = strlen(buffer + 3 + name_len) + 1; 
        int length = 3 + name_len + message_len;  

        char *disp_tmp = buffer + 3;

        int id;
         for (int i = 0; i < MAX_CLIENTS; i++) {
                    // memcmp(&user[j].address.sin_addr, &client_address.sin_addr, sizeof(struct in_addr)) != 0 && 
                    if( user[i].display_name != NULL && strcmp(user[i].display_name,disp_tmp) == 0){
                        id = i;
                    }
                }
        
        for (int j = 0; j < MAX_CLIENTS; j++) { //Sending messages from one client to others

                socklen_t len = sizeof(user[j].address);
                int dest_socket = client_sockets[j];
                
                if(user[j].protocol != NULL && user[j].channel_name != NULL  && user[id].channel_name != NULL && strcmp(user[j].channel_name,user[id].channel_name) == 0){
                    
                
                     if (strcmp(user[j].protocol, "udp") == 0) {
                        if (user[j].address.sin_port != client_address.sin_port){ //udp
                
                            sendto(dest_socket, buffer, length, 0, (struct sockaddr *)&user[j].address, len);
                        }
                     }else if(user[j].address.sin_port != client_address.sin_port && strcmp(user[j].protocol, "tcp") == 0){
                
                            memset(reply_buffer, 0, BUFFER_SIZE); 

                            char *display_name = buffer + 3;
                            char *MessageContent = buffer + 3+ strlen(display_name) + 1;

                            
                            
                
                            snprintf(reply_buffer,BUFFER_SIZE,"MSG FROM %s IS %s\r\n",display_name,MessageContent);
                            
                            send(dest_socket, reply_buffer, strlen(reply_buffer), 0); //tcp

                            memset(reply_buffer, 0, BUFFER_SIZE); 
                            }
                        // }
                    
                    
                }
            }

        }



void handle_udp_packet(int udp_socket,int client_sockets[],struct sockaddr_in client_address,struct Client user[]) {
    // Receive and process UDP packet    
    socklen_t addr_len = sizeof(client_address);
    ssize_t bytes_rx = recvfrom(udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_address, &addr_len);
    

    if (bytes_rx < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            // No data available, handle accordingly (e.g., continue or retry later)
            return;
        } else {
            perror("recvfrom");
            return;
        }
    }
    
    if(buffer[0] == 0x02) { // AUTH

        //   1 byte       2 bytes
        // +--------+--------+--------+-----~~-----+---+-------~~------+---+----~~----+---+
        // |  0x02  |    MessageID    |  Username  | 0 |  DisplayName  | 0 |  Secret  | 0 |
        // +--------+--------+--------+-----~~-----+---+-------~~------+---+----~~----+---+
        char *username = buffer + 3;
        char *display_name = buffer + strlen(username) + 1 + 3;
    
        printf("RECV %s:%d | AUTH\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));


        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (user[i].socket_sd == 0) {
                user[i].socket_sd = udp_socket;
                user[i].display_name = strdup(display_name);
                // user[i].display_name = display_name;
                printf("display: %s\n",user[i].display_name);
                user[i].address = client_address;
                user[i].channel_name = "general";
                user[i].protocol = "udp";
                user[i].authenticated = true;
                user[i].address.sin_addr = client_address.sin_addr;
                
                //printf("AUTHHH: %d client name: %s and channel name: %s\n",i,user[i].display_name,user[i].channel_name);
                break;
            }
        }
        confirm(buffer,udp_socket,client_address);

        //printf("auth channel: %s, display name: %s\n",user->channel_name,user->display_name);
        
        memset(reply_buffer, 0, BUFFER_SIZE); 
    
        udp_auth(udp_socket,client_address);

        memset(buffer, 0, BUFFER_SIZE); 
        memset(reply_buffer, 0, BUFFER_SIZE); 

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = udp_socket;
                
                break;
            }
        }

    }else if(buffer[0] == 0x04){ // MSG
                            
        //   1 byte       2 bytes
        // +--------+--------+--------+-------~~------+---+--------~~---------+---+
        // |  0x04  |    MessageID    |  DisplayName  | 0 |  MessageContents  | 0 |

        // memset(buffer, 0, BUFFER_SIZE); 
    
        
        confirm(buffer,udp_socket,client_address);
        
        udp_message(user,client_sockets,client_address);

        
        memset(buffer, 0, BUFFER_SIZE);
        // }
    }else if(buffer[0] == (char)0xFF){    
        uint16_t ref_messageID = *(uint16_t*)(buffer + 1);
        uint8_t ref_id[3];
        ref_id[0] = (uint8_t)(ref_messageID & 0xFF);
        ref_id[1] = (uint8_t)((ref_messageID >> 8) & 0xFF);

        memset(reply_buffer, 0, BUFFER_SIZE); 

        reply_buffer[0] = 0x00;
        memcpy(reply_buffer + 1, &ref_messageID,sizeof(u_int16_t));
        sendto(udp_socket, reply_buffer, 3, 0, (struct sockaddr *)&client_address, addr_len);
        
        memset(reply_buffer, 0, BUFFER_SIZE); 

        // go = false;
    }else if(buffer[0] == 0x03 ){ //join
        //   1 byte       2 bytes
        // +--------+--------+--------+-----~~-----+---+-------~~------+---+
        // |  0x03  |    MessageID    |  ChannelID | 0 |  DisplayName  | 0 |
        // +--------+--------+--------+-----~~-----+---+-------~~------+---+


        char *channel_name = buffer + 3;
        char *display = buffer + 3 + strlen(channel_name) + 1;
                            

        
        for (int i = 0; i < MAX_CLIENTS; i++) {

            if (user[i].display_name != NULL && strcmp(user[i].display_name, display) == 0) {
                user[i].channel_name = strdup(channel_name);

                break;
            }
        }
        confirm(buffer,udp_socket,client_address);

        // printf("join channel: %s, display name: %s\n",user->channel_name,user->display_name);

        memset(buffer, 0, BUFFER_SIZE); 

    }
}



void server(char ip_addr[],uint16_t port,uint16_t udp_timeout, uint8_t udp_ret){
    printf("%d%d%d%s",port,udp_ret,udp_timeout,ip_addr);
    struct Client user[MAX_CLIENTS] = {0};
    
    
    // Set of socket descriptors
    fd_set readfds;
    int max_sd = 0; // Maximum socket descriptor

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
    //TCP socket creation
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

    if (bind(welcome_socket, address, address_size) < 0) { //TOOD: &address nebo address ?
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

        
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add the master socket to the set
        // Add TCP socket and UDP socket to the set
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
            
            tcp_accept(welcome_socket,client_sockets);
         
        }
        /* ---------- ACCEPT ---------- */

        // if (FD_ISSET(udp_socket, &readfds) && go) {
        //     printf("UDPSOCKET before: %d\n",udp_socket);
        //     handle_udp_packet(udp_socket,client_sockets,client_address,user);
        //     printf("UDPSOCKET after: %d\n",udp_socket);
        //     go = false;
        // }

        
        
        if (FD_ISSET(udp_socket, &readfds)) {
            handle_udp_packet(udp_socket,client_sockets,client_address,user);
            
        }
        
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

                            user[i].authenticated = true;
                            user[i].socket_sd = sd;
                            user[i].protocol = "tcp";
                            user[i].address.sin_addr = client_address.sin_addr;
                            
                            
                            printf("RECV %s:%d | AUTH\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                            
                            /* TODO UDELAT JEDNOSUSSI*/
                            char *login_message = buffer + 5; // Move past "AUTH "
                            char *display_name_start = strstr(login_message, "AS") + 3;
                            char *display_name_end = strstr(display_name_start, " ");
                            *display_name_end = '\0'; // Null-terminate the display name
                            char *display_name = strdup(display_name_start);

                            
                            
                            user[i].display_name = display_name;
                            
                            /* TODO UDELAT JEDNOSUSSI*/
                            



                            memset(buffer, 0, BUFFER_SIZE);
                            snprintf(buffer, BUFFER_SIZE, "REPLY OK IS Sucesful auth\r\n"); 
                            send(sd, buffer, strlen(buffer), 0);
                            printf("SENT %s:%d | REPLY\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

                            user[i].channel_name = "general";

                            // for (int j = 0; j < MAX_CLIENTS; j++) { // TODO BUGUJE
                            //         if (client_sockets[j] > 0 && client_sockets[j] != sd && strcmp(user[i].channel_name, user[j].channel_name) == 0) {
                            //             // Send notification to other clients
                            //             memset(buffer, 0, BUFFER_SIZE);
                            //             snprintf(buffer, BUFFER_SIZE, "MSG FROM Server IS %s joined the %s\r\n", user[i].display_name,user[i].channel_name);
                            //             send(client_sockets[j], buffer, strlen(buffer), 0);
                            //         }
                            // }

                            
                            
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
                            // for (int j = 0; j < MAX_CLIENTS; j++) {
                            //         if (client_sockets[j] > 0 && client_sockets[j] != sd && strcmp(user[i].channel_name, user[j].channel_name) == 0) {
                            //             // Send notification to other clients
                            //             memset(buffer, 0, BUFFER_SIZE);
                            //             snprintf(buffer, BUFFER_SIZE, "MSG FROM Server IS %s joined the %s\r\n", user[i].display_name,user[i].channel_name);
                            //             send(client_sockets[j], buffer, strlen(buffer), 0);
                            //         }
                            // }

                        }else if(strncmp(buffer,"MSG",3) == 0){
                            //MSG FROM displej IS zprava

                            
                            

                            printf("RECV %s:%d | MSG\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                            char buffer_copy[BUFFER_SIZE];
                            strcpy(buffer_copy, buffer);

                            

                            char *MessageContent = strstr(buffer_copy, "IS"); //strcasestr
                            
                            if (MessageContent != NULL) {
                                MessageContent += 3;

                                //todo nelibi se mi to 
                                        char *display_name_start = buffer_copy + 9; // Assuming display_name starts after "MSG FROM"
                                        char *display_name_end = strstr(display_name_start, " IS");
                                        if (display_name_end != NULL) {
                                            *display_name_end = '\0'; // Null-terminate the display name
                                            char *display_name = strdup(display_name_start);

                                            // Update user[i].display_name
                                            user[i].display_name = display_name;
                                        }
                                
                                // char *display_name = malloc(sizeof(char*));
                                // strcpy(display_name,user[i].display_name);
                                
                                memset(buffer, 0, BUFFER_SIZE);
                                
                                snprintf(buffer, BUFFER_SIZE, "MSG FROM %s IS %s",user[i].display_name,MessageContent);
                                
                                for (int j = 0; j < MAX_CLIENTS; j++) { //Sending messages from one client to others
                                    int dest_socket = client_sockets[j];
                                
                                    // if (dest_socket != sd && dest_socket > 0 && strcmp(user[i].channel_name, user[j].channel_name) == 0 && client_sockets[j] != udp_socket) {
                                    if (dest_socket != sd && dest_socket > 0  && client_sockets[j] != udp_socket && strcmp(user[i].channel_name,user[j].channel_name) == 0) { // TODO prozatim bez joinu
                                        
                                        // printf("tady nemam byt\n");
                                        send(dest_socket, buffer, strlen(buffer), 0);
                                
                                    }
                                    if(udp_socket != 0 && user[i].display_name != NULL && user[j].display_name != NULL && MessageContent != NULL  && strcmp(user[i].channel_name,user[j].channel_name) == 0  ) { 
                                            printf("jsem tady");
                                        MessageContent[strcspn(MessageContent, "\r\n")] = 0; 
                                        
                                        memset(reply_buffer, 0, BUFFER_SIZE);
                                        reply_buffer[0] = 0x04;

                                        message_id++;
                                        uint8_t server_id[3];
                                        server_id[0] = (uint8_t)(message_id & 0xFF);
                                        server_id[1] = (uint8_t)((message_id >> 8) & 0xFF);
                                        memcpy(reply_buffer + 1, &server_id,sizeof(u_int16_t));
                                        
                                        memcpy(reply_buffer + 3, user[i].display_name,sizeof(char*));

                                        memcpy(reply_buffer + 3 + strlen(user[i].display_name) + 1, MessageContent,sizeof(char*));
                                        
                                        int length = 3 + strlen(user[i].display_name) + 1 + strlen(MessageContent) + 1;
    
                                        sendto(udp_socket, reply_buffer, length, 0, (struct sockaddr *)&user[j].address, address_size);
                                                                                
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

                                // free(user[i].channel_name);
                                free(user[i].display_name);
                                
                                client_sockets[i] = 0;
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
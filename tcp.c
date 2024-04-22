#include "tcp.h"
#include "common.h"

extern char buffer[BUFFER_SIZE];
extern char reply_buffer[BUFFER_SIZE];
extern uint8_t ref_id[3];

/**
 * @brief Accepting welcome sockets and creating new for the client
 * 
 * @param tcp_socket 
 * @param client_sockets 
 * @param client_address 
 */
void tcp_accept(int tcp_socket,int client_sockets[],struct sockaddr_in client_address){
    
    socklen_t client_address_size = sizeof(client_address);
    int new_socket = accept(tcp_socket, (struct sockaddr *) &client_address, &client_address_size);
    if (new_socket < 0) {
        fprintf(stderr,"ERR: with accept");
    }
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] == 0) {
            client_sockets[i] = new_socket;
            break;
        }
    }
}

/**
 * @brief User authentication
 * 
 * @param user 
 * @param sd 
 * @param client_address 
 * @param i 
 * @param client_sockets 
 * @param udp_socket 
 * @param address_size 
 */
void tcp_auth(struct Client user[MAX_CLIENTS],int sd,struct sockaddr_in client_address,int i,int client_sockets[], int udp_socket,int address_size){
    char buffer_copy[BUFFER_SIZE];
    strcpy(buffer_copy, buffer);

    user[i].authenticated = true;
    user[i].socket_sd = sd;
    user[i].protocol = "tcp";
    user[i].address.sin_addr = client_address.sin_addr;
    
    
    printf("RECV %s:%d | AUTH\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
    
    char *login_message = buffer + 5; 
    char *display_name_start = strstr(login_message, "AS") + 3;
    char *display_name_end = strstr(display_name_start, " ");
    *display_name_end = '\0'; 
    char *display_name = strdup(display_name_start);
    
    memset(buffer, 0, BUFFER_SIZE);
    snprintf(buffer, BUFFER_SIZE, "REPLY OK IS Sucesful auth\r\n"); 
    send(sd, buffer, strlen(buffer), 0);
    printf("SENT %s:%d | REPLY\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

    user[i].channel_name = "general";
    user[i].display_name = display_name;
    memset(buffer, 0, BUFFER_SIZE);
    snprintf(buffer, BUFFER_SIZE, "MSG FROM Server IS %s joined the %s\r\n", user[i].display_name,user[i].channel_name);

    char MessageContent[BUFFER_SIZE];
    int message_length;
    if(user[i].display_name != NULL && user[i].channel_name != NULL){
        message_length = strlen(user[i].display_name) + strlen(user[i].channel_name) + 21 ;
        snprintf(MessageContent, message_length, "%s joined the %s", user[i].display_name,user[i].channel_name);
        
    }

    //Sending message
    for (int j = 0; j < MAX_CLIENTS; j++) { 
            // TCP
            if (client_sockets[j] > 0 && client_sockets[j] != sd && strcmp(user[i].channel_name, user[j].channel_name) == 0) {
                
                send(client_sockets[j], buffer, strlen(buffer), 0);
            }
            // UDP
            if(udp_socket != 0 && user[i].display_name != NULL && user[j].display_name != NULL  && strcmp(user[i].channel_name,user[j].channel_name) == 0  ) { 
            
                char *server_name = "Server";
                memset(reply_buffer, 0, BUFFER_SIZE);
                reply_buffer[0] = 0x04;

                message_id++;
                uint8_t server_id[3];
                server_id[0] = (uint8_t)(message_id & 0xFF);
                server_id[1] = (uint8_t)((message_id >> 8) & 0xFF);
                memcpy(reply_buffer + 1, &server_id,sizeof(u_int16_t));
                
                memcpy(reply_buffer + 3, server_name,sizeof(char*));

                memcpy(reply_buffer + 3 + strlen(server_name) + 1, MessageContent,message_length);
                
                int length = 3 + strlen(server_name) + 1 + strlen(MessageContent) + 1;

                sendto(udp_socket, reply_buffer, length, 0, (struct sockaddr *)&user[j].address, address_size);
                                                        
            } 
    }

                            
}


/**
 * @brief Joining
 * 
 * @param user 
 * @param client_address 
 * @param i 
 * @param sd 
 * @param client_sockets 
 * @param udp_socket 
 * @param address_size 
 */
void tcp_join(struct Client user[MAX_CLIENTS],struct sockaddr_in client_address,int i,int sd,int client_sockets[],int udp_socket,int address_size){
  
    printf("RECV %s:%d | JOIN\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
    char *token =  buffer + 4;
    token = strtok(token," ");
    user[i].channel_name = strdup(token);
    memset(buffer, 0, BUFFER_SIZE);
    snprintf(buffer, BUFFER_SIZE, "REPLY OK IS Sucesful join\r\n");
    send(sd, buffer, strlen(buffer), 0); 
    printf("SENT %s:%d | REPLY\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

    memset(buffer, 0, BUFFER_SIZE);
    snprintf(buffer, BUFFER_SIZE, "MSG FROM Server IS %s joined the %s\r\n", user[i].display_name,user[i].channel_name);

    char MessageContent[BUFFER_SIZE];
    int message_length;
    if(user[i].display_name != NULL && user[i].channel_name != NULL){
        message_length = strlen(user[i].display_name) + strlen(user[i].channel_name) + 21 ;
        snprintf(MessageContent, message_length, "%s joined the %s", user[i].display_name,user[i].channel_name);
    }

    // Sending message
    for (int j = 0; j < MAX_CLIENTS; j++) {
            // TCP
            if (client_sockets[j] > 0 && client_sockets[j] != sd && strcmp(user[i].channel_name, user[j].channel_name) == 0) {
                
                send(client_sockets[j], buffer, strlen(buffer), 0);
            }
            // UDP
            if(udp_socket != 0 && user[i].display_name != NULL && user[j].display_name != NULL  && strcmp(user[i].channel_name,user[j].channel_name) == 0  ) { 
                    
                char *server_name = "Server";
                memset(reply_buffer, 0, BUFFER_SIZE);
                reply_buffer[0] = 0x04;

                message_id++;
                uint8_t server_id[3];
                server_id[0] = (uint8_t)(message_id & 0xFF);
                server_id[1] = (uint8_t)((message_id >> 8) & 0xFF);
                memcpy(reply_buffer + 1, &server_id,sizeof(u_int16_t));
                
                memcpy(reply_buffer + 3, server_name,sizeof(char*));

                memcpy(reply_buffer + 3 + strlen(server_name) + 1, MessageContent,message_length);
                
                int length = 3 + strlen(server_name) + 1 + strlen(MessageContent) + 1;

                sendto(udp_socket, reply_buffer, length, 0, (struct sockaddr *)&user[j].address, address_size);
                                                        
            } 
    }
}

/**
 * @brief Sending message to all users
 * 
 * @param user 
 * @param client_address 
 * @param i 
 * @param sd 
 * @param client_sockets 
 * @param udp_socket 
 * @param address_size 
 */
void tcp_msg(struct Client user[MAX_CLIENTS],struct sockaddr_in client_address,int i,int sd,int client_sockets[], int udp_socket,int address_size){
    printf("RECV %s:%d | MSG\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
    char buffer_copy[BUFFER_SIZE];
    strcpy(buffer_copy, buffer);

    char *MessageContent = strstr(buffer_copy, "IS"); 
    
    if (MessageContent != NULL) {
        MessageContent += 3;

        
        char *display_name_start = buffer_copy + 9; 
        char *display_name_end = strstr(display_name_start, " IS");
        if (display_name_end != NULL) {
            *display_name_end = '\0'; 
            char *display_name = strdup(display_name_start);
            user[i].display_name = display_name;
        }
        
        memset(buffer, 0, BUFFER_SIZE);
        
        snprintf(buffer, BUFFER_SIZE, "MSG FROM %s IS %s",user[i].display_name,MessageContent);
        
        for (int j = 0; j < MAX_CLIENTS; j++) { 
            int dest_socket = client_sockets[j];
        
            
            if (dest_socket != sd && dest_socket > 0  && client_sockets[j] != udp_socket && strcmp(user[i].channel_name,user[j].channel_name) == 0) { 
            
                send(dest_socket, buffer, strlen(buffer), 0);
        
            }
            if(udp_socket != 0 && user[i].display_name != NULL && user[j].display_name != NULL && MessageContent != NULL  && strcmp(user[i].channel_name,user[j].channel_name) == 0  ) { 
                    
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
}

/**
 * @brief Err from user
 * 
 * @param user 
 * @param client_address 
 * @param sd 
 * @param client_sockets 
 * @param i 
 */
void tcp_err(struct Client user[MAX_CLIENTS],struct sockaddr_in client_address,int sd,int client_sockets[],int i){
    printf("RECV %s:%d | ERR\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
    
    for (int j = 0; j < MAX_CLIENTS; j++) {
        if (client_sockets[j] > 0 && client_sockets[j] != sd) {
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
}

/**
 * @brief Bye from user
 * 
 * @param client_sockets 
 * @param client_address 
 * @param user 
 * @param i 
 * @param sd 
 * @param udp_socket 
 * @param address_size 
 */
void tcp_bye(int client_sockets[],struct sockaddr_in client_address,struct Client user[MAX_CLIENTS],int i,int sd,int udp_socket,int address_size){
    printf("RECV %s:%d | BYE\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
    
    memset(buffer, 0, BUFFER_SIZE);
    
    snprintf(buffer, BUFFER_SIZE, "MSG FROM Server IS %s left the %s\r\n", user[i].display_name, user[i].channel_name);

    char MessageContent[BUFFER_SIZE];
    int message_length;
    if(user[i].display_name != NULL && user[i].channel_name != NULL){
        message_length = strlen(user[i].display_name) + strlen(user[i].channel_name) + 21 ;
        snprintf(MessageContent, message_length, "%s left the %s", user[i].display_name,user[i].channel_name);
        
    }

    for (int j = 0; j < MAX_CLIENTS; j++) {
        if (client_sockets[j] > 0 && client_sockets[j] != sd && strcmp(user[i].channel_name, user[j].channel_name) == 0) {
            
            send(client_sockets[j], buffer, strlen(buffer), 0);
        }
        if(udp_socket != 0 && user[i].display_name != NULL && user[j].display_name != NULL  && strcmp(user[i].channel_name,user[j].channel_name) == 0  ) { 
                    
                char *server_name = "Server";
                memset(reply_buffer, 0, BUFFER_SIZE);
                reply_buffer[0] = 0x04;

                message_id++;
                uint8_t server_id[3];
                server_id[0] = (uint8_t)(message_id & 0xFF);
                server_id[1] = (uint8_t)((message_id >> 8) & 0xFF);
                memcpy(reply_buffer + 1, &server_id,sizeof(u_int16_t));
                
                memcpy(reply_buffer + 3, server_name,sizeof(char*));

                memcpy(reply_buffer + 3 + strlen(server_name) + 1, MessageContent,message_length);
                
                int length = 3 + strlen(server_name) + 1 + strlen(MessageContent) + 1;

                sendto(udp_socket, reply_buffer, length, 0, (struct sockaddr *)&user[j].address, address_size);
                                                        
            } 
        
    }
    shutdown(sd, SHUT_RDWR); 
    shutdown(client_sockets[i], SHUT_RDWR); 
    close(sd);
    close(client_sockets[i]);

    free(user[i].display_name);
    
    client_sockets[i] = 0;
}
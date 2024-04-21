#include "udp.h"

#include "common.h"


extern char buffer[BUFFER_SIZE];
extern char reply_buffer[BUFFER_SIZE];


extern uint8_t ref_id[3];

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


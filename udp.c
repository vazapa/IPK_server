#include "udp.h"
#include "common.h"

extern char buffer[BUFFER_SIZE];
extern char reply_buffer[BUFFER_SIZE];
extern uint8_t ref_id[3];

/**
 * @brief Confirming incoming messages
 * 
 * @param buffer 
 * @param udp_socket 
 * @param client_address 
 */
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

/**
 * @brief User authentication
 * 
 * @param udp_socket 
 * @param client_address 
 * @param client_sockets 
 * @param user 
 * @param disp_tmp 
 */
void udp_auth(int udp_socket, struct sockaddr_in client_address,int client_sockets[],struct Client user[],char* disp_tmp){

        socklen_t addr_len = sizeof(client_address);        
        reply_buffer[0] = 0x01; 
        message_id++; 
        uint8_t server_id[3];
        server_id[0] = (uint8_t)(message_id & 0xFF);
        server_id[1] = (uint8_t)((message_id >> 8) & 0xFF);

        
        memcpy(reply_buffer + 1, &server_id, sizeof(uint16_t));
        reply_buffer[3] = 1; 
        
        memcpy(reply_buffer + 4, &ref_id, sizeof(uint8_t)); 

        char* message_content = "Success";
        memcpy(reply_buffer + 6, message_content, strlen(message_content)); 

        
        reply_buffer[6 + strlen(message_content)] = '\0';

        
        size_t reply_length = strlen(reply_buffer) + 1; 

        sendto(udp_socket, reply_buffer, reply_length + 11, 0, (struct sockaddr *)&client_address, addr_len);

        printf("SENT %s:%d | REPLY\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        memset(reply_buffer, 0, BUFFER_SIZE);
        reply_buffer[0] = 0x04;
        char message_content_buf[BUFFER_SIZE];
        char* server_name= "Server";
        int message_length;
        int id;
        for (int i = 0; i < MAX_CLIENTS; i++) {
                    if( user[i].display_name != NULL && strcmp(user[i].display_name,disp_tmp) == 0){
                        id = i;
                    }
        }
        if(user[id].display_name != NULL && user[id].channel_name != NULL){
            message_length = strlen(user[id].display_name) + strlen(user[id].channel_name) + 21 ;
            snprintf(message_content_buf, message_length, "%s joined the %s", user[id].display_name,user[id].channel_name);
            
        }

        message_id++;
        server_id[0] = (uint8_t)(message_id & 0xFF);
        server_id[1] = (uint8_t)((message_id >> 8) & 0xFF);
        memcpy(reply_buffer + 1, &server_id,sizeof(u_int16_t));
        
        memcpy(reply_buffer + 3, server_name,strlen(server_name));

        memcpy(reply_buffer + 3 + strlen(server_name) + 1, message_content_buf,strlen(message_content_buf));
        
        int length = 3 + strlen(server_name) + 1 + strlen(message_content_buf) + 1;


        // Sending joined general msg
        for (int j = 0; j < MAX_CLIENTS; j++) { 

                socklen_t len = sizeof(user[j].address);
                int dest_socket = client_sockets[j];
                if(user[j].protocol != NULL && user[j].channel_name != NULL  && user[id].channel_name != NULL && strcmp(user[j].channel_name,user[id].channel_name) == 0){
                    
                     if (strcmp(user[j].protocol, "udp") == 0) {
                        
                        sendto(dest_socket, reply_buffer, length, 0, (struct sockaddr *)&user[j].address, len);
                        
                     }else if(strcmp(user[j].protocol, "tcp") == 0){
                
                            memset(reply_buffer, 0, BUFFER_SIZE); 
                
                            snprintf(reply_buffer,BUFFER_SIZE,"MSG FROM Server IS %s\r\n",message_content_buf);
                            
                            send(dest_socket, reply_buffer, strlen(reply_buffer), 0); 

                            memset(reply_buffer, 0, BUFFER_SIZE); 
                        }
                }
            }
        printf("RECV %s:%d | CONFIRM\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
}

/**
 * @brief Sending message to clients in same channel
 * 
 * @param user 
 * @param client_sockets 
 * @param client_address 
 */
void udp_message(struct Client user[MAX_CLIENTS],int client_sockets[], struct sockaddr_in client_address){ //msg
        
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

                    if( user[i].display_name != NULL && strcmp(user[i].display_name,disp_tmp) == 0){
                        id = i;
                    }
                }
        
        for (int j = 0; j < MAX_CLIENTS; j++) {

                socklen_t len = sizeof(user[j].address);
                int dest_socket = client_sockets[j];
                
                if(user[j].protocol != NULL && user[j].channel_name != NULL  && user[id].channel_name != NULL && strcmp(user[j].channel_name,user[id].channel_name) == 0){
                    
                     if (strcmp(user[j].protocol, "udp") == 0) {
                        if (user[j].address.sin_port != client_address.sin_port){ 
                
                            sendto(dest_socket, buffer, length, 0, (struct sockaddr *)&user[j].address, len);
                        }
                     }else if(user[j].address.sin_port != client_address.sin_port && strcmp(user[j].protocol, "tcp") == 0){
                
                            memset(reply_buffer, 0, BUFFER_SIZE); 

                            char *display_name = buffer + 3;
                            char *MessageContent = buffer + 3+ strlen(display_name) + 1;

                            snprintf(reply_buffer,BUFFER_SIZE,"MSG FROM %s IS %s\r\n",display_name,MessageContent);
                            
                            send(dest_socket, reply_buffer, strlen(reply_buffer), 0); 

                            memset(reply_buffer, 0, BUFFER_SIZE); 
                        }                    
                }
            }

}


/**
 * @brief Main udp function
 * 
 * @param udp_socket 
 * @param client_sockets 
 * @param client_address 
 * @param user 
 */
void handle_udp_packet(int udp_socket,int client_sockets[],struct sockaddr_in client_address,struct Client user[]) {
    
    socklen_t addr_len = sizeof(client_address);
    ssize_t bytes_rx = recvfrom(udp_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_address, &addr_len);
    

    if (bytes_rx < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            // No data
            return;
        } else {
            fprintf(stderr,"ERR: with recvfrom");
            return;
        }
    }
    
    if(buffer[0] == 0x02) { // AUTH

        char *username = buffer + 3;
        char *display_name = buffer + strlen(username) + 1 + 3;
    
        printf("RECV %s:%d | AUTH\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));


        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (user[i].socket_sd == 0) {
                user[i].socket_sd = udp_socket;
                user[i].display_name = strdup(display_name);
                
                user[i].address = client_address;
                user[i].channel_name = "general";
                user[i].protocol = "udp";
                user[i].authenticated = true;
                user[i].address.sin_addr = client_address.sin_addr;
            
                break;
            }
        }
        confirm(buffer,udp_socket,client_address);
        
        memset(reply_buffer, 0, BUFFER_SIZE); 
    
        udp_auth(udp_socket,client_address,client_sockets,user,display_name);

        memset(buffer, 0, BUFFER_SIZE); 
        memset(reply_buffer, 0, BUFFER_SIZE); 

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] == 0) {
                client_sockets[i] = udp_socket;
                
                break;
            }
        }

    }else if(buffer[0] == 0x04){ // MSG
                            
        printf("RECV %s:%d | MSG\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        confirm(buffer,udp_socket,client_address);

        udp_message(user,client_sockets,client_address);

        
        memset(buffer, 0, BUFFER_SIZE);
        
    }else if(buffer[0] == (char)0xFF){    // err
        
        confirm(buffer,udp_socket,client_address);

        memset(buffer, 0, BUFFER_SIZE); 

        
    }else if(buffer[0] == 0x03 ){ //join
        
        char *channel_name = buffer + 3;
        char *display = buffer + 3 + strlen(channel_name) + 1;
                            
        for (int i = 0; i < MAX_CLIENTS; i++) {

            if (user[i].display_name != NULL && strcmp(user[i].display_name, display) == 0) {
                user[i].channel_name = strdup(channel_name);

                break;
            }
        }
        confirm(buffer,udp_socket,client_address);

        memset(reply_buffer, 0, BUFFER_SIZE);
        reply_buffer[0] = 0x04;
        char message_content_buf[BUFFER_SIZE];
        char* server_name= "Server";
        int message_length;
        int id;
        for (int i = 0; i < MAX_CLIENTS; i++) {
                    if( user[i].display_name != NULL && strcmp(user[i].display_name,display) == 0){
                        id = i;
                    }
        }
        if(user[id].display_name != NULL && user[id].channel_name != NULL){
            message_length = strlen(user[id].display_name) + strlen(user[id].channel_name) + 21 ;
            snprintf(message_content_buf, message_length, "%s joined the %s", user[id].display_name,user[id].channel_name);
        }

        message_id++;
        uint8_t server_id[3];
        server_id[0] = (uint8_t)(message_id & 0xFF);
        server_id[1] = (uint8_t)((message_id >> 8) & 0xFF);
        memcpy(reply_buffer + 1, &server_id,sizeof(u_int16_t));
        
        memcpy(reply_buffer + 3, server_name,strlen(server_name));

        memcpy(reply_buffer + 3 + strlen(server_name) + 1, message_content_buf,strlen(message_content_buf));
        
        int length = 3 + strlen(server_name) + 1 + strlen(message_content_buf) + 1;

        for (int j = 0; j < MAX_CLIENTS; j++) { 

                socklen_t len = sizeof(user[j].address);
                int dest_socket = client_sockets[j];
                
                if(user[j].protocol != NULL && user[j].channel_name != NULL  && user[id].channel_name != NULL && strcmp(user[j].channel_name,user[id].channel_name) == 0){
                    
                
                     if (strcmp(user[j].protocol, "udp") == 0) {
                        
                        sendto(dest_socket, reply_buffer, length, 0, (struct sockaddr *)&user[j].address, len);
                        
                     }else if(strcmp(user[j].protocol, "tcp") == 0){
                
                            memset(reply_buffer, 0, BUFFER_SIZE); 
                
                            snprintf(reply_buffer,BUFFER_SIZE,"MSG FROM Server IS %s\r\n",message_content_buf);
                            
                            send(dest_socket, reply_buffer, strlen(reply_buffer), 0);

                            memset(reply_buffer, 0, BUFFER_SIZE); 
                        }
                }
            }
        memset(buffer, 0, BUFFER_SIZE); 
    }
}


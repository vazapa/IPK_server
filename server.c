#include "server.h"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 3

char *MessageContent;
char *display_name;
char buffer[];

void server(char ip_addr[],uint16_t port,uint16_t udp_timeout, uint8_t udp_ret){
    

    //TCP socket creation
    int welcome_socket = socket(AF_INET, SOCK_STREAM, 0); 
    if (welcome_socket <= 0)
    {
        fprintf(stderr,"ERR: with creating TCP socket");
    }
    int enable = 1; // TOOD umoznuje rychle znovu nastaveni portu, idk jestli to mame pouzit
    setsockopt(welcome_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));


    struct sockaddr_in server_address; 

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET; 
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); 
    server_address.sin_port = htons(port);
    struct sockaddr *address = (struct sockaddr *) &server_address; 
    int address_size = sizeof(server_address);
    
    if (bind(welcome_socket, address, address_size) < 0) { //TOOD: &address nebo address ?
        fprintf(stderr,"ERR: with TCP bind");
    }

    int max_waiting_connections = 2; // TODO zvysit
    if (listen(welcome_socket, max_waiting_connections) < 0) {
        fprintf(stderr,"ERR: with TCP listen");
    }

    struct sockaddr *comm_addr = NULL;
    socklen_t comm_addr_size;

 // Set of socket descriptors
    fd_set readfds;
    int max_sd; // Maximum socket descriptor

    // Client socket descriptors
    int client_sockets[MAX_CLIENTS] = {0};

    // Main loop
    while (1) {
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

        // If something happened on the master socket, then it's an incoming connection
        if (FD_ISSET(welcome_socket, &readfds)) {
            struct sockaddr_in client_address;
            socklen_t client_address_size = sizeof(client_address);
            int new_socket = accept(welcome_socket, (struct sockaddr *) &client_address, &client_address_size);
            if (new_socket < 0) {
                perror("ERR: accepting connection");
                exit(EXIT_FAILURE);
            }

            // Inform user of socket number - used in send and receive commands
            printf("New connection, socket fd is %d, ip is: %s, port is %d\n", new_socket, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

            // Add new socket to array of sockets
            for (int i = 0; i < MAX_CLIENTS; i++) {
                // If position is empty
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        

        // Else it's some IO operation on some other socket
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                // Check if it was for closing, and also read the incoming message
                memset(buffer, 0, BUFFER_SIZE);
                int valread = recv(sd, buffer, BUFFER_SIZE, 0);
                if (valread == 0) {
                    // Somebody disconnected, get his details and print
                    struct sockaddr_in client_address;
                    socklen_t client_address_size = sizeof(client_address);
                    getpeername(sd, (struct sockaddr *) &client_address, &client_address_size); //TODO smazat
                    printf("Host disconnected, ip %s, port %d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

                    // Close the socket and mark as 0 in list for reuse
                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    if(strncmp(buffer,"AUTH",4) == 0){ // TODO checknout spravnost
                        memset(buffer, 0, BUFFER_SIZE);
                        snprintf(buffer, BUFFER_SIZE, "REPLY OK IS Sucesful auth\r\n"); 
                        send(sd, buffer, strlen(buffer), 0);
                        continue;
                    }else if(strncmp(buffer,"MSG",3) == 0){
                        //MSG FROM displej IS zprava

                        char buffer_copy[BUFFER_SIZE];
                        strcpy(buffer_copy, buffer);
                        MessageContent = strstr(buffer_copy, "IS"); //strcasestr
                        
                        if (MessageContent != NULL) {
                            display_name = strtok(buffer_copy + 9, " ");
                            MessageContent += 3;
                            printf("%s: %s",display_name, MessageContent);
                        }
                    }
                    memset(buffer, 0, BUFFER_SIZE);
                    
                    // Echo back the message that came in
                    // buffer[valread] = '\0'; // Null-terminate the string
                    // printf("Message: %s\n",buffer);
                    // send(sd, buffer, strlen(buffer), 0);
                }
            }
        }
    


}
    shutdown(welcome_socket, SHUT_RDWR); 
    close(welcome_socket); 
}
#include "server.h"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 3

/* TODO
- poslat zpravu vsem ze uzivatel se pripojil
- poslat zpravu vsem klientum ze uzivatel leavnul
- join vyresit
- asi kontrolovani spravnosti loginu atd
*/


struct Client {
    int socket_sd;
    char *display_name; // TODO max cisla
};


char *MessageContent;
// char *display_name;
char buffer[];
volatile sig_atomic_t keepRunning = 1;

void intHandler(int sig) {
    keepRunning = 0;
}

void server(char ip_addr[],uint16_t port,uint16_t udp_timeout, uint8_t udp_ret){
    
    struct Client user[MAX_CLIENTS];


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
    signal(SIGINT, intHandler); //ctrl+c and ctrl+c


    int flags = fcntl(welcome_socket, F_GETFL, 0);
    fcntl(welcome_socket, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in client_address;
    socklen_t client_address_size = sizeof(client_address);
    // Main loop
    while (keepRunning) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add the master socket to the set
        FD_SET(welcome_socket, &readfds);
        max_sd = welcome_socket;

        // Add child sockets to set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            user->socket_sd = client_sockets[i];

            // If valid socket descriptor, add to read list
            if (user->socket_sd > 0)
                FD_SET(user->socket_sd, &readfds);

            // Get the highest file descriptor number
            if (user->socket_sd > max_sd)
                max_sd = user->socket_sd;
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
                // perror("ERR: accepting connection"); // TODO pri ctrl+c to dela bordel
                // exit(EXIT_FAILURE);
                break;
            }
            // Inform user of socket number - used in send and receive commands
            // printf("New connection, socket fd is %d, ip is: %s, port is %d\n", new_socket, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

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
            user->socket_sd = client_sockets[i];

            

            if (FD_ISSET(user->socket_sd, &readfds)) {
                // Check if it was for closing, and also read the incoming message
                getpeername(user->socket_sd, (struct sockaddr *) &client_address, &client_address_size); //TODO mozna pouzit neco jineho
                memset(buffer, 0, BUFFER_SIZE);
                int valread = recv(user->socket_sd, buffer, BUFFER_SIZE, 0);
                if (valread == 0) {
                    // Somebody disconnected, get his details and print
                    // printf("Host disconnected, ip %s, port %d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

                    // Close the socket and mark as 0 in list for reuse
                    close(user->socket_sd);
                    client_sockets[i] = 0;
                } else {
                    if(strncmp(buffer,"AUTH",4) == 0){ // TODO checknout spravnost
                        printf("RECV %s:%d | AUTH\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                        memset(buffer, 0, BUFFER_SIZE);
                        snprintf(buffer, BUFFER_SIZE, "REPLY OK IS Sucesful auth\r\n"); 
                        send(user->socket_sd, buffer, strlen(buffer), 0);
                        printf("SENT %s:%d | REPLY\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                        continue;
                    }else if(strncmp(buffer,"MSG",3) == 0){
                        //MSG FROM displej IS zprava
                        printf("RECV %s:%d | MSG\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                        char buffer_copy[BUFFER_SIZE];
                        strcpy(buffer_copy, buffer);
                        MessageContent = strstr(buffer_copy, "IS"); //strcasestr
                        
                        if (MessageContent != NULL) {
                            user->display_name = strtok(buffer_copy + 9, " ");
                            char *display_name2 = malloc(sizeof(user->display_name));

                            strcpy(display_name2,user->display_name);
                            MessageContent += 3;
                            // printf("%s: %s",display_name, MessageContent);
                            memset(buffer, 0, BUFFER_SIZE);
                            snprintf(buffer, BUFFER_SIZE, "MSG FROM %s IS %s\r\n",display_name2,MessageContent);
                            for (int i = 0; i < MAX_CLIENTS; i++) { //Sending messages from one client to others
                                // int sd = client_sockets[i];
                                if(user->socket_sd != client_sockets[i]){
                                    send(client_sockets[i], buffer, strlen(buffer), 0);
                                }
                            }
                        }
                    }else if(strncmp(buffer,"BYE",3) == 0){
                            printf("RECV %s:%d | BYE\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                            // Iterate over all client sockets
                            for (int i = 0; i < MAX_CLIENTS; i++) {
                                if (client_sockets[i] > 0 && client_sockets[i] != user->socket_sd) {
                                    // Send notification to other clients
                                    memset(buffer, 0, BUFFER_SIZE);
                                    snprintf(buffer, BUFFER_SIZE, "MSG FROM Server IS %s left the channel\n", user->display_name);
                                    send(client_sockets[i], buffer, strlen(buffer), 0);
                                }
                            }
                            close(user->socket_sd);
                            client_sockets[i] = 0;
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
    for (int i = 0; i < MAX_CLIENTS; i++) {
        user->socket_sd = client_sockets[i];
        if (user->socket_sd > 0) {
            // Send BYE message
            getpeername(user->socket_sd, (struct sockaddr *) &client_address, &client_address_size); //TODO mozna pouzit neco jineho
            printf("SENT %s:%d | BYE\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
            send(user->socket_sd, "BYE", strlen("BYE"), 0);
            // Close socket
            close(user->socket_sd);
            // Mark socket as 0 in the list
            client_sockets[i] = 0;
        }
    }
    shutdown(welcome_socket, SHUT_RDWR); 
    close(welcome_socket); 
}
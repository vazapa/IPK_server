#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"

int main(int argc,char *argv[]){
    char ip_addr[INET_ADDRSTRLEN]; 
    strcpy(ip_addr, "0.0.0.0"); 

    uint16_t port = 4567;
    uint16_t udp_timeout = 250;
    uint8_t udp_ret = 3;

    if (argv[1] != NULL &&  strcmp(argv[1], "-h") == 0) {
        printf("Usage: %s -l [IP address] -p [port] -d [UDP confirmation timeout] -r [Max UDP retransmissions]\n", argv[0]);
        exit(0);
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            if (++i < argc)
                port = atoi(argv[i]);
        } else if (strcmp(argv[i], "-d") == 0) {
            if (++i < argc)
                udp_timeout = atoi(argv[i]);
        }else if (strcmp(argv[i], "-r") == 0) {
            if (++i < argc)
                udp_ret = atoi(argv[i]);
        }else if (strcmp(argv[i], "-l") == 0) {
            if (++i < argc)
                strncpy(ip_addr, argv[i], sizeof(ip_addr)); 
        }
    }

    server(ip_addr,port,udp_timeout,udp_ret);
    
    return 0;
}
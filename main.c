#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"

// Argument	Value	Possible values 	Meaning or expected program behaviour
// -l	    0.0.0.0	IP address	        Server listening IP address for welcome sockets
// -p	    4567	uint16	            Server listening port for welcome sockets
// -d	    250	    uint16	            UDP confirmation timeout
// -r	    3	    uint8	            Maximum number of UDP retransmissions
// -h			                        Prints program help output and exits


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
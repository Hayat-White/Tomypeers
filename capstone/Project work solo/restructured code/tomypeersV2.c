#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <WinSock2.h> // Include Winsock library
#include <windows.h>
#pragma comment(lib, "ws2_32.lib") // Link with ws2_32.lib

struct connections{
    uint32_t id;
    int socket_descriptor;
    struct sockaddr_in address;
};



int main(){
    SOCKET connection; // holds information for the current machine
    char host[256]; // array for host(ip) information
    char port[10]; // array for port number
    char command[10]; // this is the command for the terminal
    WSADATA wsaData; // Holds the socket WSADATA from the WSAstartup
    struct connections servers[10];


    
}

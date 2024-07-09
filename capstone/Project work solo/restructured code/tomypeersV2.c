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

void* listen_for_connections(void* param); // Function to listen for incoming connections (runs completely in the background)

int main(){
    SOCKET connection; // holds information for the current machine
    char host[256]; // array for host(ip) information
    char port[10]; // array for port number
    char command[10]; // this is the command for the terminal
    WSADATA wsaData; // Holds the socket WSADATA from the WSAstartup
    struct connections servers[10];
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return 1;
    }

    _beginthreadex(NULL, 0, listen_for_connections, NULL, 0, NULL); // Starts the listening in the background


    
}

// update to handle mutliple connections (fd_set or something else)
void* listen_for_connections(void* param) {
    SOCKET listen_socket;
    struct sockaddr_in server_addr;

    // Create socket for listening
    if ((listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        printf("Error creating listen socket: %d\n", WSAGetLastError());
        return NULL;
    }

    // Setup server address information
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen on all interfaces
    server_addr.sin_port = htons(12172); // Change this to your desired port number

    // Bind the socket
    if (bind(listen_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed with error: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        return NULL;
    }

    // Get the IP address of the local machine
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        printf("Error getting hostname: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        return NULL;
    }

    struct hostent* hostinfo;
    if ((hostinfo = gethostbyname(hostname)) == NULL) {
        printf("Error getting host information: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        return NULL;
    }

    struct in_addr** addr_list = (struct in_addr**)hostinfo->h_addr_list;

    // Display IP address and port number
    printf("\nHosted on IP address: %s\n", inet_ntoa(*addr_list[0])); // Assuming the first address is IPv4
    printf("Hosted on port number: %d\n", ntohs(server_addr.sin_port));

    // Listen for incoming connections
    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed with error: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        return NULL;
    }

    printf("Listening for incoming connections...\n");

    // Accept incoming connections
    SOCKET client_socket;
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    while (1) {
        client_socket = accept(listen_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == INVALID_SOCKET) {
            printf("Accept failed: %d\n", WSAGetLastError());
            closesocket(listen_socket);
            return NULL;
        }

        // Handle the incoming connection here, you can perform necessary actions
        printf("Incoming connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        //sending the list of files to transfer in a list
        send_file_list(client_socket);
        //create internal file_list for checking
        char file_list[2048] = "";
        create_file_list(file_list);
        // Receive and print messages from the client until "exit" is received
        char file_name[128];
        int bytes_received;
        do {
            printf("\nTRYING TO GRAB THE FILE FROM THE USER\n");
            bytes_received = recv(client_socket, file_name, sizeof(file_name), 0);
            //printf("\nServer has recieved: '%i' bytes\n", bytes_received);
            if (bytes_received > 0) {
                file_name[bytes_received] = '\0'; // Null-terminate the received data
                printf("\nFile wanted from connection: %s\n", file_name);
                // check file_list for the file if the file exists start to send the file, if the file doesn't exist send back to the user a message stating it doesn't exist
                if (strstr(file_list, file_name) != NULL) {
                    // use open and send to send the file to the client
                    char upload_path[128];
                    snprintf(upload_path,sizeof(upload_path),"upload/%s",file_name);
                    FILE* file = fopen(upload_path,"rb");
                    if (file){
                        printf("Sending file '%s' to connection...\n",file_name);
                        char file_buffer[3072];
                        size_t bytes_read;
                        while((bytes_read = fread(file_buffer,1,sizeof(file_buffer),file)) > 0){
                            if(send(client_socket,file_buffer,bytes_read,0) == SOCKET_ERROR) {
                                printf("Error sending file data: %d\n", WSAGetLastError());
                                break;
                            }
                        }
                        fclose(file);
                    }else{
                        printf("Error opening file '%s': %d\n", file_name, errno);
                    }
                }else {
                    // File doesn't exist, send back a message to the user
                    const char* message = "File does not exist.";
                    send(client_socket, message, strlen(message), 0);
                }
            } else if (bytes_received == 0) {
                printf("Client disconnected.\n");
            } else {
                printf("Recv error: %d\n", WSAGetLastError());
            }
        } while (bytes_received > 0 && strcmp(file_name, "exit") != 0);

        // Close the client socket
        closesocket(client_socket);
    }

    // Close the listening socket
    closesocket(listen_socket);

    return NULL;
}
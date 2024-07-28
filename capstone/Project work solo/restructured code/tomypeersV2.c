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
void establlish_connection(struct sockaddr_in *server_connecting);
int command_selection();
void list_directory(); 

int main(){
    SOCKET connection; // holds information for the current machine
    char host[256]; // array for host(ip) information
    char port[10]; // array for port number
    int command = 1;
    WSADATA wsaData; // Holds the socket WSADATA from the WSAstartup
    struct sockaddr_in server;
    struct sockaddr_in *server_connecting = &server;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return 1;
    }
    if ((connection = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) { // Creates and stores socket information inside of connection
        printf("Could not create socket: %d\n", WSAGetLastError());
        return 1;
    }
    
    _beginthreadex(NULL, 0, listen_for_connections, NULL, 0, NULL); // Starts the listening in the background
    //add function for commands instead of blocks
    /*Command values:
        upload_list = 2
        connect = 1
        exit = 0
        error = -1
    */
    do{
        command = command_selection();
        if(command == 1){
            establish_connection(server_connecting);
        }else if(command == 2){
            list_directory();
        }else if(command = -1){
            printf("Incorrect command: failed with error\n");
        }
    }while(command != 0);

    
}
//function that connects the users together
void establlish_connection(struct sockaddr_in *server_connecting){

}

void list_directory(){
    WIN32_FIND_DATA find_file_data;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    DWORD dwError;

    // Path to the "upload" directory
    const char* directory_path = "upload\\*";

    // Find the first file in the directory
    hFind = FindFirstFile(directory_path, &find_file_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Error listing directory: %d\n", GetLastError());
        return;
    }

    // Print the list of files
    printf("\nFiles in 'upload' directory:\n");
    do {
        // Check if it's a file (not a directory)
        if (!(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            printf("%s (%llu bytes)\n", find_file_data.cFileName,
                   ((ULONG64)find_file_data.nFileSizeHigh << 32) + find_file_data.nFileSizeLow);
        }
    } while (FindNextFile(hFind, &find_file_data) != 0);
    printf("\n");
    dwError = GetLastError();
    if (dwError != ERROR_NO_MORE_FILES) {
        printf("Error listing directory: %d\n", dwError);
    }

    // Close the handle
    FindClose(hFind);
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

int command_selection(){
    char command[10];
    printf("Enter command (exit, connect, upload_list)> ");
    scanf("%s",command);
    if(strcmp(command, "upload_list") == 0){
        return 2;
    }    
    if(strcmp(command, "connect") == 0){
        return 1;
    }
    if(strcmp(command, "exit") == 0){
        return 0;
    }
    return -1;

}
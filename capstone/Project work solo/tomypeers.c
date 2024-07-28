#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <WinSock2.h> // Include Winsock library
#include <windows.h>

// Define a structure for the hashmap node
typedef struct Node {
    char filename[256];
    size_t filesize;
    struct Node* next;
} Node;

// Define the hashmap structure
#define HASH_SIZE 100
Node* hashmap[HASH_SIZE];

#pragma comment(lib, "ws2_32.lib") // Link with ws2_32.lib
void list_directory(); // Lists the current upload directory of the host
void* listen_for_connections(void* param); // Function to listen for incoming connections (runs completely in the background)
void create_file_list(char* file_list); // creates a filelist 
size_t get_filesize(const char* filename); // Grabs the file size of the requested file

//build a function for connecting

//build a function for handling the data recieving for the purpose of writing that data

//build a function that takes the data from above and actual write the information to a file

//Functions to remove because functionality just isn't there yet and general purpose fruitless
void insert(const char* filename, size_t filesize); // Inserts a new value into the hashmap
size_t hash_function(const char* filename); //hash function
void print_hashmap(); // prints hashmap

int main() {
    SOCKET connection; // holds information for the current machine
    char host[256]; // array for host(ip) information
    char port[10]; // array for port number
    char command[10]; // this is the command for the terminal
    WSADATA wsaData; // Holds the socket WSADATA from the WSAstartup
    struct sockaddr_in server; // Holds information for the host that we are connecting to

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return 1;
    }

    _beginthreadex(NULL, 0, listen_for_connections, NULL, 0, NULL); // Starts the listening in the background

    while ((strcmp(command, "exit") != 0)) {
        printf("Enter command (exit, connect, uplist)> ");
        scanf("%s", command);

        if (strcmp(command, "connect") == 0) {
            printf("Enter ip address to connect to> ");
            scanf("%s", host);
            printf("Enter port number to connect to> ");
            scanf("%s", port);

            if ((connection = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) { // Creates and stores socket information inside of connection
                printf("Could not create socket: %d\n", WSAGetLastError());
                return 1;
            }

            server.sin_family = AF_INET; // sets to IPv4
            server.sin_addr.s_addr = inet_addr(host); // convert char array of host(ip) address
            server.sin_port = htons(atoi(port)); // convert char array of port

            if (connect(connection, (struct sockaddr *)&server, sizeof(server)) < 0) { // Attempts the connection
                printf("Connection failed\n");
                return 1;
            }

            // Connected, send a message
            printf("Connected to server at %s:%s\n", host, port);
            //Capture packets sent by host to display files inside the system
            char file_info[2048];
            int bytes_received = recv(connection, file_info, sizeof(file_info), 0);
            if (bytes_received >= 0) {
                file_info[bytes_received] = '\0'; // Null-terminate the received data
                //printf("Bytes received from sending the file system '%i'",bytes_received);
                printf("\nFiles available for download:\n%s\n", file_info);

                //parse data and populate the hashmap
                char* token = strtok(file_info,"\n");
                while(token != NULL){
                    char filename[256];
                    size_t filesize;
                    sscanf(token,"%s (%lu bytes)", filename, &filesize);
                    insert(filename,filesize);
                    token = strtok(NULL, "\n");
                }
            } else {
                printf("Recv error: %d\n", WSAGetLastError());
            }

            char recv_buffer[4096];
            char send_buffer[128];
            do {
                memset(send_buffer,0,sizeof(send_buffer));
                memset(recv_buffer,0,sizeof(recv_buffer));
                printf("Enter a file you would like to download(or exit)> ");
                scanf("%s", send_buffer);
                if(strcmp(send_buffer, "exit") == 0){
                    break;
                }
                send(connection, send_buffer, strlen(send_buffer), 0);
                printf("\nSENT THE FILENAME\n");
                unsigned long total_bytes_recv = 0;
                bytes_received=recv(connection, recv_buffer, sizeof(recv_buffer),0);
                total_bytes_recv += bytes_received;
                printf("\nRECEIVED THE FILE DATA\n");
                if(bytes_received > 0){
                    recv_buffer[bytes_received] = '\0';
                    if(strcmp(recv_buffer,"File does not exist.") == 0){
                        printf("\nFile Does not exist.\n");
                    }else{
                        char download_path[128];
                        snprintf(download_path,sizeof(download_path),"download/%s",send_buffer);
                        FILE* file = fopen(download_path,"wb");
                        if(file){
                            fwrite(recv_buffer,1,bytes_received,file);
                            //check and see if the total amount of data is captured
                            size_t filesize = get_filesize(send_buffer);
                            if(filesize != total_bytes_recv){
                                do {
                                    //Attempt to get another recv from the server
                                    memset(recv_buffer,0,sizeof(recv_buffer));
                                    bytes_received=recv(connection, recv_buffer, sizeof(recv_buffer),0);
                                    //Add bytes_received to total_bytes_recv again
                                    total_bytes_recv+=bytes_received;
                                    //Write the data to the file once again
                                    fwrite(recv_buffer,1,bytes_received,file);
                                    //download progress read out
                                    if(total_bytes_recv % 10000 == 0){ //doesn't display percentage unless files are certain sizes
                                        double percentage_done = (double)total_bytes_recv/filesize * 100;
                                        printf("\n(%f/100.000000)\n",percentage_done);
                                    }
                                }while(filesize != total_bytes_recv);
                            }
                            
                            fclose(file);
                            printf("\nFile '%s' downloaded successfully.\n",send_buffer);
                        }else{
                            printf("\nFile error creating '%s'",send_buffer);
                        }
                    }
                }

            } while (strcmp(send_buffer, "exit") != 0);
            // Cleanup Winsock
            closesocket(connection);
            memset(port,0,sizeof(port));
            memset(host,0,sizeof(host));
        } else if (strcmp(command, "uplist") == 0) { // Lists all the files that the user is wanting to transfer(folder upload in current directory)
            list_directory();
            print_hashmap();
        }
    }
    // Cleanup Winsock
    WSACleanup();

    return 0;
}
//function to send file list
void send_file_list(SOCKET socket) {
    // List the directory and send the file list to the client
    // Function to list files in the "upload" directory
    WIN32_FIND_DATA find_file_data;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    DWORD dwError; // just meant to check the last error message sent

    // Path to the "upload" directory
    const char* directory_path = "upload\\*";

    // Find the first file in the directory
    hFind = FindFirstFile(directory_path, &find_file_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("Error listing directory: %d\n", GetLastError());
        return;
    }

    // Prepare buffer for file list
    char file_list[1024] = "";
    
    // Concatenate file names with their sizes into the buffer
    do {
        if (strcmp(find_file_data.cFileName, ".") != 0 && strcmp(find_file_data.cFileName, "..") != 0) {
            char file_info[256];
            sprintf(file_info, "%s (%lu bytes)\n", find_file_data.cFileName, find_file_data.nFileSizeLow);
            strcat(file_list, file_info);
        }
    } while (FindNextFile(hFind, &find_file_data) != 0);

    dwError = GetLastError();
    if (dwError != ERROR_NO_MORE_FILES) {
        printf("Error listing directory: %d\n", dwError);
    }

    // Close the handle(pointer index)
    FindClose(hFind);

    // Send the file list to the client
    send(socket, file_list, strlen(file_list), 0);
}
//server side function that runs while program is functioning
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
//lists the upload directory of the host
void list_directory() {
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
//function that creates the hash table
void create_file_list(char* file_list) {
    // List the directory and save it to the file_list
    // Function to list files in the "upload" directory
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
    
    // Concatenate file names with their sizes into the buffer
    do {
        if (strcmp(find_file_data.cFileName, ".") != 0 && strcmp(find_file_data.cFileName, "..") != 0) {
            char file_info[256];
            sprintf(file_info, "%s (%lu bytes)\n", find_file_data.cFileName, find_file_data.nFileSizeLow);
            strcat(file_list, file_info);
        }
    } while (FindNextFile(hFind, &find_file_data) != 0);

    dwError = GetLastError();
    if (dwError != ERROR_NO_MORE_FILES) {
        printf("Error listing directory: %d\n", dwError);
    }

    // Close the handle
    FindClose(hFind);
}
// Hash function to calculate index based on filename
size_t hash_function(const char* filename) {
    size_t hash = 0;
    for (size_t i = 0; filename[i] != '\0'; i++) {
        hash = 31 * hash + filename[i];
    }
    return hash % HASH_SIZE;
}
// Function to insert a new node into the hashmap
void insert(const char* filename, size_t filesize) {
    size_t index = hash_function(filename);

    // Create a new node
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }
    strcpy(newNode->filename, filename);
    newNode->filesize = filesize;
    newNode->next = NULL;

    // Insert the new node at the beginning of the linked list at index
    newNode->next = hashmap[index];
    hashmap[index] = newNode;
}
// Function to retrieve filesize given filename
size_t get_filesize(const char* filename) {
    size_t index = hash_function(filename);
    Node* current = hashmap[index];
    while (current != NULL) {
        if (strcmp(current->filename, filename) == 0) {
            return current->filesize;
        }
        current = current->next;
    }
    return 0; // File not found
}
//prints the current hashtable
void print_hashmap() {
    printf("Hashmap contents:\n");
    for (int i = 0; i < HASH_SIZE; i++) {
        Node* current = hashmap[i];
        while (current != NULL) {
            printf("Filename: %s, Filesize: %lu\n", current->filename, current->filesize);
            current = current->next;
        }
    }
}


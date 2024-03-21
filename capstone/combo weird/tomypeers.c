#include "unistd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h> // Windows Sockets header
#include <ws2tcpip.h> // TCP/IP functions
#include "pthread.h"
#include <fcntl.h>
#include<Windows.h>

#define MAX_PENDING 5
#define MAX_FILENAME_LEN 255
#define MAX_LINE 256

struct peer_entry {
    UINT32 id;
    SOCKET socket_descriptor; // Change socket descriptor type
    char files[MAX_PENDING][MAX_FILENAME_LEN];
    struct sockaddr_in address;
    int numFiles;
};

void* run_registry(void* arg);
void* run_peer(void* arg);
SOCKET bind_and_listen(const char* service);
SOCKET lookup_and_connect(const char* host, const char* service);
int find_max_fd(fd_set* fs);

int main(int argc, char* argv[]) {
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    if (argc != 2) {
        printf("ERROR: Incorrect number of arguments.\n");
        printf("USAGE: %s <port number>\n", argv[0]);
        return 1;
    }

    char role[10];
    printf("Enter 'registry' to start as registry or 'peer' to start as peer: ");
    scanf("%s", role);

    if (strcmp(role, "registry") == 0) {
        pthread_t registry_thread;
        pthread_create(&registry_thread, NULL, run_registry, argv[1]);
        pthread_join(registry_thread, NULL);
    } else if (strcmp(role, "peer") == 0) {
        pthread_t peer_thread;
        pthread_create(&peer_thread, NULL, run_peer, argv[1]);
        pthread_join(peer_thread, NULL);
    } else {
        printf("Invalid role entered. Please enter 'registry' or 'peer'.\n");
        return 1;
    }

    // Cleanup Winsock
    WSACleanup();

    return 0;
}

void* run_registry(void* arg) {
    const char* port = (const char*)arg;
    char buffer[MAX_LINE];
    struct peer_entry peer[MAX_PENDING];
    int numOfSocket = 0;
    socklen_t addrlen;
    UINT8 action;

    fd_set all_sockets, call_set;
    FD_ZERO(&all_sockets);

    SOCKET listen_socket = bind_and_listen(port);
    FD_SET(listen_socket, &all_sockets);
    int max_socket = listen_socket;

    while (1) {
        call_set = all_sockets;
        int num_s = select(max_socket + 1, &call_set, NULL, NULL, NULL);
        if (num_s < 0) {
            perror("ERROR in select() call");
            return NULL;
        }

        for (SOCKET s = 3; s <= max_socket; ++s) {
            if (!FD_ISSET(s, &call_set))
                continue;

            if (s == listen_socket) {
                peer[numOfSocket].socket_descriptor = accept(s, ((struct sockaddr*)&peer[numOfSocket].address), &addrlen);
                SOCKET newFD = peer[numOfSocket].socket_descriptor;
                if (newFD == INVALID_SOCKET) {
                    perror("accept");
                } else {
                    FD_SET(newFD, &all_sockets);
                    if (newFD > max_socket) {
                        max_socket = newFD;
                    }
                    printf("Socket %d connected\n", newFD);
                    numOfSocket++;
                }
            } else {
                int numbytes;
                if ((numbytes = recv(s, buffer, sizeof(buffer), 0)) <= 0) {
                    if (numbytes == 0) {
                        printf("socket %d closed\n", s);
                    } else {
                        perror("recv");
                    }
                    closesocket(s);
                    numOfSocket--;
                    FD_CLR(s, &all_sockets);
                } else {
                    // Handle received data
                    // Code to process commands from peers
                    // This part needs to be implemented based on your requirements
                }
            }
        }
    }

    return NULL;
}

void* run_peer(void* arg) {
    const char* host = "localhost"; // Assuming peer runs on the same machine
    const char* port = (const char*)arg;

    // Connect to the registry
    SOCKET registry_socket = lookup_and_connect(host, port);
    if (registry_socket == INVALID_SOCKET) {
        perror("Unable to connect to registry");
        return NULL;
    }

    // Code for peer functionality
    // This part needs to be implemented based on your requirements

    return NULL;
}

SOCKET bind_and_listen(const char* service) {
    struct addrinfo hints;
    struct addrinfo* rp, * result;
    SOCKET s = INVALID_SOCKET;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int ret = getaddrinfo(NULL, service, &hints, &result);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        return INVALID_SOCKET;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (s == INVALID_SOCKET) {
            continue;
        }

        if (bind(s, rp->ai_addr, (int)rp->ai_addrlen) == SOCKET_ERROR) {
            closesocket(s);
            s = INVALID_SOCKET;
            continue;
        }

        break;
    }

    if (s == INVALID_SOCKET) {
        fprintf(stderr, "Unable to bind socket\n");
        freeaddrinfo(result);
        return INVALID_SOCKET;
    }

    freeaddrinfo(result);

    if (listen(s, MAX_PENDING) == SOCKET_ERROR) {
        perror("listen");
        closesocket(s);
        return INVALID_SOCKET;
    }

    return s;
}

SOCKET lookup_and_connect(const char* host, const char* service) {
    struct addrinfo hints;
    struct addrinfo* result, * rp;
    SOCKET s = INVALID_SOCKET;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int ret = getaddrinfo(host, service, &hints, &result);
    if (ret != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        return INVALID_SOCKET;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (s == INVALID_SOCKET) {
            continue;
        }

        if (connect(s, rp->ai_addr, (int)rp->ai_addrlen) == SOCKET_ERROR) {
            closesocket(s);
            s = INVALID_SOCKET;
            continue;
        }

        break;
    }

    freeaddrinfo(result);

    if (s == INVALID_SOCKET) {
        fprintf(stderr, "Unable to connect\n");
        return INVALID_SOCKET;
    }

    return s;
}

int find_max_fd(fd_set* fs) {
    int ret = 0;
    for (SOCKET i = MAX_PENDING; i >= 0 && ret == 0; --i) {
        if (FD_ISSET(i, fs)) {
            ret = i;
        }
    }
    return ret;
}

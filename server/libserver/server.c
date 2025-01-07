#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <asm-generic/socket.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include "server.h"

addressList* globalListPtr;
int* serverSocketPtr;

addressList* create_address_list(size_t initialCapacity) {
    addressList* list = (addressList*)malloc(sizeof(addressList));
    if (list == NULL) {
        perror("malloc list failed");
        return NULL;
    }

    (*list).addresses = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in) * initialCapacity);
    if ((*list).addresses == NULL) {
        perror("malloc addresses failed");
        free(list);
        return NULL;
    }

    (*list).count = 0;
    (*list).capacity = initialCapacity;

    return list; 
}

short int add_address(addressList* list, const struct sockaddr_in* address) {
    if (list == NULL || address == NULL) return -1;

    if ((*list).count == (*list).capacity) {
        size_t newCapacity = (*list).capacity * (round(2/(*list).capacity)*10) +1;
        struct sockaddr_in* newAddresses = (struct sockaddr_in*)realloc((*list).addresses, sizeof(struct sockaddr_in) * newCapacity);

        if (newAddresses == NULL) {
            perror("realloc failed");
            return 0;
        }

        (*list).addresses = newAddresses;
        (*list).capacity = newCapacity;
    } 

    memcpy(&(*list).addresses[(*list).count], address, sizeof(struct sockaddr_in));
    (*list).count++;
    return 1;
}

int find_address(addressList* list, const struct sockaddr_in* address) {
    if (list == NULL || address == NULL) return -2;

    for (size_t i = 0; i < (*list).count; i++) {
        if (memcmp(&(*list).addresses[i], address, sizeof(struct sockaddr_in)) == 0) {
            return i;
        }
    }

    return -1;
}

short int remove_address(addressList* list, const struct sockaddr_in* address) {
    if (list == NULL || address == NULL) return -1;

    int index = find_address(list, address);
    if (index < 0) {
        return 0;
    }

    if (index < (*list).count -1) {
        memmove(&(*list).addresses[index], &(*list).addresses[index -1], sizeof(struct sockaddr_in) * ((*list).count - index - 1));
    }
    (*list).count--;
    return 1;
}

void free_address_list(addressList* list) {
    if (list == NULL) return;

    free((*list).addresses);
    free(list);
}

void shutDownServerHandler(int)
{
    free_address_list(globalListPtr);

    close(*serverSocketPtr);
    exit(EXIT_SUCCESS);
}

void handleClient(int* serverSocket, addressList* list, char* buffer) {
    for (size_t i = 0; i < (*list).count; i++)
    {
        struct sockaddr_in* address = &(*list).addresses[i];
        int f = sendto(*serverSocket, (const char*) buffer, sizeof(buffer), MSG_CONFIRM, (struct sockaddr*) address, sizeof(*address));

        if (f < 0) {
            perror("failed sendto, retrying");
            
            f = sendto(*serverSocket, (const char*) buffer, sizeof(buffer), MSG_CONFIRM, (struct sockaddr*) address, sizeof(*address));
            if (f < 0) {
                perror("second failed sendto");
                remove_address(list, address);
            }
        }
    }

    memset(buffer, 0, sizeof(buffer));
}

void serveServer(addressList* list, int *serverSocket)
{
    globalListPtr = list;

    *serverSocket = socket(AF_INET, SOCK_DGRAM, 0);

    int socketOption = 1;

    if (setsockopt(*serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &socketOption, sizeof(socketOption))) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(*serverSocket, (struct sockaddr*) &serverAddr, sizeof(serverAddr))) {
        perror("bind socket failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in clientAddr;

    int clientSock, n;
    char buffer[BUFFER];

    int len = sizeof(clientAddr);

    while (1)
    {
        n = recvfrom(*serverSocket, (char*) buffer, BUFFER -1, MSG_WAITALL, (struct sockaddr*) &clientAddr, &len);

        if (n < 0) {
            perror("recvfrom failed");
            continue;
        }

        if (find_address(list, &clientAddr) < 0) {
            add_address(list, &clientAddr);
        }

        handleClient(serverSocket, list, buffer);
    }

    shutdown(*serverSocket, SHUT_RDWR);
    close(*serverSocket);
}
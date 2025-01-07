#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <asm-generic/socket.h>
#include <signal.h>
#include <pthread.h>

#define PORT 3030
#define BUFFER 1024

struct threadArgs {
    int clientSocket;
    struct sockaddr_in serverAddr;
};

void *receiveMessages(void* arg) {
    struct threadArgs* args = arg;

    char buffer[BUFFER];
    int n;
    socklen_t len = sizeof(args->serverAddr);

    while (1) {
        memset(buffer, 0, BUFFER);

        n = recvfrom(args->clientSocket, (char*) buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr*) &args->serverAddr, &len);

        if (n < 0) {
            perror("recvfrom failed");
            break;
        }
        buffer[n] = '\0';

        printf("Server: %s\n\r", buffer);
    }

    return NULL;
}

void *sendMessages(void* arg) {
    struct threadArgs* args = arg;

    char buffer[BUFFER];

    while(1) {
        memset(buffer, 0, sizeof(buffer));
        if (fgets(buffer, BUFFER, stdin) == NULL) {
            if (ferror(stdin)) {
                perror("fgets failed");
            }
            break;
        }

        ssize_t bytes = sendto(args->clientSocket, buffer, sizeof(buffer), MSG_CONFIRM, (struct sockaddr*) &args->serverAddr, sizeof(args->serverAddr));
        if (bytes < 0) {
            perror("sendto failed");
            break;
        }
    }

    return NULL;
}

int createSocket(void) {
    int sockFd;
    if ((sockFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    return sockFd;
}

struct sockaddr_in getServerAddr(void) {
    struct sockaddr_in serverAddr;

    memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    return serverAddr;
}

int main(void) {
    int socket = createSocket();
    struct sockaddr_in server = getServerAddr();

    const char* message = "connect";

    ssize_t bytes = sendto(socket, (const char*) message, strlen(message), MSG_CONFIRM, (const struct sockaddr*) &server, sizeof(server));
    if (bytes < 0) {
        perror("sendto failed");
        exit(EXIT_FAILURE);
    }

    printf("Server connect\n\r");

    struct threadArgs args;
    args.clientSocket = socket;
    args.serverAddr = server;

    pthread_t receiveThread, sendThread;

    if (pthread_create(&receiveThread, NULL, receiveMessages, (void*) &args) != 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }

    if (pthread_create(&sendThread, NULL, sendMessages, (void*) &args) != 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
    }

    pthread_join(receiveThread, NULL);
    pthread_join(sendThread, NULL);

    printf("Client closed \n\r");
    close(socket);
    
    return 0;
}
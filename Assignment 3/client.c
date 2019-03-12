#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "base64.c"

#define BUFFER_SIZE 2049

static int connect_socket(int server_port, char *server_ip) {
    struct sockaddr_in server_addr;
    int server_socket;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        close(server_socket);
        return -1;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (!inet_pton(AF_INET, server_ip, &(server_addr.sin_addr))) {
        perror("inet_pton");
        close(server_socket);
        return -1;
    }

    if (connect(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(server_socket);
        return -1;
    }
    return server_socket; 
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "<executable code><Server IP Address><Server Port number>\n");
        exit(EXIT_FAILURE);
    }
    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    int server_socket = connect_socket(server_port, server_ip);
    if (server_socket == -1) {
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    char encoded_msg[BUFFER_SIZE];
    while (1) {
        printf("Enter message to be sent:\n");
        scanf ("%[^\n]%*c", buffer);
        printf("%s", buffer);
        // fgets(buffer, BUFFER_SIZE, stdin);
        encode(buffer, encoded_msg);
        printf("%s", encoded_msg);
        // int retval = send(server_socket, buffer, strlen(buffer), 0);
        int retval = send(server_socket, encoded_msg, strlen(encoded_msg), 0);
        if (retval == -1) {
            perror("send()");
        }
        if (buffer[0] == '1') {
            int bytes_read = read(server_socket, buffer, BUFFER_SIZE);
            buffer[bytes_read] = '\0';
            printf("%s\n", buffer);
        }
        else if (buffer[0] == '3') {
            close(server_socket);
            exit(EXIT_SUCCESS);
        }
        else perror("Invalid message type\n");
    }
}
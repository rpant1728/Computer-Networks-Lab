#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "base64.c"

#define MSG_LEN 255
#define BUFFER_SIZE 2049

/* Establish a TCP connection with given server ip and port and return socket descriptor */
static int connect_socket(int server_port, char *server_ip) {
    struct sockaddr_in server_addr;
    int client_socket;

    /* Create a TCP socket and save its file descriptor in client_socket */
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        close(client_socket);
        return -1;
    }
    /* Initialise a struct with server details */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    /* Convert server_ip string to internal bit format used by server_addr */
    if (!inet_pton(AF_INET, server_ip, &(server_addr.sin_addr))) {
        perror("inet_pton");
        close(client_socket);
        return -1;
    }

    /* Open a connection on socket fd client_socket to peer at server_addr */
    if (connect(client_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_socket);
        return -1;
    }
    return client_socket; 
}

int main(int argc, char* argv[]) {

    /* Input server ip and port from commandline arguments */
    if (argc != 3) {
        fprintf(stderr, "<executable code><Server IP Address><Server Port number>\n");
        exit(EXIT_FAILURE);
    }
    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);

    /* Open TCP connection with server */
    int client_socket = connect_socket(server_port, server_ip);
    if (client_socket == -1) {
        exit(EXIT_FAILURE);
    }

    /* Buffer stores input messages and output messages */
    char buffer[BUFFER_SIZE];
    /* encode (input) buffer and store in encoded_msg and then send */
    char encoded_msg[BUFFER_SIZE];
    int bytes_read;

    /* Loop till a type 3 message is sent */
    while (1) {
        printf("Enter message to be sent:\n");
        scanf ("%[^\n]%*c", buffer); /* scan till newline */

        /* check validity of message to be sent */
        if (strlen(buffer) > MSG_LEN) {
            fprintf(stderr, "Message Discarded, size limit exceeded\n");
            continue;
        }
        if (buffer[0] != '1' && buffer[0] != '3') {
            fprintf(stderr, "Message Discarded: (Usage: <type> <message>)\n");
            continue;
        }

        /* Message validated, now encode and send */
        encode(buffer, encoded_msg); /* call base64 encode from self-defined library, then send */
        int retval = send(client_socket, encoded_msg, strlen(encoded_msg), 0);
        if (retval == -1) {
            perror("send()");
        }
        if (buffer[0] == '1') { /* Type 1 message wait for ACK from server */
            bytes_read = read(client_socket, buffer, BUFFER_SIZE);
            buffer[bytes_read] = '\0';
            printf("%s\n", buffer);
        }
        else if (buffer[0] == '3') { /* Type 3 message, close fd after receiving ACK from server */
            bytes_read = read(client_socket, buffer, BUFFER_SIZE);
            buffer[bytes_read] = '\0';
            printf("%s\n", buffer);
            close(client_socket);
            exit(EXIT_SUCCESS);
        }
        else fprintf(stderr, "Message discarded (Usage: <type> <message>)\n");
    }
}
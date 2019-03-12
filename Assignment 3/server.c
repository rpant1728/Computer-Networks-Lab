#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "base64.c"

#undef MAX
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MAX_CLIENTS 64
#define BUFFER_SIZE 2049

static int set_welcome_socket(int PORT) {
    struct sockaddr_in addr;
    int welcome_socket, yes = 1;

    if ((welcome_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }
    if (setsockopt(welcome_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("socket");
        close(welcome_socket);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    // addr.sin_addr.s_addr = INADDR_ANY;    
    if (bind(welcome_socket, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind");
        close(welcome_socket);
        return -1;
    }
    printf("Listening on port %d\n", PORT);
    if (listen(welcome_socket, 5) == -1) {
        perror("listen");
        close(welcome_socket);
        return -1;
    }
    return welcome_socket;
}



int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Server: <executable code><Server Port number>\n");
        exit(EXIT_FAILURE);
    }
    int PORT = atoi(argv[1]);
    // printf("%d", PORT);
    int active_sockets[MAX_CLIENTS];
    int ndfs = 0, i = 0, trigger;
    for (i = 0; i < MAX_CLIENTS; i++) active_sockets[i] = 0;
    char buffer[BUFFER_SIZE], decoded_msg[BUFFER_SIZE];
    fd_set readfds;


    int welcome_socket = set_welcome_socket(PORT);
    if (welcome_socket == -1) {
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client_addr;
    unsigned int len = sizeof(client_addr);
    char *ack = "2 ACK\0";

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(welcome_socket, &readfds);
        ndfs = welcome_socket;

        for (i = 0; i < MAX_CLIENTS; i++) {
            if (active_sockets[i]) {
                FD_SET(active_sockets[i], &readfds);
                ndfs = MAX(ndfs, active_sockets);
            }
        }

        trigger = select(ndfs+1, &readfds, NULL, NULL, NULL);
        // if (trigger == -1 && errno != EINTR) {
        //     perror("select()");
        //     exit(EXIT_FAILURE);
        // }
        if (trigger == -1 && errno == EINTR)
            continue;

        if (trigger == -1) {
            perror("select()");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(welcome_socket, &readfds)) {
            memset(&client_addr, 0, len);
            
            int new_socket = accept(welcome_socket, (struct sockaddr *) &client_addr, &len);
            if (new_socket == -1) {
                perror("accept()");
            }  
            else {
                printf("New client: socket_fd %d, ip %s, port %d\n",
                        new_socket, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));                     
                        
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (!active_sockets[i]) {
                        active_sockets[i] = new_socket;
                        break;
                    }
                }
            }
        }
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (!active_sockets[i]) continue;

            int s = active_sockets[i];
            if (FD_ISSET(s, &readfds)) {
                int bytes_read = read(s, buffer, BUFFER_SIZE);
                printf("%s\n", buffer);
                int decoded_len;
                decode(buffer, decoded_msg, &decoded_len);
                printf("%d\n", decoded_len);
                // int decoded_len = strlen(decoded_msg);
                if (decoded_len == 0) continue;
                else {
                    if (decoded_msg[0] == '1') {
                        printf("%s\n", decoded_msg);
                        if (send(s, ack, strlen(ack), 0) == -1) {
                            perror("ack");
                        }
                    }
                    else if (decoded_msg[0] == '3') {
                        if (send(s, ack, strlen(ack), 0) == -1) {
                            perror("ack");
                        }
                        getpeername(s, (struct sockaddr *) &client_addr, &len);
                        printf("Disconnect: socket_fd %d, ip %s, port %d\n",
                                s, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port)); 
                        close(s);
                        active_sockets[i] = 0;
                    }
                    else perror("Invalid message type\n");
                } 
            } 
        }
    }
}
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "base64.c"

#define BUFFER_SIZE 2049
pthread_mutex_t mutexstdin;

void *client_handler(void *new_socket) {
    int s = (int) new_socket;

    struct sockaddr_in client_addr;
    unsigned int len = sizeof(client_addr);
    getpeername(s, (struct sockaddr *) &client_addr, &len);
    char ip[16];
    inet_ntop(AF_INET, &(client_addr.sin_addr), ip, INET_ADDRSTRLEN);

    char buffer[BUFFER_SIZE], decoded_msg[BUFFER_SIZE];
    int bytes_read, decoded_len;
    char *ack = "2 ACK\0";

    printf("New client: socket_fd %d, ip %s, port %d\n",
        s, ip, ntohs(client_addr.sin_port)); 

    while (1) {
        bytes_read = read(s, buffer, BUFFER_SIZE);
        buffer[bytes_read] = '\0';
        decode(buffer, decoded_msg);
        decoded_len = strlen(decoded_msg);
        
        if (decoded_len == 0) {
            break;
        }
        else {
            if (decoded_msg[0] == '1') {
                pthread_mutex_lock (&mutexstdin);
                printf("Message from socket_fd %d, ip %s, port %d:\n%s\n%s\n", s, ip, ntohs(client_addr.sin_port), buffer, decoded_msg);
                pthread_mutex_unlock (&mutexstdin);
                if (send(s, ack, strlen(ack), 0) == -1) {
                    perror("ack");
                }
                // if (send(s, decoded_msg, strlen(decoded_msg), 0) == -1) {
                //     perror("ack");
                // }
            }
            else if (decoded_msg[0] == '3') {
                if (send(s, ack, strlen(ack), 0) == -1) {
                    perror("ack");
                }
                printf("Disconnect: socket_fd %d, ip %s, port %d\n",
                        s, ip, ntohs(client_addr.sin_port)); 
                break;
            }
            else fprintf(stderr, "Message Discarded (Usage: <type> <message>)/n");
        }
    }
    close(s);
    pthread_exit(NULL);
}

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
    int welcome_socket, new_socket;

    welcome_socket = set_welcome_socket(PORT);
    if (welcome_socket == -1) {
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client_addr;
    unsigned int len = sizeof(client_addr);
    pthread_t thread_id;
    pthread_mutex_init(&mutexstdin, NULL);
    
    while (1) {
        memset(&client_addr, 0, len);
        
        new_socket = accept(welcome_socket, (struct sockaddr *) &client_addr, &len);
        if (new_socket == -1) {
            perror("accept()");
        }  
        else {
            if (pthread_create(&thread_id, NULL, client_handler, (void *) new_socket) < 0) {
                perror("thread not created");                
            }                    
        }
    }
    pthread_mutex_destroy(&mutexstdin);
    pthread_exit(NULL);
} 
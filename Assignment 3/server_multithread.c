// including libraries 
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

// Function each child thread uses to server its own client
void *client_handler(void *new_socket) {
    int s = (int) new_socket; // get client's socket

    // Define and populate a struct with client ip, port
    struct sockaddr_in client_addr;
    unsigned int len = sizeof(client_addr);
    getpeername(s, (struct sockaddr *) &client_addr, &len);
    char client_ip[16]; // To store human readable client ip 
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

    /* Buffer stores input messages from client */
    /* decode (input) buffer and store in decoded_msg */
    char buffer[BUFFER_SIZE], decoded_msg[BUFFER_SIZE];
    int bytes_read, decoded_len;
    char *ack = "2 ACK\0";

    printf("New client: socket_fd %d, ip %s, port %d\n",
        s, client_ip, ntohs(client_addr.sin_port)); 

    // loop indefinitely, read encoded msg, decode it and continue accordingly
    while (1) {
        bytes_read = read(s, buffer, BUFFER_SIZE); // read encoded msg
        buffer[bytes_read] = '\0';
        decode(buffer, decoded_msg);    // decode it
        decoded_len = strlen(decoded_msg);
        
        if (decoded_len == 0) {
            break;
        }
        else {
            if (decoded_msg[0] == '1') {  /* Type 1 message, display then send ACK */
                pthread_mutex_lock (&mutexstdin);
                printf("Message from socket_fd %d, ip %s, port %d:\n%s\n%s\n", s, client_ip, ntohs(client_addr.sin_port), buffer, decoded_msg);
                pthread_mutex_unlock (&mutexstdin);
                if (send(s, ack, strlen(ack), 0) == -1) {
                    perror("ack");
                }
            }
            else if (decoded_msg[0] == '3') {   /* Type 3 message, send ACK then break loop */
                if (send(s, ack, strlen(ack), 0) == -1) {
                    perror("ack");
                }
                printf("Disconnect: socket_fd %d, ip %s, port %d\n",
                        s, client_ip, ntohs(client_addr.sin_port)); 
                break;
            }
            else fprintf(stderr, "Unknown Message Discarded (Usage: <type> <message>)/n");
        }
    }
    // loop exited, close client_socket and this thread
    close(s);
    pthread_exit(NULL);
}

/* Setup welcome socket on given port of localhost and listen indefinitely */ 
static int set_welcome_socket(int PORT) {
    struct sockaddr_in addr;
    int welcome_socket, yes = 1;

    /* Create a TCP socket and save its file descriptor in welcome_socket */
    if ((welcome_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }
    /* Reuse sockets */
    if (setsockopt(welcome_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("socket");
        close(welcome_socket);
        return -1;
    }

    // Populate struct addr with given port  
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    // addr.sin_addr.s_addr = INADDR_ANY;   

    //     
    if (bind(welcome_socket, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind");
        close(welcome_socket);
        return -1;
    }

    printf("Listening on port %d\n", PORT);
    // Max 5 pending client connect requests before server refuses incoming requests
    if (listen(welcome_socket, 5) == -1) {  
        perror("listen");
        close(welcome_socket);
        return -1;
    }
    return welcome_socket;
}

int main(int argc, char* argv[]) {
    /* Input server port as commandline argument */
    if (argc != 2) {
        fprintf(stderr, "Server: <executable code><Server Port number>\n");
        exit(EXIT_FAILURE);
    }
    int PORT = atoi(argv[1]);

    /* Setup welcome setup using set_welcome_socket() function*/
    int welcome_socket;
    welcome_socket = set_welcome_socket(PORT);
    if (welcome_socket == -1) {
        exit(EXIT_FAILURE);
    }

    // data structures for processing each incoming connection request
    int new_socket;
    struct sockaddr_in client_addr;
    unsigned int len = sizeof(client_addr);
    pthread_t thread_id;
    pthread_mutex_init(&mutexstdin, NULL); // lock to synchronise sharing of stdout
    
    // loop indefinitely, spawning a new child thread each of which engages one client only
    while (1) {
        memset(&client_addr, 0, len);
        // get new_socket by accepting incoming request
        new_socket = accept(welcome_socket, (struct sockaddr *) &client_addr, &len);
        if (new_socket == -1) {
            perror("accept()");
        }  
        else {
            // child thread is passed its start function - client_handler
            if (pthread_create(&thread_id, NULL, client_handler, (void *) new_socket) < 0) {
                perror("thread not created");                
            }                    
        }
    }
    pthread_mutex_destroy(&mutexstdin);
    pthread_exit(NULL);
} 
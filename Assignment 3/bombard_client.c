#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h>  // for open
#include <unistd.h> // for close
#include <pthread.h>
#include "base64.c"

#define BUFFER_SIZE 2049

void *clientThread(void *arg)
{
    printf("In thread\n");
    struct sockaddr_in server_addr;
    int server_socket;
    int server_port = 8080;
    char *server_ip = "127.0.0.1";

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        close(server_socket);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (!inet_pton(AF_INET, server_ip, &(server_addr.sin_addr)))
    {
        perror("inet_pton");
        close(server_socket);
    }

    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(server_socket);
    }
    char buffer[BUFFER_SIZE], encoded_msg[BUFFER_SIZE];
    int bytes_read;
    sprintf(buffer, "1 client%ld", pthread_self());
    encode(buffer, encoded_msg);
    int retval = send(server_socket, encoded_msg, strlen(encoded_msg), 0);
    if (retval == -1)
    {
        perror("send()");
    }
    if (buffer[0] == '1')
    {
        bytes_read = read(server_socket, buffer, BUFFER_SIZE);
        buffer[bytes_read] = '\0';
        printf("%s\n", buffer);
    }
    else if (buffer[0] == '3')
    {
        bytes_read = read(server_socket, buffer, BUFFER_SIZE);
        buffer[bytes_read] = '\0';
        printf("%s\n", buffer);
        close(server_socket);
        exit(EXIT_SUCCESS);
    }
    else
        fprintf(stderr, "Message Discarded (Usage: <type> <message>)\n");

    sleep(pthread_self() % 5);
    sprintf(buffer, "3 exit%ld", pthread_self());
    encode(buffer, encoded_msg);
    retval = send(server_socket, encoded_msg, strlen(encoded_msg), 0);
    if (retval == -1)
    {
        perror("send()");
    }
    if (buffer[0] == '1')
    {
        bytes_read = read(server_socket, buffer, BUFFER_SIZE);
        buffer[bytes_read] = '\0';
        printf("%s\n", buffer);
    }
    else if (buffer[0] == '3')
    {
        bytes_read = read(server_socket, buffer, BUFFER_SIZE);
        buffer[bytes_read] = '\0';
        printf("%s\n", buffer);
        close(server_socket);
        pthread_exit(NULL);
        // exit(EXIT_SUCCESS);
    }
    else
        fprintf(stderr, "Message Discarded (Usage: <type> <message>)\n");
    close(server_socket);
    pthread_exit(NULL);
}
int main()
{
    int i = 0;
    pthread_t tid[51];
    while (i < 5)
    {
        if (pthread_create(&tid[i], NULL, clientThread, NULL) != 0)
            printf("Failed to create thread\n");
        i++;
    }
    sleep(100);
    return 0;
}
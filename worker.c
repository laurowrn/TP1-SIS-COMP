/* 
    This programa open a TCP conection with a server and wait for requistions.
    
    When a requistion is received, the worker process it and send the result.
*/

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/tcp.h>

#define BUFFER_SIZE 1024
#define PORT 5000

int receive_message(int socket_id, char* buffer){
    int i = 0, n;
    while((n = recv(socket_id, &buffer[i], 1, 0)) > 0){
        if(buffer[i] == '\0'){
            break;
        }
        i++;
    }
    if(n < 0){
        return n;
    }
    return i;
}

double perform_operation(const char *operation, double a, double b) {
    if (strcmp(operation, "add") == 0) {
        return a + b;
    } else if (strcmp(operation, "subtract") == 0) {
        return a - b;
    } else if (strcmp(operation, "multiply") == 0) {
        return a * b;
    } else if (strcmp(operation, "divide") == 0) {
        return a / b;
    } else {
        return 0.0;
    }
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    const char worker_hello[] = "worker";

    if (argc < 2){
        printf("Usage: %s <ip address>\n", argv[0]);
        exit(0);
    }

    /* Create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, TCP_NODELAY && !TCP_CORK);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    /* Configure server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    /* Connect to the server */
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }

    /* Send message identifying itself as a worker. */
    if (send(sockfd, worker_hello, strlen(worker_hello) + 1, 0) < 0) {
        perror("Error sending worker hello");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    while (1) {

        /* Receive request */
        memset(buffer, 0, BUFFER_SIZE);
        if (recv(sockfd, buffer, sizeof(buffer) + 1, 0) < 0) {
            perror("Error receiving request");
            exit(EXIT_FAILURE);
        }

        /* If the server sends "quit", close the connection and exit */
        if (strcmp(buffer, "quit") == 0) {
            printf("Worker quitting...\n");
            break;
        }

        /* Parse the request and perform the operation */
        char operation[32];
        double a, b;
        sscanf(buffer, "%s %lf %lf", operation, &a, &b);
        printf("Worker received request: %s %lf %lf\n", operation, a, b);
        fflush(stdout);
        double result = perform_operation(operation, a, b);

        /* Send the result back to the server */
        snprintf(buffer, BUFFER_SIZE, "%.2lf", result);
        if (send(sockfd, buffer, strlen(buffer) + 1, 0) < 0) {
            perror("Error sending result");
            exit(EXIT_FAILURE);
        }

    }
    /* Close the socket */
    close(sockfd);

    return 0;
}
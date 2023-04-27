#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

#define PORT 5000
#define BACKLOG 50
#define MAX_WORKERS 50

enum worker_state {IDLE, BUSY};

    //Struct para identificar socket - idependente se é worker ou server
struct socket_data{
    int socket_id;
    struct sockaddr_in *socket_address;
};

struct worker{
    struct socket_data data;
    enum worker_state state;
};

//Variáveis dos workers
struct worker workers[MAX_WORKERS];
int workers_count = 0;
pthread_mutex_t workers_change;
sem_t idle_workers;
void exit_handler(int);

void exit_handler(int sig) {
    char exit_message[1024];
    memset(exit_message, 0, sizeof(exit_message));
    snprintf(exit_message, sizeof(exit_message), "%s", "quit");

    for(int i = 0; i < workers_count; i++){
        send(workers[i].data.socket_id, exit_message, sizeof(exit_message) + 1, 0);
    }
    sleep(2);
    printf("\nSever desligando!\n");
    fflush(stdout);
    exit(1);
}

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

    //Função que determina o comportamento das conexões com o servidor
void *route_sockets(void *received_socket){
    char socket_type[1024];
    char send_buffer[1024];
    memset(socket_type, 0, sizeof(socket_type));

    //Cria um ponteiro para struck socket_data
    struct socket_data *connected_socket = (struct socket_data *)received_socket;
    printf("Received connection from %s:%d\n", inet_ntoa(connected_socket->socket_address->sin_addr), ntohs(connected_socket->socket_address->sin_port));
    fflush(stdout);

    if (receive_message(connected_socket->socket_id, socket_type) < 0) {
        perror("Error receiving request");
        exit(EXIT_FAILURE);
    }

    if (strcmp(socket_type, "client") == 0){
        printf("Client socket connected.\n");
        fflush(stdout);

        char operation[1024];
        memset(operation, 0, sizeof(operation));

        char result[1024];
        memset(result, 0, sizeof(result));

        if (receive_message(connected_socket->socket_id, operation) < 0) {
            perror("Error receiving request");
            exit(EXIT_FAILURE);
        }
        

        if(sem_trywait(&idle_workers) == 0){
            pthread_mutex_lock(&workers_change);
            int worker_index = 0;
            while(workers[worker_index].state != IDLE){
                worker_index++;
            }

            workers[worker_index].state = BUSY;
            pthread_mutex_unlock(&workers_change);

            
            send(workers[worker_index].data.socket_id, operation, sizeof(operation) + 1, 0);


            if (receive_message(workers[worker_index].data.socket_id, result) < 0) {
                perror("Error receiving result");
                exit(EXIT_FAILURE);
            }

            pthread_mutex_lock(&workers_change);
            workers[worker_index].state = IDLE;
            sem_post(&idle_workers);
            pthread_mutex_unlock(&workers_change);

            send(connected_socket->socket_id, result, sizeof(result) + 1, 0);
            close(connected_socket->socket_id);
        }else{
            snprintf(result, sizeof(result), "%s", "No available workers.");
            send(connected_socket->socket_id, result, sizeof(result) + 1, 0);
            close(connected_socket->socket_id);
        }
    }else if(strcmp(socket_type, "worker") == 0){
        if(workers_count < MAX_WORKERS){
            pthread_mutex_lock(&workers_change);
            struct socket_data *worker_data = (struct socket_data *) malloc(sizeof(struct socket_data));
            worker_data->socket_address = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
            worker_data->socket_address = connected_socket->socket_address;
            worker_data->socket_id = connected_socket->socket_id;

            struct worker *connected_worker = (struct worker*) malloc(sizeof(struct worker));

            connected_worker->data = *worker_data;
            connected_worker->state = IDLE;

            workers[workers_count] = *connected_worker;

            printf("Worker socket connected.\n");
            fflush(stdout);
            workers_count++;
            sem_post(&idle_workers);
            pthread_mutex_unlock(&workers_change);
        }else{
            printf("Worker tried to connect, but the workers buffer is full.\n");
            fflush(stdout);
            close(connected_socket->socket_id);
        }
    }else{
        printf("Unknow socket tried to connect.\n");
        fflush(stdout);
        close(connected_socket->socket_id);
    }
    return 0;
}

int main(int argc, char *argv[]){
    //Declarando um identificador para quem vai conectar no servidor
    int listen_id = 0, address_length;
    //Ponteiro para struct do tipo socket_data
    struct socket_data *connected_socket;

    pthread_mutex_init(&workers_change, NULL);

    signal(SIGINT, exit_handler);

    //Thread que vai definir quem é servidor e quem é cliente
    pthread_t routing_thread;

    if(sem_init(&idle_workers, 0, 0) != 0){
        fprintf(stderr, "Error creating semaphore");
        return 1;
    }
    
    //Declarando struct do tipo sockaddr_in para identificar o endereço do servidor
    struct sockaddr_in server_address; 
    
    //Cria o Socket do tipo TCP
    if((listen_id = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    	perror("socket");
    	return 1;
    }
    
    //Inicializa a struct do servidor com valor nulo
    memset(&server_address, 0, sizeof(server_address));

    //Configura o servidor para receber conexões TCP/IP de qualquer endereço
    server_address.sin_family = AF_INET;                //Familia de protocolo
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); //Permite conexões de qualquer IP
    server_address.sin_port = htons(PORT);              //Escuta as entradas na Porta definida (5000)

    //Vincula o socket criado com o endereço do servidor - Associa com o IP da máquina
    if (bind(listen_id, (struct sockaddr*)&server_address, sizeof(server_address)) < 0){
    	perror("bind");
    	return 1;
    }

    //Servidor esta escutando conecções
    listen(listen_id, BACKLOG); 

    //Fica aceitando coneções
    while(1){
        //Alocando espaço de memória pra uma struct do tipo socket_data
        connected_socket = (struct socket_data*) malloc(sizeof(struct socket_data));

        //Também tem que alocar espaço para o endereço do socket
        connected_socket->socket_address = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
    
        //Define o tamanho da structW
        address_length = sizeof(struct sockaddr_in);
        
        //Atribui o valor do id do socket
        connected_socket->socket_id = accept(listen_id, (struct sockaddr*)connected_socket->socket_address, (socklen_t*)&address_length);

        //Cria uma thread que vai definir quem é client e quem é worker
        pthread_create(&routing_thread, NULL, route_sockets, (void *) connected_socket);
        pthread_detach(routing_thread);
        //Atribui o valor do endereço do socket
    }   
}
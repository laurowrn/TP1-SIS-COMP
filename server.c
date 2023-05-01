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

//Struct para identificar socket - idependente se e worker ou server
struct socket_data{
    int socket_id;
    struct sockaddr_in *socket_address;
};

//Struct para o worker contendo uma struct socket_data e um estado que 
//indica se o worker ta ocioso ou trabalhando
struct worker{
    struct socket_data data;
    enum worker_state state;
};

//Variaveis globais dos workers
//Vetor de structs de workers
struct worker workers[MAX_WORKERS];
//Indicator para o numero de workers
int workers_count = 0;
//Mutex para gerenciar o acesso ao vetor de workers
pthread_mutex_t workers_change;
//Semaforo que indica o numero de workers disponiveis
sem_t idle_workers;

//Funcao para desligar o servidor com a desconexao dos workers
void exit_handler(int signal) {
    char exit_message[1024];
    memset(exit_message, 0, sizeof(exit_message));
    snprintf(exit_message, sizeof(exit_message), "%s", "quit");

    printf("\nDesconectando workers...\n");
    fflush(stdout);
    for(int i = 0; i < workers_count; i++){
        send(workers[i].data.socket_id, exit_message, sizeof(exit_message) + 1, 0);
    }
    sleep(2);

    printf("\nSever desligando...\n");
    fflush(stdout);
    exit(1);
}

//Funcao que percorre o buffer recebido byte a byte, terminando a leitura ao 
//encontrar o caractere nulo, retornando a quantidade de bytes recebidos
//caso haja um erro, retorna um valor menor que zero
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

//Funcao que executa as operacoes destinadas ao cliente, apos a indetificacao do socket
//pelo servidor
void client_socket(struct socket_data *connected_socket){
    //String para armazenar a operacao
    char operation[1024];
    memset(operation, 0, sizeof(operation));

    //String para armazenar o resultado
    char result[1024];
    memset(result, 0, sizeof(result));

    //Tenta receber os dados da mensagem de operacao. Caso de errado o servidor encerra
    if (receive_message(connected_socket->socket_id, operation) < 0) {
        perror("Error receiving request");
        exit(EXIT_FAILURE);
    }
    
    //Se receber normalmente a mensagem, tenta acesso ao semaforo dos workers.
    //Para conseguir entrar precisa ter workers disponiveis, caso contrario a conexao e encerrada -> else
    //Se o resultado do try_wait for igual a 0, indica que conseguiu acessar,
    if(sem_trywait(&idle_workers) == 0){
        //Usa o mutex para evitar condicao de corrida no acesso ao vetor de workers
        pthread_mutex_lock(&workers_change);
        //Percore todo o vetor, partindo do elemento zero, ate encontrar um worker ocioso
        int worker_index = 0;
        while(workers[worker_index].state != IDLE){
            worker_index++;
        }
        //Quando encontrar, declara que tal worker esta ocupado
        workers[worker_index].state = BUSY;
        //Libera o acesso com o unlock
        pthread_mutex_unlock(&workers_change);
        
        // envia para o worker conectado a operacao e os valores a serem calculados
        send(workers[worker_index].data.socket_id, operation, sizeof(operation) + 1, 0);

        //Recebe o resultado que o worker calculou
        //Caso de erro, o servidor e encerrado
        if (receive_message(workers[worker_index].data.socket_id, result) < 0) {
            perror("Error receiving result");
            exit(EXIT_FAILURE);
        }
        
        //Mutex que impede que mais de um worker mude seu estado para ocioso, evitando
        //condicao de corrida de clientes que buscam se conectar com algum worker
        pthread_mutex_lock(&workers_change);
        
        //Atualizacao do estado do worker apos a realizacao da operacao requisitada
        //pelo cliente
        workers[worker_index].state = IDLE;
        sem_post(&idle_workers);
        pthread_mutex_unlock(&workers_change);
        
        //Envia o resultado da operacao para o cliente
        send(connected_socket->socket_id, result, sizeof(result) + 1, 0);
    }else{
        //Caso nao tenha workers disponiveis ao tentar a conexao
        //Coloca no buffer result, e manda ao cliente a string
        snprintf(result, sizeof(result), "%s", "No available workers.");
        send(connected_socket->socket_id, result, sizeof(result) + 1, 0);
    }
    //Fecha a conexao com o cliente e libera o seu espaço de memória
    close(connected_socket->socket_id);
    free(connected_socket);
}

//Funcao que executa as operacoes destinadas ao worker, apos a indetificacao do socket
//pelo servidor
void worker_socket(struct socket_data *connected_socket) {
    //Verifica se ainda existem vagas para criar workers, visto que o numero maximo foi limitado
    if(workers_count < MAX_WORKERS){
        //Caso seja possivel adicionar, usa o mutex para garantir o acesso exclusivo aos dados do vetor de sockets
        pthread_mutex_lock(&workers_change);
        //Declara e inicializa a estrutura necessaria para um novo worker
        //Primeiro na struct socket_data
        struct socket_data *worker_data = (struct socket_data *) malloc(sizeof(struct socket_data));
        worker_data->socket_address = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
        worker_data->socket_address = connected_socket->socket_address;
        worker_data->socket_id = connected_socket->socket_id;
        //Em seguida, na struct worker
        struct worker *connected_worker = (struct worker*) malloc(sizeof(struct worker));

        //Adiciona os dados ao worker
        connected_worker->data = *worker_data;
        //Adiciona o estado de ocioso ao worker
        connected_worker->state = IDLE;

        //Adiciona o novo worker ao vetor de workers
        workers[workers_count] = *connected_worker;

        //Printa na tela que o worker foi adicionado
        printf("Worker socket connected.\n");
        fflush(stdout);
        //Adiciona o contador e o semaforo
        workers_count++;
        sem_post(&idle_workers);
        //Libera o mutex pois terminou sua seção crítica
        pthread_mutex_unlock(&workers_change);
    }else{
        //Nao ha espaco no vetor para alocar mais um socket worker, logo sua conexao e 
        //encerrada
        printf("Worker tried to connect, but the workers buffer is full.\n");
        fflush(stdout);
        close(connected_socket->socket_id);
    }
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

    //Recebe a primeira mensagem para identificar se qual socket esta tentando se conectar
    if (receive_message(connected_socket->socket_id, socket_type) < 0) {
        perror("Error receiving request");
        exit(EXIT_FAILURE);
    }

    //Verifica se a mensagem recebida vem de um socket worker ou client
    if (strcmp(socket_type, "client") == 0){
        printf("Client socket connected.\n");
        // printf("Client socket connected with worker %d.\n", connected_socket->socket_id);
        fflush(stdout);
        client_socket(connected_socket);    

    }else if(strcmp(socket_type, "worker") == 0){
        worker_socket(connected_socket);
    }else{
        //Caso a mensagem seja diferente das duas opcoes, a conexao e encerrada
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

    //Inicializa o mutex
    pthread_mutex_init(&workers_change, NULL);

    //Vincula a função de desligamento à interrupção do servidor
    signal(SIGINT, exit_handler);

    //Thread que vai definir quem é servidor e quem é cliente
    pthread_t routing_thread;

    //Criando o semaforo
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
    }   
}
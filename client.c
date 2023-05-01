#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

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

int main(int argc, char *argv[]){
    int server_id = 0, n = 0;
    char result[1024];
    char send_buffer[1024];
    struct sockaddr_in server_address; 

    //Confere se todos os argumentos de entrada necessarios foram passados
    if (argc < 3){
        printf("Usage: %s <ip address> <list of parameters>\n", argv[0]);
        exit(0);
    }

    //seta os valores do buffer para 0
    memset(result, 0, sizeof(result));
    memset(send_buffer, 0, sizeof(send_buffer));
    
    //Cria o Socket 
    if((server_id = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        return 1;
    } 

	//Configura o IP de destino e porta na estrutura sockaddr_in
    memset(&server_address, 0, sizeof(server_address)); 

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(5000); 

    //Atribui o IP da maquina passado na execucao do codigo ao endereco do socket
    if(inet_pton(AF_INET, argv[1], &server_address.sin_addr)<=0){
        perror("inet_pton");
        return 1;
    } 

	//Conecta ao servidor.
    if( connect(server_id, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
    	perror("connect");
       	return 1;
    } 

    // declara strings para cada argumento da operacao que o cliente deseja calcular
    char *operation = argv[2];
    char *first_number = argv[3];
    char *second_number = argv[4];

    //Envia pro servidor dizendo que e um cliente
    snprintf(send_buffer, sizeof(send_buffer), "%s", "client");
    send(server_id, send_buffer, strlen(send_buffer) + 1, 0);

    //Envia para o servido a operacao desejada e os valores a serem calculados
    snprintf(send_buffer, sizeof(send_buffer), "%s %s %s", operation, first_number, second_number);
    send(server_id, send_buffer, strlen(send_buffer) + 1, 0);

    //Recebe o resultado da operacao pelo servidor
    //Caso retorne menor que zero, indica um erro e a aplicacao e encerrada
    if (receive_message(server_id, result) < 0) {
        perror("Error receiving result");
        exit(EXIT_FAILURE);
    }

    //Printa o resultado
    printf("%s\n", result);
    fflush(stdout);

    return 0;
}
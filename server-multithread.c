/* Adaptado de https://www.thegeekstuff.com/2011/12/c-socket-programming/?utm_source=feedburner */

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

struct socket_data{
    int socket_id;
    struct sockaddr_in *socket_address;
};


int recv_msg(int sk, char* b){
    int i = 0, ret;
    while((ret = recv(sk, &b[i], 1, 0)) > 0){
        if(b[i] == '\0'){
            break;
        }
        i++;
    }
    if(ret < 0){
        return ret;
    }
    return i;
}

void * client_handle(void* connected_socket){
    struct socket_data *client = (struct socket_data *)connected_socket;
    char receive_buffer[1024];
    time_t ticks; 

    memset(receive_buffer, 0, sizeof(receive_buffer)); 

    /* Imprime IP e porta do cliente. */
    printf("Received connection from %s:%d\n", inet_ntoa(client->socket_address->sin_addr), ntohs(client->socket_address->sin_port));
    fflush(stdout);

    if (recv_msg(client->socket_id, receive_buffer) < 0) {
        perror("Error receiving request");
        exit(EXIT_FAILURE);
    }


    printf("%s", receive_buffer);
    fflush(stdout);


    if (recv_msg(client->socket_id, receive_buffer) < 0) {
        perror("Error receiving request");
        exit(EXIT_FAILURE);
    }

    printf("%s", receive_buffer);
    fflush(stdout);

/*     sleep(1);    
 
    ticks = time(NULL);
    snprintf(receive_buffer, sizeof(receive_buffer), "%.24s\r\n", ctime(&ticks));
        
    send(client->socket_id, receive_buffer, strlen(receive_buffer)+1, 0);

    close(client->socket_id);

    free(client->socket_address);
    free(client);

    return NULL; */
}

int main(int argc, char *argv[])
{
    int listen_id = 0;
    struct sockaddr_in server_address; 
    int address_length;
    struct socket_data *connected_socket;
    pthread_t routing_thread;

    /* Cria o Socket: SOCK_STREAM = TCP */
    if ( (listen_id = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    	perror("socket");
    	return 1;
    }

    memset(&server_address, 0, sizeof(server_address));

	/* Configura servidor para receber conexoes de qualquer endereço:
	 * INADDR_ANY e ouvir na porta 5000 */ 
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(5000); 

	/* Associa o socket a estrutura sockaddr_in */
    if (bind(listen_id, (struct sockaddr*)&server_address, sizeof(server_address)) < 0){
    	perror("bind");
    	return 1;
    } 

	/* Inicia a escuta na porta */
    listen(listen_id, 10); 

    while(1) {
        connected_socket = (struct socket_data *)malloc(sizeof(struct socket_data));
        connected_socket->socket_address = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
        address_length = sizeof(struct sockaddr_in);

		/* Aguarda a conexão */	
        connected_socket->socket_id = accept(listen_id, (struct sockaddr*)connected_socket->socket_address, (socklen_t*)&address_length); 

        pthread_create(&routing_thread, NULL, client_handle, (void *)connected_socket);
        pthread_detach(routing_thread);

     }
}


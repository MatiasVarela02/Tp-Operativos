#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>

#define IP_CPU "127.0.0.1"
#define PUERTO_CPU "4444"

typedef enum{
	MESSAGE,
	}message_code;

t_log* logger;

void* recibir_buffer(int*, int);
int iniciar_servidor(char* ip, char* puerto);
int esperar_cliente(int);
void recibir_mensaje(int);
int recibir_operacion(int);


#endif /* UTILS_H_ */

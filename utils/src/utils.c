#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include<commons/collections/list.h>
#include <commons/log.h>
#include <commons/config.h>
#include "utils.h"
#include<pthread.h>
#include <math.h>


/* ------------------------------ PROTOCOLO ------------------------------*/


void enviar_pcb(PCB* pcb,int socket_cliente, bool esConexionMemoria, PeticionMemoria codigo){

	int size = 2* sizeof(long)+ 6 * sizeof(int) + sizeof(double) + sizeof(status) + sizeof(float); //tamaño de pcb(sin instrucciones)
	void* stream = malloc(size);
	int desplazamiento = 0;

	if(esConexionMemoria){
		size += sizeof(int);

		memcpy(stream,&codigo, sizeof(PeticionMemoria));
		desplazamiento += sizeof(status);
	}

	memcpy(stream + desplazamiento, &(pcb->estado),sizeof(int));
	desplazamiento += sizeof(status);
	memcpy(stream + desplazamiento, &(pcb->estimacion_rafaga) ,sizeof(float) );
	desplazamiento += sizeof(float);
	memcpy(stream + desplazamiento, &(pcb->milisegundos_bloqueo) ,sizeof(long int) );
	desplazamiento += sizeof(long int);
	memcpy(stream + desplazamiento, &(pcb->pid), sizeof(long int));
	desplazamiento += sizeof(long int);
	memcpy(stream + desplazamiento, &(pcb->ultima_rafaga),sizeof(double) );
	desplazamiento += sizeof(double);
	memcpy(stream + desplazamiento, &(pcb->tabla_paginas),sizeof(int) );
	desplazamiento += sizeof(int);
	memcpy(stream + desplazamiento, &(pcb->tamanio),sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(stream + desplazamiento, &(pcb->cantidadInstrucciones),sizeof(int) );
	desplazamiento += sizeof(int);
	memcpy(stream + desplazamiento, &(pcb->cantInstruccionesEje),sizeof(int) );
	desplazamiento += sizeof(int);
	memcpy(stream + desplazamiento, &(pcb->parametros_pendientes),sizeof(int) );
	desplazamiento += sizeof(int);
	memcpy(stream + desplazamiento, &(pcb->socketTerminal),sizeof(int) );
	desplazamiento += sizeof(int);

	//mostrar_pcb(pcb);

	for(int i=0;i< pcb->cantidadInstrucciones;i++){

		size += tamanio_instruccion(pcb->instrucciones);
		stream = serializar_instruccion_pcb(pcb->instrucciones, stream, desplazamiento, size);
		desplazamiento = size;
		pcb->instrucciones = pcb->instrucciones->sig;

	}

	send(socket_cliente,stream,size,0);
	free(stream);
}

void* serializar_instruccion_pcb(INSTRUCCION* instruccion, void* stream, int desplazamiento, int size){

	stream = realloc(stream, size);

	memcpy(stream + desplazamiento, &instruccion->orden,sizeof(int) );
	desplazamiento += sizeof(int);
	memcpy(stream + desplazamiento, &instruccion->code, sizeof(op_code));
	desplazamiento += sizeof(op_code);
	memcpy(stream + desplazamiento, &instruccion->cant, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(stream + desplazamiento, instruccion->parametros, sizeof(int) * instruccion->cant);
	desplazamiento += sizeof(int) * instruccion->cant;
	


	return stream;
}


int tamanio_instruccion(INSTRUCCION* instruccion){

	int tamanio_instruccion = sizeof(int)*(2 + (instruccion->cant) ) + sizeof(op_code);

	return tamanio_instruccion;

}

PCB* recibir_pcb(int socket_cliente){


	PCB* pcb_recibido = malloc(sizeof(PCB));


	recv(socket_cliente, &pcb_recibido->estado, sizeof(int), MSG_WAITALL);
	recv(socket_cliente, &pcb_recibido->estimacion_rafaga, sizeof(float), MSG_WAITALL);
	recv(socket_cliente, &pcb_recibido->milisegundos_bloqueo, sizeof(long int), MSG_WAITALL);
	recv(socket_cliente, &pcb_recibido->pid, sizeof(long int), MSG_WAITALL);
	recv(socket_cliente, &pcb_recibido->ultima_rafaga, sizeof(double), MSG_WAITALL);
	recv(socket_cliente, &pcb_recibido->tabla_paginas, sizeof(int), MSG_WAITALL);
	recv(socket_cliente, &pcb_recibido->tamanio, sizeof(int), MSG_WAITALL);
	recv(socket_cliente, &pcb_recibido->cantidadInstrucciones, sizeof(int), MSG_WAITALL);
	recv(socket_cliente, &pcb_recibido->cantInstruccionesEje, sizeof(int), MSG_WAITALL);
	recv(socket_cliente, &pcb_recibido->parametros_pendientes, sizeof(int), MSG_WAITALL);
	recv(socket_cliente, &pcb_recibido->socketTerminal, sizeof(int), MSG_WAITALL);

	/*
	printf("\n Estado: %d\n ", pcb_recibido->estado);
	printf("\n Cant instr eje: %d\n ", pcb_recibido->cantInstruccionesEje);
	printf("\n Parametros pendientes: %d\n ", pcb_recibido->parametros_pendientes);
	printf("\n Cantidad total de instrucciones: %d\n", pcb_recibido->cantidadInstrucciones);
	*/

	pcb_recibido->instrucciones = NULL;

	insertar_instrucciones_pcb(pcb_recibido, socket_cliente);
	//log_info(logger,"Recibi las instrucciones");
	
	pcb_recibido->program_counter = pcb_recibido->instrucciones;

	if(pcb_recibido->cantInstruccionesEje){
		for(int i=0; i<pcb_recibido->cantInstruccionesEje;i++){

		pcb_recibido->program_counter = pcb_recibido->program_counter->sig;
		}
	}

	return pcb_recibido;

}



INSTRUCCION* deserializar_instruccion_pcb(int socket_cliente){
	INSTRUCCION* instruccion = malloc(sizeof(INSTRUCCION));

	recv(socket_cliente, &instruccion->orden, sizeof(int), MSG_WAITALL);
	recv(socket_cliente, &instruccion->code, sizeof(op_code), MSG_WAITALL);
	recv(socket_cliente, &instruccion->cant, sizeof(int), MSG_WAITALL);
	if(instruccion->cant > 0 ){ //si no es EXIT
		instruccion->parametros = malloc(sizeof(int)* instruccion->cant);
		recv(socket_cliente, instruccion->parametros, sizeof(int) * instruccion->cant, MSG_WAITALL);
	}

	return instruccion;
}



void insertar_instrucciones_pcb(PCB* pcb,int socket_cliente){
	for(int i=0;i< pcb->cantidadInstrucciones;i++){
		INSTRUCCION *actual = pcb->instrucciones;
		INSTRUCCION *nuevo = (INSTRUCCION *)malloc(sizeof(INSTRUCCION));
		nuevo = deserializar_instruccion_pcb(socket_cliente);

			if (pcb->instrucciones == NULL)
				{
					nuevo->sig = pcb->instrucciones;
					pcb->instrucciones = nuevo;
				}
			else
				{
					while (actual->sig != NULL)
					{
						actual = actual->sig;

					}

					nuevo->sig = actual->sig;
					actual->sig = nuevo;

				}
		}

}

PROCESO_SERIALIZADO* recibir_paquete(int socket_cliente)
{	
	int tamanioProceso;
	int tamanioBuffer;
	int cantidadInstrucciones;
	void * buffer;

	PROCESO_SERIALIZADO* procesoSerializado = malloc(sizeof(PROCESO_SERIALIZADO));

	recv(socket_cliente, &tamanioProceso, sizeof(int), MSG_WAITALL);	
	recv(socket_cliente, &tamanioBuffer, sizeof(int), MSG_WAITALL);
	recv(socket_cliente, &cantidadInstrucciones, sizeof(int), MSG_WAITALL);
	procesoSerializado->streamInstrucciones = malloc(tamanioBuffer);
	recv(socket_cliente, procesoSerializado->streamInstrucciones, tamanioBuffer, MSG_WAITALL);

	procesoSerializado->tamanioProceso = tamanioProceso;
	procesoSerializado->tamanioStream = tamanioBuffer;
	procesoSerializado->cantidadInstrucciones = cantidadInstrucciones;

	return procesoSerializado;
}


void liberar_conexion(int socket_cliente) {
	close(socket_cliente);
}

void enviar_texto(char* mensaje, int socket_cliente) {

	int size = strlen(mensaje) + 1;
	int bytes = size + 2 * sizeof(int);
	void * magic = malloc(bytes);
	int desplazamiento = 0;
	message_code operation = MESSAGE;

	memcpy(magic + desplazamiento, &(operation), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, &(size), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(magic + desplazamiento, mensaje, size);

	void* a_enviar = magic;
	send(socket_cliente, a_enviar, bytes, MSG_NOSIGNAL);
}

char* recibir_texto(int socket_cliente) {
	int size;
	int operacion;
	char* texto;
	recv(socket_cliente, &operacion, sizeof(int), MSG_WAITALL);
	if(operacion == MESSAGE){
		recv(socket_cliente, &size, sizeof(int), MSG_WAITALL);
		texto = malloc(size);
		recv(socket_cliente, texto, size, MSG_WAITALL);		
		//printf("%s\n",texto);
		return texto;
	}
	return "Error: El socket recibio algo que no es un mensaje";
}

int crear_conexion(char *ip, char* puerto) {
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = 0;
	socket_cliente = socket(server_info->ai_family, server_info->ai_socktype,
			server_info->ai_protocol);

	connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

	//freeaddrinfo(server_info);  esto estaba tirando segemtation fault
	return socket_cliente;

}

void* recibir_buffer(int* size, int socket_cliente) {
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void eliminar_paquete(PAQUETE* paquete) {
	free(paquete->buffer->streamInstrucciones);
	free(paquete->buffer);
	free(paquete);
}

void enviar_paquete(PAQUETE* paquete, int socket_cliente) {
	//log_info(logger, "Arranco a enviar el paquete");

	int bytes = paquete->buffer->tamanioStream + 4 * sizeof(int);
	void * a_enviar = malloc(bytes);
	int desplazamiento = 0;

	memcpy(a_enviar + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(a_enviar + desplazamiento, &(paquete->buffer->tamanioProceso),
			sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(a_enviar + desplazamiento, &(paquete->buffer->tamanioStream),
			sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(a_enviar + desplazamiento, &(paquete->buffer->cantidadInstrucciones),
			sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(a_enviar + desplazamiento, paquete->buffer->streamInstrucciones,
			paquete->buffer->tamanioStream);
	desplazamiento += paquete->buffer->tamanioStream;

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);

}


PAQUETE* crear_paquete(PROCESO_SERIALIZADO* procesoSerializado) {
	PAQUETE* paquete = malloc(sizeof(PAQUETE));
	paquete->codigo_operacion = PACKAGE;
	paquete->buffer = malloc(sizeof(PROCESO_SERIALIZADO));
	paquete->buffer = procesoSerializado;

	return paquete;
}

void serializar_instruccion(INSTRUCCION* instruccion,
		PROCESO_SERIALIZADO* procesoSerializado) {

	int tamanioInstruccion = sizeof(int) * (2 + (instruccion->cant))
			+ sizeof(op_code);

	//en la primer iteracion funciona como malloc
	procesoSerializado->streamInstrucciones = realloc(
			procesoSerializado->streamInstrucciones,
			procesoSerializado->tamanioStream + tamanioInstruccion);
	void* stream = malloc(tamanioInstruccion);
	int offset = 0;

	memcpy(stream + offset, &instruccion->orden, sizeof(int));
	offset += sizeof(int);
	memcpy(stream + offset, &instruccion->code, sizeof(op_code));
	offset += sizeof(op_code);
	memcpy(stream + offset, &instruccion->cant, sizeof(int));
	offset += sizeof(int);
	memcpy(stream + offset, instruccion->parametros,
			sizeof(int) * instruccion->cant);

	memcpy(
			(procesoSerializado->streamInstrucciones)
					+ (procesoSerializado->tamanioStream), stream,
			tamanioInstruccion);
	procesoSerializado->tamanioStream += tamanioInstruccion;

	free(stream);

}


void serializar_proceso(PROCESO* proceso,
		PROCESO_SERIALIZADO* procesoSerializado) {
	INSTRUCCION* instruccionActual = malloc(sizeof(INSTRUCCION));
	instruccionActual = proceso->instrucciones;

	for (int i = 0; i < proceso->cantidadInstrucciones; ++i) {
		serializar_instruccion(instruccionActual, procesoSerializado);
		instruccionActual = instruccionActual->sig;
	}

	procesoSerializado->cantidadInstrucciones = proceso->cantidadInstrucciones;

}

int esperar_cliente_procesador(int socket_servidor)
{

	int socket_cliente = accept(socket_servidor, NULL, NULL);
	log_info(logger, "Se conecto un cliente!");

	return socket_cliente;
}

int iniciar_servidor(char* ip,char* puerto)
{

	int socket_servidor;

	struct addrinfo hints, *servinfo, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &servinfo);

	socket_servidor = socket(servinfo->ai_family,
	                    servinfo->ai_socktype,
	                    servinfo->ai_protocol);

	int yes = 1;
	setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);

	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);

	listen(socket_servidor, SOMAXCONN);


	freeaddrinfo(servinfo);

	//log_info(logger, "Socket servidor: %d ",socket_servidor);
	return socket_servidor;

}

int esperar_cliente(t_log* logger, const char* name, int socket_servidor) {
    struct sockaddr_in dir_cliente;
    socklen_t tam_direccion = sizeof(struct sockaddr_in);

    int socket_cliente = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

    //log_info(logger, "Cliente conectado a %s\n", name);

    return socket_cliente;
}




void deserializar_proceso(PROCESO_SERIALIZADO* procesoSerializado,PROCESO* procesoDeserializado){

	procesoDeserializado->instrucciones = NULL;
	int desplazamiento = 0;


	 for (int i = 0; i < procesoSerializado->cantidadInstrucciones  ; ++i){

		INSTRUCCION* instruccionActual = procesoDeserializado->instrucciones;
		INSTRUCCION *nuevo = (INSTRUCCION *)malloc(sizeof(INSTRUCCION));
		desplazamiento = deserializar_instruccion(nuevo, procesoSerializado->streamInstrucciones, desplazamiento);

		if (procesoDeserializado->instrucciones == NULL)
			{
				nuevo->sig = procesoDeserializado->instrucciones;
				procesoDeserializado->instrucciones = nuevo;
			}
		else
			{
				while (instruccionActual->sig != NULL)
				{
					instruccionActual = instruccionActual->sig;

				}

				nuevo->sig = instruccionActual->sig;
				instruccionActual->sig = nuevo;

			}
	 }


	procesoDeserializado->cantidadInstrucciones = procesoSerializado->cantidadInstrucciones;
	procesoDeserializado->tamanioProceso = procesoSerializado->tamanioProceso;

}

int deserializar_instruccion(INSTRUCCION* instruccion, void* stream, int desplazamientoInicial) {

	int desplazamiento = desplazamientoInicial;

    memcpy(&(instruccion->orden), stream + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(&(instruccion->code), stream + desplazamiento, sizeof(op_code));
    desplazamiento += sizeof(op_code);
    memcpy(&(instruccion->cant), stream + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);

    instruccion->parametros = malloc(sizeof(int)* instruccion->cant);
    memcpy(instruccion->parametros, stream + desplazamiento, sizeof(int)* instruccion->cant);

    desplazamiento += (sizeof(int)* instruccion->cant);

    return desplazamiento;

}

void terminar_programa(int conexion)
{
	liberar_conexion(conexion);
}

int server_escuchar(t_log* logger, char* server_name, int server_socket,int conexion_Dispatch,CONFIGURACION_KERNEL* configKernelDatos) {
    int cliente_socket = esperar_cliente(logger, server_name, server_socket);

    if (cliente_socket != -1) {
        pthread_t hilo;
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        args->log = logger;
        args->fd = cliente_socket;
        args->server_name = server_name;
        args->conexion_Dispatch = conexion_Dispatch;
        args->configKernelDatos = configKernelDatos;

        pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) args);
        pthread_detach(hilo);
        return 1;
    }
    return 0;
}

/* ------------------------------ TERMINAL ------------------------------*/

void mostrarInstrucciones(INSTRUCCION *listaInstrucciones) {
	printf("\nListado de Instrucciones:\n");
	INSTRUCCION *aux = listaInstrucciones;
	while (aux != NULL) {
		printf("valor de enum: %d  ;  \n", aux->code);
		printf("Cantidad parametros: %d  \n", aux->cant);
		if (aux->cant != 0) {
			printf("primer parametro: %d \n", aux->parametros[0]);
			if (aux->cant == 2) {
				printf("segundo parametro: %d \n", aux->parametros[1]);
			}
		}

		printf("orden de llegada de la instruccion: %d  \n;  ", aux->orden);
		printf("--------------------------------------------- \n");

		aux = aux->sig;
	}
}


int instruccionValida(char* codigoOperacion) {
	return ((strcmp(codigoOperacion, "NO_OP") == 0)
			|| (strcmp(codigoOperacion, "I/O") == 0)
			|| (strcmp(codigoOperacion, "READ") == 0)
			|| (strcmp(codigoOperacion, "WRITE") == 0)
			|| (strcmp(codigoOperacion, "COPY") == 0)
			|| (strcmp(codigoOperacion, "EXIT") == 0));
}

PROCESO_SERIALIZADO* inicializar_proceso_serializado(int tamanioProceso) {
	PROCESO_SERIALIZADO* procesoSerializado = malloc(
			sizeof(PROCESO_SERIALIZADO));
	procesoSerializado->tamanioStream = 0;
	procesoSerializado->streamInstrucciones = NULL;
	procesoSerializado->tamanioProceso = tamanioProceso;
	return procesoSerializado;
}


void insertarInstruccion(INSTRUCCION **listaInstrucciones, char *texto,
		int parametros[], int cantidad, int orden) {
	INSTRUCCION *actual = *listaInstrucciones;
	INSTRUCCION *nuevo = (INSTRUCCION *) malloc(sizeof(INSTRUCCION));
	int* aux = (int*) malloc(sizeof(int) * cantidad);

	if (strcmp(texto, "NO_OP") == 0) {
		nuevo->code = 0;
	} else if (strcmp(texto, "I/O") == 0) {
		nuevo->code = 1;
	} else if (strcmp(texto, "READ") == 0) {
		nuevo->code = 2;
	} else if (strcmp(texto, "WRITE") == 0) {
		nuevo->code = 3;
	} else if (strcmp(texto, "COPY") == 0) {
		nuevo->code = 4;
	} else if (strcmp(texto, "EXIT") == 0) {
		nuevo->code = 5;
	}

	nuevo->orden = orden;

	if (cantidad >= 1) {
		aux[0] = parametros[0];
	}
	if (cantidad == 2) {
		aux[1] = parametros[1];
	}

	nuevo->parametros = aux;
	nuevo->cant = cantidad;

	if (*listaInstrucciones == NULL) {
		nuevo->sig = *listaInstrucciones;
		*listaInstrucciones = nuevo;
	} else {
		while (actual->sig != NULL) {
			actual = actual->sig;
		}

		nuevo->sig = actual->sig;
		actual->sig = nuevo;
	}

}


/* ------------------------------ PROCESADOR ------------------------------*/

int recibirInterrupcion(int socket_cliente){
	int hayInterrupcion=0;
	recv(socket_cliente, &hayInterrupcion, sizeof(int), MSG_WAITALL);

	return hayInterrupcion == 1;
}

void mostrarInstruccion(INSTRUCCION* instruccion){
	if(instruccion!=NULL){

		switch (instruccion->code) {
			case NO_OP:;
				printf(" \033[38;5;195m NO_OP\n");
				break;
			case IO:
				printf(" \033[38;5;195m IO\n");
				break;
			case EXIT:
				printf("\033[38;5;195m EXIT\n");
				break;
			case READ:
				printf("\033[38;5;195m READ\n");
				break;
			case WRITE:
				printf("\033[38;5;195m WRITE\n");
				break;
			case COPY:
				printf("\033[38;5;195m COPY\n");
				break;
			default:
				printf("Codigo no reconocido\n");
				return;
			}
		for (int i = 0; i < instruccion->cant; ++i){

			printf("\033[38;5;183m Parametro %d: %d \n", i+1, instruccion->parametros[i]);

		}
	}
	else{
		printf("No hay Intruccion a ejecutar\n");
	}

}


PROCESSOR_ARQ* inicializarArqProcesador(void){
	PROCESSOR_ARQ* procesador = malloc(sizeof(PROCESSOR_ARQ));
	procesador->program_counter = NULL;
	procesador->instruction_executed_register = 0;
	procesador->memory_address_register = 0;
	procesador->memory_buffer_register = 0;
	procesador->tiempo_NOOP = 0;
	procesador->tipo_syscall = NONE;
	procesador->param_syscall = 0;

	return procesador;
}


void fetch(PCB* pcb_recibido, PROCESSOR_ARQ* procesador){

	if(procesador->tipo_syscall == SYSREPEAT){
		//No cambio de instruccion sino que repito la anterior con una iteracion menos
		*procesador->program_counter->parametros = procesador->param_syscall;

		//reseteo el indicador de syscalls
		procesador->param_syscall = 0;
		procesador->tipo_syscall = NONE;
		return;
	}
	else if(procesador->nuevoPCB){
		//Cargo la primera instruccion del pcb
		procesador->program_counter = pcb_recibido->program_counter;
		procesador->nuevoPCB = false;
		if(pcb_recibido->parametros_pendientes != 0){ //sigo una NOOP con las iteraciones restantes
			printf("Habia parametros pendientes");
			*procesador->program_counter->parametros = pcb_recibido->parametros_pendientes;
			pcb_recibido->parametros_pendientes = 0;
		}
	}
	else{
		//si no se cambio de pcb entonces sigo con la siguiente instruccion
		log_info(logger, "Siguiente instruccion");
		procesador->program_counter = procesador->program_counter->sig;
		procesador->instruction_executed_register++;
	}
}

void decode_and_fetch_operands(PCB* pcb_recibido,PROCESSOR_ARQ* procesador, char* reemplazo_tlb, int conexion_memoria){

	switch (procesador->program_counter->code) {
		case READ:
			procesador->memory_address_register = *procesador->program_counter->parametros;
			return;
		case WRITE:
			procesador->memory_address_register = procesador->program_counter->parametros[0];
			procesador->memory_buffer_register = procesador->program_counter->parametros[1];
			return;
		case COPY:
			procesador->memory_address_register = procesador->program_counter->parametros[0];
			int valorACopiar = pedirValorAMemoria(procesador->TLB, procesador->filasTLB,  procesador->program_counter->parametros[1], procesador->memoria_tamanio_pagina, procesador->memoria_entradas_por_tabla, reemplazo_tlb, conexion_memoria, pcb_recibido->tabla_paginas);
			procesador->memory_buffer_register = valorACopiar;
			return;
		default:
			return;
 	}
}

void execute(PROCESSOR_ARQ* procesador, char* reemplazo_tlb, int conexion_memoria){

	switch (procesador->program_counter->code) {
		case NO_OP:; //dormir por x tiempo e indicar que la operacion se repite 
			float tiempoEnSegundos = procesador->tiempo_NOOP/1000;
			sleep(tiempoEnSegundos);
			int iteracionesPendientes = *procesador->program_counter->parametros - 1; 
			if(iteracionesPendientes > 0){
				procesador->tipo_syscall = SYSREPEAT;
				procesador->param_syscall = iteracionesPendientes;
			}
			break;
		case IO://devolver pcb con tiempo de bloqueo
			procesador->tipo_syscall = SYSIO;
			procesador->param_syscall = *procesador->program_counter->parametros;
			break; 
		case EXIT://fin del programa, devolver PCB
			log_info(logger, "Mando sysexit");
			procesador->tipo_syscall = SYSEXIT;
			break; 
		case READ:
			pedirValorAMemoria(procesador->TLB, procesador->filasTLB, procesador->memory_address_register, procesador->memoria_tamanio_pagina, procesador->memoria_entradas_por_tabla, reemplazo_tlb, conexion_memoria, procesador->tabla_primer_nivel);
			log_info(logger, "\033[38;5;159mLei el valor de memoria: %d", procesador->memory_buffer_register);
			break;
		case WRITE:
			escribirEnMemoria(procesador->TLB, procesador->filasTLB,procesador->memory_address_register, procesador->memory_buffer_register, procesador->memoria_tamanio_pagina, procesador->memoria_entradas_por_tabla, reemplazo_tlb, conexion_memoria, procesador->tabla_primer_nivel);
			break;
		case COPY:
			escribirEnMemoria(procesador->TLB, procesador->filasTLB,procesador->memory_address_register, procesador->memory_buffer_register, procesador->memoria_tamanio_pagina, procesador->memoria_entradas_por_tabla, reemplazo_tlb, conexion_memoria, procesador->tabla_primer_nivel);
			break;
		default:
			log_error(logger,"Error, la operacion es desconocida");
			break;
		}
}


void actualizar_PCB(PCB* pcb_recibido, PROCESSOR_ARQ* procesador, double tiempo_rafaga, int interrupciones){
	//actualizo el estado con el que sale de la cpu
	if(procesador->tipo_syscall == SYSEXIT){
		pcb_recibido->estado = EXIT_STATUS;
		return;
	}
	else if(procesador->tipo_syscall == SYSREPEAT){
		procesador->instruction_executed_register--; 
		//para el contador quede igual, compensa el ++ de adelante pq no se corre la instruccion completa
		pcb_recibido->parametros_pendientes = procesador->param_syscall;

		procesador->tipo_syscall = NONE;
		procesador->param_syscall = 0;

	}
	else if(procesador->tipo_syscall == SYSIO){
		//agrego los milisegundos de bloqueo por IO
		pcb_recibido->milisegundos_bloqueo = procesador->param_syscall;
		pcb_recibido->estado = BLOCKED;
	}			
	else if(interrupciones){
		pcb_recibido->estado = READY;
	}	

	//Actualiza el PC del PCB con la siguiente instruccion a ejecutar cuando vuelva al procesador
	//o bien con NULL si se hizo un exit
	log_info(logger, "Recien corri la instruccion: ");
	mostrarInstruccion(procesador->program_counter);

	procesador->instruction_executed_register++;
	pcb_recibido->cantInstruccionesEje = procesador->instruction_executed_register;

	log_info(logger,"La prox instruccion a correr es: ");
	mostrarInstruccion(procesador->program_counter->sig);
	printf("\nInstrucciones ejecutadas: %i\n",pcb_recibido->cantInstruccionesEje);

	pcb_recibido->ultima_rafaga = tiempo_rafaga;  
}

DIRECCION_FISICA* MMU(int direccion_logica, int tamanio_pagina, int entradas_por_tabla){
	DIRECCION_FISICA* direccion= malloc(sizeof(DIRECCION_FISICA));
	double np = direccion_logica / tamanio_pagina;

	int numero_pagina = floor(np);
	int entrada_tabla_1er_nivel = floor(numero_pagina / entradas_por_tabla);
	int entrada_tabla_2do_nivel = numero_pagina % (entradas_por_tabla);
	int desplazamiento = direccion_logica - (numero_pagina * tamanio_pagina);

	direccion->entrada_tabla_1er_nivel = entrada_tabla_1er_nivel;
	direccion->entrada_tabla_2do_nivel = entrada_tabla_2do_nivel;
	direccion->desplazamiento = desplazamiento;

	return direccion;
}

int pedirValorAMemoria(int tlb [][4], int filasTLB, int direccion_logica, int tamanio_pagina, int entradas_por_tabla, char* reemplazo_tlb, int conexion_memoria, int tabla_primer_nivel ){
	int marco;
	int valor;
	double np = direccion_logica / tamanio_pagina;
	int numero_pagina = floor(np);
	DIRECCION_FISICA* direccion_fisica = MMU(direccion_logica, tamanio_pagina, entradas_por_tabla);

	marco = buscarEnTLB (tlb,filasTLB, numero_pagina, direccion_fisica, reemplazo_tlb, conexion_memoria, tabla_primer_nivel);
	
	pedirLecturaPorMarco(conexion_memoria, marco, direccion_fisica->desplazamiento);
	valor = recibirRegistroDeMemoria(conexion_memoria);
	
	return valor;
}

void escribirEnMemoria(int tlb [][4], int filasTLB, int direccion_logica, int valor, int tamanio_pagina, int entradas_por_tabla, char* reemplazo_tlb, int conexion_memoria, int tabla_primer_nivel){
	int marco;


	double np = direccion_logica / tamanio_pagina;
	int numero_pagina = floor(np);

	log_info(logger, "Voy a hacer una escritura del valor %d", valor);
	printf("NP: %d \n",numero_pagina);


	DIRECCION_FISICA* direccion_fisica = MMU(direccion_logica, tamanio_pagina, entradas_por_tabla);

	log_info(logger, "Traduje la direccion fisica, que tiene como valores:");
	printf("T1: %d \n",direccion_fisica->entrada_tabla_1er_nivel);
	printf("T2: %d \n",direccion_fisica->entrada_tabla_2do_nivel);
	printf("D: %d \n",direccion_fisica->desplazamiento);

	log_info(logger, "Voy a buscar el marco");

	marco = buscarEnTLB(tlb,filasTLB, numero_pagina, direccion_fisica, reemplazo_tlb, conexion_memoria, tabla_primer_nivel);

	log_info(logger, "EL marco es: %d ", marco);

	pedirEscrituraPorMarco(conexion_memoria ,marco, direccion_fisica->desplazamiento, valor);
	char* respuesta = recibir_texto(conexion_memoria);
	if(strcmp(respuesta, "OK") == 0){
		log_info(logger, "\033[38;5;120m La escritura fue exitosa!!");
	}
	else{
		log_error(logger,"Hubo un fallo en la escritura :(");
	}
}

void vaciarTLB (int tlb [][4], int filasTLB){
	memset(tlb, -1, sizeof(tlb[0][0]) * 4 * filasTLB);
}

void insertarEnTLB (int tlb [][4], int filasTLB, int numero_pagina, int numero_marco, char* reemplazo_tlb){

	log_info(logger, "\033[38;5;120mRecibi el marco: %d",numero_marco);
	/*primero recorro y me fijo si el marco ya estaba en la TLB
	  en caso que si, piso esa entrada y hago un return*/
	int reemplazoMarco = 0;
	int posicion = 0;

	for (int i = 0; i < filasTLB; i++){

        if (tlb[i][1] == numero_marco)
        {
           reemplazoMarco = 1;
		   posicion = i;
		   break;
        }
    }

	if(reemplazoMarco){		
		time_t tiempoActual = time(NULL);
		tlb[posicion][0] = numero_pagina;
		tlb[posicion][2] = tiempoActual;
		tlb[posicion][3] = tiempoActual;
		mostrarTLB(tlb, filasTLB);

		return;
	}

	/*luego recorro y me fijo si hay un espacio libre
	  en caso que si, lo guardo ahi y hago un return */

	int found = 0;
	posicion = 0;

	for (int j = 0; j < filasTLB; j++){

        if (tlb[j][0] == -1)
        {
           found = 1;
		   posicion = j;
		   break;
        }
    }

	if(found){		
		time_t tiempoActual = time(NULL);
		tlb[posicion][0] = numero_pagina;
		tlb[posicion][1] = numero_marco;
		tlb[posicion][2] = tiempoActual;
		tlb[posicion][3] = tiempoActual;
		mostrarTLB(tlb, filasTLB);

		return;
	}

	//sino reemplazo segun el algoritmo

	else{
		int columnaAlgoritmo;
		if(strcmp(reemplazo_tlb, "FIFO") == 0){
			columnaAlgoritmo = 2;
		}
		else if(strcmp(reemplazo_tlb, "LRU") == 0){
			columnaAlgoritmo = 3;
		}
		else{
			printf("No se reconocio el algoritmo de reemplazo de la TLB");
			return;
		}			
		reemplazoPorAlgoritmo(tlb, filasTLB, numero_pagina, numero_marco, columnaAlgoritmo);
	}

	mostrarTLB(tlb, filasTLB);

}

int buscarEnTLB (int tlb [][4], int filasTLB, int numero_pagina, DIRECCION_FISICA* direccion_fisica, char* reemplazo_tlb, int conexion_memoria, int tabla_primer_nivel){

	int marco = buscarPorPagina( tlb, filasTLB, numero_pagina);
	if(marco == -1){
	log_info(logger, "El marco no esta en la TLB :( le pido la tabla de 2n a memoria ");


	PeticionMemoria codigo_t_segundo_nivel = SEGUNDATABLA;
	pedirRegistroAMemoria(conexion_memoria, tabla_primer_nivel, direccion_fisica->entrada_tabla_1er_nivel, codigo_t_segundo_nivel);
	
	int tabla_segundo_nivel = recibirRegistroDeMemoria(conexion_memoria);

	log_info(logger, "\033[38;5;120mMemoria me devolvio %d", tabla_segundo_nivel);

	PeticionMemoria codigo_marco = MARCO;
	pedirRegistroAMemoria(conexion_memoria, tabla_segundo_nivel, direccion_fisica->entrada_tabla_2do_nivel, codigo_marco);

	marco = recibirRegistroDeMemoria(conexion_memoria);

	insertarEnTLB (tlb, filasTLB, numero_pagina, marco, reemplazo_tlb);
	}

	return marco;
}

void pedirRegistroAMemoria(int conexion_memoria,int numero_tabla, int entrada,PeticionMemoria codigoPeticion ){
	t_buffer* buffer = malloc(sizeof(t_buffer));

	buffer->size = sizeof(int) * 3;

	void* stream = malloc(buffer->size);
	int offset = 0;

	memcpy(stream + offset,&codigoPeticion, sizeof(int));
	offset += sizeof(int);
	memcpy(stream + offset,&numero_tabla, sizeof(int));
	offset += sizeof(int);
	memcpy(stream + offset,&entrada, sizeof(int));
	offset += sizeof(int);

	buffer->stream = stream;

	send(conexion_memoria,stream,buffer->size,0);
	free(stream);
}

int recibirRegistroDeMemoria(int conexion_memoria){
	int registro;

	recv(conexion_memoria, &registro, sizeof(int), MSG_WAITALL);

	log_info(logger, "Recibi un registro de memoria");

	return registro;
}

int buscarPorPagina(int tlb [][4], int filasTLB, int numero_pagina){
	int j;
	int found = 0;
	int posicion = 0;

	mostrarTLB(tlb, filasTLB);

	//recorro la primer columna
	for (int j = 0; j < filasTLB; j++){

        if (tlb[j][0] == numero_pagina)
        {
           found = 1;
		   posicion = j;
        }
    }
	if(found){
		//como encontre, guardo ultimo uso y devuelvo el marco
		log_info(logger, "\033[38;5;195m El marco estaba en la tlb");

		time_t tiempoActual = time(NULL);
		tlb[posicion][3] = tiempoActual;
		return tlb[posicion][1];
	}
	//si no encuentro devuelvo -1
	return -1;
}

void mostrarTLB(int TLB [][4], int filasTLB){
	/* NUMERO DE PAGINA | MARCO | TIEMPO DE INSERCION | TIEMPO DE ULTIMO USO */
	log_info(logger, "\033[0;38;5;117mNRO DE PAGINA |  MARCO  |  TIEMPO INSERCION  | TIEMPO ULT USO");
	for (int i = 0; i < filasTLB; i++)
    {
        /*for (int j = 0; j < 4; j++) {
            printf("%d | ", TLB[i][j]);			
        }*/
        //printf("\n");
    	
		int j = 0;
		log_info(logger, "\033[0;38;5;121m\t%d \t  |    %d    |     %d  \t |    %d", TLB[i][j], TLB[i][j+1],TLB[i][j+2],TLB[i][j+3]); 
		//la idea es con un logger pero sale mal y me tengo q ir, mas tarde veo sino si puede otro

    }
}


void reemplazoPorAlgoritmo(int tlb [][4], int filasTLB, int numero_pagina, int numero_marco, int columna){
	log_info(logger, "\033[38;5;212m Hay que hacer un reemplazo en la tlb");
	
	int j;
	int posicion = 0;
	int aReemplazar = tlb[posicion][columna];

	for (int j = 0; j < filasTLB; j++){
		//indica que la primer fecha es mayor a la segunda, o sea guardo la mas vieja
        if (difftime(aReemplazar, tlb[j][columna]) > 0){
		    posicion = j;
			aReemplazar = tlb[j][columna];
        }
    }

	time_t tiempoActual = time(NULL);
	tlb[posicion][0] = numero_pagina;
	tlb[posicion][1] = numero_marco;
	tlb[posicion][2] = tiempoActual;
	tlb[posicion][3] = tiempoActual;

	log_info(logger, "\033[38;5;212m El reemplazo se hizo exitosamente en la posicion %d !", posicion);

}

void pedirLecturaPorMarco(int conexion_memoria, int marco, int desplazamiento){
	PeticionMemoria codigo_lectura = LEER;

	t_buffer* buffer = malloc(sizeof(t_buffer));

	buffer->size = sizeof(int) * 3;

	void* stream = malloc(buffer->size);
	int offset = 0;

	memcpy(stream + offset,&codigo_lectura, sizeof(PeticionMemoria));
	offset += sizeof(int);
	memcpy(stream + offset,&marco, sizeof(int));
	offset += sizeof(int);
	memcpy(stream + offset,&desplazamiento, sizeof(int));
	offset += sizeof(int);

	buffer->stream = stream;

	send(conexion_memoria,stream,buffer->size,0);
	free(stream);
}


void pedirEscrituraPorMarco(int conexion_memoria, int marco, int desplazamiento, int valor){
	PeticionMemoria codigo_lectura = ESCRIBIR;

	t_buffer* buffer = malloc(sizeof(t_buffer));

	buffer->size = sizeof(int) * 4;

	void* stream = malloc(buffer->size);
	int offset = 0;

	memcpy(stream + offset,&codigo_lectura, sizeof(PeticionMemoria));
	offset += sizeof(int);
	memcpy(stream + offset,&marco, sizeof(int));
	offset += sizeof(int);
	memcpy(stream + offset,&desplazamiento, sizeof(int));
	offset += sizeof(int);
	memcpy(stream + offset,&valor, sizeof(int));
	offset += sizeof(int);

	buffer->stream = stream;

	send(conexion_memoria,stream,buffer->size,0);
	free(stream);
}

/* ------------------------------ KERNEL ------------------------------*/

void estimarRafaga(PCB* pcb, float alfa){
	float nuevaEstimacion = (alfa * pcb->ultima_rafaga) + ( (1-alfa) * pcb->estimacion_rafaga );
	pcb->estimacion_rafaga = nuevaEstimacion;
}

void mostrar_pcb(PCB* pcb){

	if(pcb!=NULL){
		printf("\npid: %li\n",pcb->pid);
		printf("estado: %i\n",pcb->estado);
		printf("estimacion rafaga: %.2f\n",pcb->estimacion_rafaga);
		printf("bloqueo: %li\n",pcb->milisegundos_bloqueo);
		printf("tabla paginas: %i\n",pcb->tabla_paginas);
		printf("tamanio: %i\n",pcb->tamanio);
		printf("Cant Instrucciones: %i\n",pcb->cantidadInstrucciones);
		printf("Cant Instrucciones Ejecutadas: %i\n",pcb->cantInstruccionesEje);

		if(pcb->program_counter!=NULL){
			printf("Program counter orden: %i\n",pcb->program_counter->orden);
			printf("Program counter code: %i\n",pcb->program_counter->code);
			printf("Program counter cant: %i\n",pcb->program_counter->cant);
			for (int i = 0; i < pcb->program_counter->cant; ++i){

				printf("Parametro %d: %d \n", i+1, pcb->program_counter->parametros[i]);

			}
		}
		else if(pcb->program_counter==NULL){
			printf("\nProgram counter = NULL\n");
		}

		if(pcb->instrucciones== NULL){
			printf("\nInstrucciones = NULL\n");
		}

		INSTRUCCION *aux = (INSTRUCCION *)malloc(sizeof(INSTRUCCION));
		aux = pcb->instrucciones;
		while (aux != NULL){

			printf("\nINSTRUCCION %i\n",aux->orden);
			printf("Orden: %i\n",aux->orden);
			printf("Code: %i\n",aux->code);
			printf("Cant: %i\n",aux->cant);

			for (int i = 0; i < aux->cant  ; ++i){

				printf("Parametro %d: %d \n", i+1, aux->parametros[i]);

			}

			aux = aux->sig;
		}

		free(aux);
	}
}

void liberar_pcb(PCB* pcb){
	while(pcb->instrucciones != NULL){
		INSTRUCCION* aux_instruccion = pcb->instrucciones;
		pcb->instrucciones = pcb->instrucciones->sig;

		free(aux_instruccion);
	}

	free(pcb->program_counter);

	free(pcb);
}


t_config* iniciar_config(char* direccion)
{
	t_config* nuevo_config;

	if((nuevo_config = config_create(direccion)) == NULL){
		log_error(logger, "No pude iniciar la config\n");
		exit(2);
	}

	return nuevo_config;
}


PCB* crearPCB(PROCESO* proceso, CONFIGURACION_KERNEL* configKernelDatos, int socket) {
	PCB* pcb = malloc(sizeof(PCB));
	pcb->pid = nro_proceso;
	nro_proceso++;
	pcb->tamanio = proceso->tamanioProceso;
	pcb->tabla_paginas = 0;
	pcb->estimacion_rafaga = configKernelDatos->estimacion_inicial;
	pcb->estado = NEW;
	pcb->milisegundos_bloqueo = 0;
	pcb->cantidadInstrucciones = proceso->cantidadInstrucciones;
	pcb->cantInstruccionesEje = 0;
	pcb->program_counter = proceso->instrucciones;
	pcb->instrucciones = proceso->instrucciones;
	pcb->ultima_rafaga = configKernelDatos->estimacion_inicial;
	pcb->parametros_pendientes = 0;
	pcb->socketTerminal = socket;

	return pcb;
}


static void procesar_conexion(void* void_args)  {
	// aca tengo que crear el pcb, por cada instanciacion de consola => un nuevo pcb



	    t_procesar_conexion_args* args = (t_procesar_conexion_args*) void_args;
	    t_log* logger = args->log;
	    int cliente_socket = args->fd;
	    char* server_name = args->server_name;
	    int conexion_Dispatch = args->conexion_Dispatch;
	    CONFIGURACION_KERNEL* configKernelDatos= malloc(sizeof(CONFIGURACION_KERNEL));
	    configKernelDatos = args->configKernelDatos;
	    free(args);

	    op_code cop;
	    int nro_proceso = 1;
	     //while (cliente_socket != -1) {
            /*
	         if (recv(cliente_socket, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
	             log_info(logger, "DISCONNECT!");
	             return;
	         }
	         */
	    	 recv(cliente_socket, &cop, sizeof(op_code), 0);
	    	 printf(" el codi de ope es %d \n",cop );

	        switch (cop) {

	    	case MESSAGE: ;//Recibo mensaje de la consola

	    		char* mensajeRecibido = recibir_texto(cliente_socket);
	    			printf("El mensaje fue recibido: %s \n",mensajeRecibido);

	    			//Envio respuesta a la consola
	    			char* rta="Hola Terminal";
	    			enviar_texto(rta,cliente_socket);
	    			printf("Respuesta enviada\n");
	    			terminar_programa(cliente_socket);
	    			break;

	    		case PACKAGE:;
	    			PROCESO* procesoDeserializado = malloc(sizeof(PROCESO));
	    			PROCESO_SERIALIZADO* procesoSerializado = malloc(sizeof(PROCESO_SERIALIZADO));
	    			procesoSerializado = recibir_paquete(cliente_socket);
	    			deserializar_proceso(procesoSerializado,procesoDeserializado);
	    			INSTRUCCION* instruccion = procesoDeserializado->instrucciones;
	    			PCB* pcb = malloc(sizeof(PCB));
	    			pcb = crearPCB(procesoDeserializado, configKernelDatos, cliente_socket);
	    			enviar_pcb(pcb,conexion_Dispatch, false, NINGUNA);
	    			//PCB* pcb_recibido = malloc(sizeof(PCB));
	    			//pcb_recibido = recibir_pcb(conexion_Dispatch);
	    			//mostrar_pcb(pcb_recibido);

	    			while(instruccion->sig != NULL){
	    				printf("--------------------------------------------- \n");

	    				printf("Orden: %d \n", instruccion->orden);
	    				printf("Code Enum: %d \n", instruccion->code);
	    				printf("Cant de parametros: %d \n", instruccion->cant);
	    			for (int i = 0; i < instruccion->cant  ; ++i){
	    				printf("Parametro %d: %d \n", i+1, instruccion->parametros[i]);
	    			}
	    				instruccion = instruccion->sig;
	    			}
	    			terminar_programa(cliente_socket);
	    			liberar_pcb(pcb);
	    			//liberar_pcb(pcb_recibido);
	    			break;

	            case -1:
	                log_error(logger, "Cliente desconectado de %s...", server_name);
	                return;
	            default:
	                log_error(logger, "Algo anduvo mal en el server de %s", server_name);
	                return;
	        }
	     //}


	}

LISTAPROCESOS* ordenarPorMenorRafaga(LISTAPROCESOS* lista)
{
	log_info(logger, "Ordeno por menor rafaga");
	if(lista == NULL) {
		log_error(logger, "NO hay procesos en la lista");
		exit(2);
	}
	if(lista->sig == NULL) {
		return lista;
	}
	LISTAPROCESOS *aux = (LISTAPROCESOS *)malloc(sizeof(LISTAPROCESOS));
	LISTAPROCESOS *res = (LISTAPROCESOS *)malloc(sizeof(LISTAPROCESOS));
   	res = NULL;
   	PCB max;
   	aux = lista;
   	while( lista!=NULL )
   	{
    	max = aux->pcb;

      	while( aux!=NULL )
      	{
        PCB b = aux->pcb;
        if( compararRafagas(max.estimacion_rafaga,b.estimacion_rafaga))
        {
        	max = b;
        }
        aux = aux->sig;
    }
      	agregarProcesoLista(&res,max);
      	lista = eliminarProceso(lista,max);
      	aux = lista;
   	}
   	return res;
}

void agregarProcesoLista(LISTAPROCESOS** listaProceso, PCB pcb){

	LISTAPROCESOS *actual = *listaProceso;

	LISTAPROCESOS *nuevo = (LISTAPROCESOS *)malloc(sizeof(LISTAPROCESOS));

	nuevo->pcb=pcb;

        if (*listaProceso == NULL)
	    {
	        nuevo->sig = NULL;
	        nuevo->cantidadProcesos=0;
	        *listaProceso = nuevo;
	    }
	    else
	    {
	        while (actual->sig != NULL)
	        {
	            actual = actual->sig;
	        }

	        nuevo->sig = actual->sig;

	        actual->sig = nuevo;
	    }
}

//Obtener el primer proceso de la lista
PCB obtenerPrimerProceso(LISTAPROCESOS* listaProcesos){
		LISTAPROCESOS* actual = listaProcesos;
		PCB primerPCB = actual->pcb;
		return primerPCB;
}

LISTAPROCESOS* eliminarPrimerProceso(LISTAPROCESOS* listaProcesos){
	if(listaProcesos != NULL) {
		LISTAPROCESOS* actual = listaProcesos;
		listaProcesos = actual->sig;
		//listaProcesos->cantidadProcesos = actual->cantidadProcesos - 1;
		free(actual);
	}
	return listaProcesos;
}


LISTAPROCESOS* eliminarProceso(LISTAPROCESOS* lista, PCB pcb) {
	LISTAPROCESOS* aux = lista;
	LISTAPROCESOS* ant = NULL;
	while(aux!=NULL && (pcb.pid != aux->pcb.pid)) {
		ant = aux;
		aux=aux->sig;
	}
	if(ant==NULL) {
		lista = aux->sig;
	}
	else {
		ant->sig = aux->sig;
	}
	free(aux);
	return lista;

}

int compararRafagas(float a, float b) {
	return a>b?1:0;
}
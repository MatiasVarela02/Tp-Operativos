/*
 ============================================================================
 Name        : procesador.c
 Author      : theBug
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include "procesador.h"


#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include<commons/config.h>
#include <time.h>

sem_t interrupcionMutex;
bool interrupciones;
int (*TLB) [4];
/* La estructura de la tlb es:*/
/* NUMERO DE PAGINA | MARCO | TIEMPO DE INSERCION | TIEMPO DE ULTIMO USO */


static void ciclo_procesador(void* args){
	log_info(logger, "\033[38;5;225mArranca ciclo procesador !!");

	t_hiloProcesador_args* argumentosHilo = (t_hiloProcesador_args*) args;

	int conexion_dispatch = argumentosHilo->conexion_Dispatch;
	char* reemplazo_tlb = argumentosHilo->reemplazo_tlb;
	int conexion_memoria = argumentosHilo->conexion_memoria;


	PROCESSOR_ARQ* procesador = malloc(sizeof(PROCESSOR_ARQ));

	procesador = argumentosHilo->procesador;
	bool huboInterrupciones = false;
	bool huboSyscall = false;
	clock_t inicio, fin;
	double tiempo_rafaga;

	int cliente_dispatch = esperar_cliente_procesador(conexion_dispatch);

	PCB* pcb_recibido = malloc(sizeof(PCB));
	
	//mostrar_pcb(pcb_recibido);//Ya no queda en null

	//mientras siga conectado al kernel, recibo PCBS
	while(cliente_dispatch>0){
		pcb_recibido = recibir_pcb(cliente_dispatch);
		log_info(logger, "\033[38;5;213m Recibi el pcb: %ld",pcb_recibido->pid);
		
		//mostrar_pcb(pcb_recibido);
		
		inicio = clock();
		
		//procesador->instruction_register = pcb_recibido->cantidadProgramCounter;
		procesador->instruction_executed_register = pcb_recibido->cantInstruccionesEje;
		procesador->nuevoPCB = true;
		procesador->tabla_primer_nivel = pcb_recibido->tabla_paginas;

		while( (!huboInterrupciones) && (!huboSyscall) ){
			fetch(pcb_recibido, procesador);
			log_info(logger, "\n \033[38;5;195mCodigo de instruccion a ejecutar:  ");
			mostrarInstruccion(procesador->program_counter);
			decode_and_fetch_operands(pcb_recibido, procesador, reemplazo_tlb, conexion_memoria);
			execute(procesador, reemplazo_tlb, conexion_memoria);
			huboSyscall = (procesador->tipo_syscall == SYSIO || procesador->tipo_syscall == SYSEXIT);
			sem_wait(&interrupcionMutex);
			huboInterrupciones = interrupciones;
			sem_post(&interrupcionMutex);
		}

		fin = clock();
		tiempo_rafaga = ((double) (fin - inicio)) / CLOCKS_PER_SEC;
	
		log_info(logger, "Actualizo PCB:");
		actualizar_PCB(pcb_recibido, procesador, tiempo_rafaga, huboInterrupciones);
		vaciarTLB (TLB, procesador->filasTLB);
		huboSyscall = false;
		huboInterrupciones = false;	
		procesador->tipo_syscall = NONE;

		sem_wait(&interrupcionMutex);
		interrupciones = 0;
		sem_post(&interrupcionMutex);

		enviar_pcb(pcb_recibido,cliente_dispatch, false , NINGUNA);
		log_info(logger, "Envio el pcb  %li al kernel",pcb_recibido->pid);
	}
	log_info(logger,"Libero pcb !!");

	liberar_pcb(pcb_recibido);
	
	log_info(logger, "Libero conexion !!");

	liberar_conexion(cliente_dispatch);
}

static void atender_interrupciones(void* args){		
	log_info(logger, "\033[38;5;225mArranca atender interrupciones !!");

	t_hiloProcesador_args* argumentosHilo = (t_hiloProcesador_args*) args;

	int conexion_Interrupt = argumentosHilo->conexion_Dispatch;
	int cliente_interrupt = esperar_cliente_procesador(conexion_Interrupt);

	//printf("\n Arranco a escuchar las instrucciones !!\n");
	while(cliente_interrupt >0){
		if(recibirInterrupcion(cliente_interrupt)){
			sem_wait(&interrupcionMutex);
			interrupciones = 1;
			log_info(logger, "Recibi una interrupcion");
			sem_post(&interrupcionMutex);
		}
	}
	printf("\n Libero interrupt !!\n");

	liberar_conexion(cliente_interrupt);
}

CONFIG_MEMORIA* recibirConfigMemoria(int socket) {
	CONFIG_MEMORIA* recibido = malloc(sizeof(CONFIG_MEMORIA));

	recv(socket, &recibido->entradas_por_tabla, sizeof(int), MSG_WAITALL);
	recv(socket, &recibido->tam_pagina, sizeof(int), MSG_WAITALL);

	log_info(logger, "Recibi config memoria!!");

	return recibido;
}

int main(void) {
	PROCESSOR_ARQ* procesador = malloc(sizeof(PROCESSOR_ARQ));
	procesador = inicializarArqProcesador();
	/* 
	char *config_dir = getcwd(NULL, 0);
	config_dir = realloc(config_dir, strlen(config_dir)+strlen("/procesador.config"));
	strcat(config_dir,"/procesador.config");
		t_config* configCPU = config_create(config_dir);
*/
	sem_init(&interrupcionMutex, 0, 1);

	CONFIGURACION_PROCESADOR* configCPUDatos= malloc(sizeof(CONFIGURACION_PROCESADOR));
	t_config* configCPU = config_create("/home/utnso/Escritorio/procesador.config");

	configCPUDatos->entradas_tlb = config_get_int_value(configCPU,"ENTRADAS_TLB");
	configCPUDatos->reemplazo_tlb = config_get_string_value(configCPU,"REEMPLAZO_TLB");
	configCPUDatos->retardo_noop = config_get_int_value(configCPU,"RETARDO_NOOP");
	configCPUDatos->ip_memoria = config_get_string_value(configCPU,"IP_MEMORIA");
	configCPUDatos->puerto_memoria = config_get_string_value(configCPU,"PUERTO_MEMORIA");
	configCPUDatos->puerto_escucha_dispatch = config_get_string_value(configCPU,"PUERTO_ESCUCHA_DISPATCH");
	configCPUDatos->puerto_escucha_interrupt = config_get_string_value(configCPU,"PUERTO_ESCUCHA_INTERRUPT");

	logger = log_create("procesador.log", "Servidor", 1, LOG_LEVEL_DEBUG);

	procesador->tiempo_NOOP = configCPUDatos->retardo_noop;
	char* puerto_cpu_dispatch = configCPUDatos->puerto_escucha_dispatch;
	char* puerto_cpu_interrupt = configCPUDatos->puerto_escucha_interrupt;

	int conexionMemoria = crear_conexion(configCPUDatos->ip_memoria, configCPUDatos->puerto_memoria);

	enviar_texto("Handshake procesador-memoria", conexionMemoria);

	CONFIG_MEMORIA* configMemoria = recibirConfigMemoria(conexionMemoria);
	//printf("Recibi %i\n",configMemoria->tam_pagina);
	//printf("Recibi %i\n",configMemoria->entradas_por_tabla);

	int server_dispatch = iniciar_servidor(IP, puerto_cpu_dispatch);
	int server_Interrupt = iniciar_servidor(IP, puerto_cpu_interrupt);
	log_info(logger, "Servidores listos para recibir al cliente");

	procesador->memoria_tamanio_pagina = configMemoria->tam_pagina;
	procesador->memoria_entradas_por_tabla = configMemoria->entradas_por_tabla;

	TLB = malloc( sizeof(int[4][configCPUDatos->entradas_tlb]) );
	//inicializo la tlb con todos ceros
	vaciarTLB(TLB, configCPUDatos->entradas_tlb);

	procesador->TLB = TLB;
	procesador->filasTLB =  configCPUDatos->entradas_tlb;

	///COMIENZA LA CONEXION PROCESADOR KERNEL

	t_hiloProcesador_args* hiloProcesadorArgs = malloc(sizeof(t_hiloProcesador_args));
	hiloProcesadorArgs->conexion_Dispatch = server_dispatch;
	hiloProcesadorArgs->procesador = procesador;
	hiloProcesadorArgs->reemplazo_tlb = configCPUDatos->reemplazo_tlb;
	hiloProcesadorArgs->conexion_memoria = conexionMemoria;

	t_hiloInterrupt_args* hiloInterruptArgs = malloc(sizeof(t_hiloInterrupt_args));
	hiloInterruptArgs->conexion_Interrupt = server_Interrupt;

	pthread_t hiloProcesador;
	pthread_t hiloInterrupt;

	pthread_create(&hiloProcesador, NULL, (void*) ciclo_procesador, (void*) hiloProcesadorArgs);
	pthread_create(&hiloInterrupt, NULL, (void*) atender_interrupciones, (void*) hiloInterruptArgs);
	
	pthread_join(hiloProcesador,NULL);
	pthread_join(hiloInterrupt, NULL);

	config_destroy(configCPU);
	sem_destroy(&interrupcionMutex);

	return EXIT_SUCCESS;
}


/*
 ============================================================================
 Name        : kernel.c
 Author      : theBug
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "kernel.h"

CONFIGURACION_KERNEL* configKernelDatos;
LISTAPROCESOS* procesosNew;
LISTAPROCESOS* procesosReady;
LISTAPROCESOS* procesosRunning;
LISTAPROCESOS* procesosBlocked;
LISTAPROCESOS* procesosSuspendBlocked;
LISTAPROCESOS* procesosSuspendReady;

int grado_multiprogramacion;

sem_t mutexMultiProgramacion;
sem_t procesosNewMutex;
sem_t procesosRunningMutex;
sem_t procesosReadyMutex;
sem_t procesosBlockedMutex;
sem_t procesosSuspendReadyMutex;
sem_t procesosSuspendBlockedMutex;
sem_t mutexEjecutando;
sem_t gradoMultiProgramacion;
sem_t libreDeEjecutar;
sem_t libreDeIO;
int ejecutando;
int conexion_Dispatch;
int conexion_Interrupt;
int cantidadHilos;
int aEjecutar;

void disminuirGradoMultiprogramacion();
void aumentarGradoMultiprogramacion();
void planificarFifo(void* void_args);
void planificarSRT(void* void_args);
int manejarConexionConCpuFIFO(PCB pcb, int socket_consola, int* seguirEnHilo, int conexionMemoria, CONFIGURACION_KERNEL* configKernelDatos);
void desalojarPCB();
int manejarConexionConCpuSRT(int cliente_socket, int aEjecutar, CONFIGURACION_KERNEL* configKernelDatos, int conexionMemoria);
//PCB* crearPCB(PROCESO* proceso, CONFIGURACION_KERNEL* configKernelDatos);
int kernel_escuchar(t_log* logger, char* server_name, int server_socket,
		int conexion_Dispatch, CONFIGURACION_KERNEL* configKernelDatos, int conexionMemoria);
void enviarTablaPaginasAMemoria(int conexion_memoria, int tabla_paginas, PeticionMemoria cod_op);

void inicializarListas() {
	procesosNew = NULL;
	procesosReady = NULL;
	procesosRunning = NULL;
	procesosBlocked = NULL;
	procesosSuspendBlocked = NULL;
	procesosSuspendReady = NULL;
}

void inicializarSemaforos() {
	sem_init(&procesosSuspendBlockedMutex, 0, 1);
	sem_init(&mutexMultiProgramacion, 0, 1);
	sem_init(&procesosNewMutex, 0, 1);
	sem_init(&procesosReadyMutex, 0, 1);
	sem_init(&procesosRunningMutex, 0, 1);
	sem_init(&procesosBlockedMutex, 0, 1);
	sem_init(&procesosSuspendReadyMutex, 0, 1);
	sem_init(&mutexEjecutando, 0, 1);
	sem_init(&libreDeEjecutar,0,1);
	sem_init(&libreDeIO,0,1);
}

int main(void) {
	inicializarListas();
	inicializarSemaforos();
	ejecutando = 0;
	nro_proceso = 0;
	cantidadHilos=0;
	aEjecutar = 0;

	/*char *config_dir = getcwd(NULL, 0);
	 config_dir = realloc(config_dir,
	 strlen(config_dir) + strlen("/kernel.config"));
	 strcat(config_dir, "/kernel.config");*/

	CONFIGURACION_KERNEL* configKernelDatos = malloc(
			sizeof(CONFIGURACION_KERNEL));
	t_config* configKernel = malloc(sizeof(t_config));
	configKernel = config_create("/home/utnso/Escritorio/kernel.config");

	configKernelDatos->ip_memoria = config_get_string_value(configKernel,
			"IP_MEMORIA");
	configKernelDatos->puerto_memoria = config_get_string_value(configKernel,
			"PUERTO_MEMORIA");
	configKernelDatos->ip_cpu = config_get_string_value(configKernel, "IP_CPU");
	configKernelDatos->puerto_cpu_dispatch = config_get_string_value(
			configKernel, "PUERTO_CPU_DISPATCH");
	configKernelDatos->puerto_cpu_interrupt = config_get_string_value(
			configKernel, "PUERTO_CPU_INTERRUPT");
	configKernelDatos->puerto_escucha = config_get_string_value(configKernel,
			"PUERTO_ESCUCHA");
	configKernelDatos->algoritmo_planificacion = config_get_string_value(
			configKernel, "ALGORITMO_PLANIFICACION");
	configKernelDatos->estimacion_inicial = config_get_int_value(configKernel,
			"ESTIMACION_INICIAL");
	configKernelDatos->alfa = atof(config_get_string_value(configKernel, "ALFA"));
	//configKernelDatos->alfa = config_get_int_value(configKernel, "ALFA");	
	configKernelDatos->grado_multiprogramacion = config_get_int_value(
			configKernel, "GRADO_MULTIPROGRAMACION");
	configKernelDatos->tiempo_maximo_bloqueado = config_get_int_value(
			configKernel, "TIEMPO_MAXIMO_BLOQUEADO");

	char* ip_Dispatch = configKernelDatos->ip_cpu;
	char* puerto_Dispatch = configKernelDatos->puerto_cpu_dispatch;
	char* ip_Interrupt = configKernelDatos->ip_cpu;
	char* puerto_Interrupt = configKernelDatos->puerto_cpu_interrupt;
	grado_multiprogramacion = configKernelDatos->grado_multiprogramacion;
	sem_init(&gradoMultiProgramacion, 0, grado_multiprogramacion);

	//COMIENZA LA CONEXION KERNEL PROCESADOR
	conexion_Dispatch = crear_conexion(ip_Dispatch, puerto_Dispatch);
	conexion_Interrupt = crear_conexion(ip_Interrupt, puerto_Interrupt);
	
	//COMIENZA LA CONEXION KERNEL MEMORIA
	int conexionMemoria = crear_conexion(configKernelDatos->ip_memoria,
			configKernelDatos->puerto_memoria);

	//crea servidor
	int socket_servidor = iniciar_servidor("0.0.0.0",
			configKernelDatos->puerto_escucha);
	enviar_texto("Handshake kernel-memoria", conexionMemoria);

	logger = log_create("log.log", "Kernel", 1, LOG_LEVEL_DEBUG);
	log_info(logger, "Kernel listo para recibir a consola");
	while (kernel_escuchar(logger, "kernel", socket_servidor, conexion_Dispatch,
			configKernelDatos, conexionMemoria) > 0)
		;


	//terminar_programa(socket_cliente);

	config_destroy(configKernel);

	free(configKernelDatos);

	//liberar_pcb(pcb_a_enviar);	//Crear pcb
	liberar_conexion(conexion_Dispatch);
	liberar_conexion(conexion_Interrupt);
	log_destroy(logger);

	sem_destroy(&mutexMultiProgramacion);
	sem_destroy(&procesosNewMutex);
	sem_destroy(&procesosRunningMutex);
	sem_destroy(&procesosReadyMutex);
	sem_destroy(&procesosBlockedMutex);
	sem_destroy(&procesosSuspendReadyMutex);
	sem_destroy(&mutexEjecutando);
	sem_destroy(&procesosSuspendBlockedMutex);

	return EXIT_SUCCESS;
}
void enviarTablaPaginasAMemoria(int conexion_memoria, int tabla_paginas, PeticionMemoria cod_op){

	t_buffer* buffer = malloc(sizeof(t_buffer));

	buffer->size = 2 * sizeof(int);

	void* stream = malloc(buffer->size);
	int offset = 0;

	memcpy(stream + offset,&cod_op, sizeof(PeticionMemoria));
	offset += sizeof(int);
	memcpy(stream + offset,&tabla_paginas, sizeof(long));
	offset += sizeof(long);


	buffer->stream = stream;

	send(conexion_memoria,stream,buffer->size,0);
	free(stream);
}

void disminuirGradoMultiprogramacion() {
	sem_wait(&mutexMultiProgramacion);
	grado_multiprogramacion--; //porque puse a algun pcb en ready
	sem_post(&mutexMultiProgramacion);
}

void aumentarGradoMultiprogramacion() {
	sem_wait(&mutexMultiProgramacion);
	grado_multiprogramacion++; //porque puse a algun pcb en ready
	sem_post(&mutexMultiProgramacion);
}

void planificarFifo(void* void_args) {
	int primeraVez = 1;

	int seguirEnHilo=1;
	// aca tengo que crear el pcb, por cada instanciacion de consola => un nuevo pcb
	t_procesar_conexion_args* args = (t_procesar_conexion_args*) void_args;
	t_log* logger = args->log;
	int cliente_socket = args->fd;
	char* server_name = args->server_name;
	CONFIGURACION_KERNEL* configKernelDatos = malloc(
			sizeof(CONFIGURACION_KERNEL));
	configKernelDatos = args->configKernelDatos;
	int conexionMemoria = args->conexionMemoria;

	free(args);

	op_code cop;

	recv(cliente_socket, &cop, sizeof(op_code), 0);
	printf(" el codigo de operacion es %d \n", cop);

	if (cop != -1) {

		PROCESO* procesoDeserializado = malloc(sizeof(PROCESO));
		PROCESO_SERIALIZADO* procesoSerializado = malloc(
				sizeof(PROCESO_SERIALIZADO));
		procesoSerializado = recibir_paquete(cliente_socket);
		deserializar_proceso(procesoSerializado, procesoDeserializado);
		INSTRUCCION* instruccion = procesoDeserializado->instrucciones;

		PCB pcb = *(crearPCB(procesoDeserializado, configKernelDatos, cliente_socket));
		log_info(logger, "Se creo un PCB");

		aEjecutar++;
		sem_wait(&gradoMultiProgramacion);
		sem_post(&gradoMultiProgramacion);
		if (grado_multiprogramacion > 0) {

			while ( aEjecutar > 0 && seguirEnHilo==1) {
				//log_info(logger, "Grado: %d, aEj: %d, seguiHilo: %d", grado_multiprogramacion, aEjecutar, seguirEnHilo);
				if (primeraVez == 1) {
					primeraVez = 0;
					if (procesosSuspendReady != NULL) {
						sem_wait(&procesosSuspendReadyMutex);
						PCB pcb_nuevo = obtenerPrimerProceso(procesosSuspendReady);
							procesosSuspendReady = eliminarPrimerProceso(procesosSuspendReady);
							sem_post(&procesosSuspendReadyMutex);
							sem_wait(&procesosReadyMutex);
							pcb_nuevo.estado = 1;
							agregarProcesoLista(&procesosReady, pcb_nuevo);
							procesosReady->cantidadProcesos =
									procesosReady->cantidadProcesos + 1;
							sem_wait(&gradoMultiProgramacion);
							disminuirGradoMultiprogramacion();
							sem_post(&procesosReadyMutex);

							// al nuevo lo agrego a la cola de news
							sem_wait(&procesosNewMutex);
							agregarProcesoLista(&procesosNew, pcb);
							procesosNew->cantidadProcesos =
									procesosNew->cantidadProcesos + 1;
							sem_post(&procesosNewMutex);
					} else if (procesosNew == NULL) {

						//log_info(logger, "Enviando PCB a memoria para crear las estructuras necesarias");
						enviar_pcb(&pcb, conexionMemoria, true, CREAR);
						log_info(logger, "Envie el pcb %li a memoria",pcb.pid);
						pcb = *recibir_pcb(conexionMemoria);
						log_info(logger, "Recibi un pcb %li de memoria",pcb.pid);

						if ((pcb.tabla_paginas >= 0)) {

							sem_wait(&procesosReadyMutex);
							pcb.estado = 1;
							agregarProcesoLista(&procesosReady, pcb);
							procesosReady->cantidadProcesos =
									procesosReady->cantidadProcesos + 1;
							sem_wait(&gradoMultiProgramacion);
							disminuirGradoMultiprogramacion();
							sem_post(&procesosReadyMutex);

						} else {
							log_error(logger,"No hay memoria suficiente, el proceso quedara en estado new");
							sem_wait(&procesosNewMutex);
							agregarProcesoLista(&procesosNew, pcb);
							procesosNew->cantidadProcesos =
									procesosNew->cantidadProcesos + 1;
							sem_post(&procesosNewMutex);
						}

					} else if (procesosNew != NULL) {

						//SIGNIFICA Q HAY MEMORIA?
						sem_wait(&procesosNewMutex);

						// al nuevo lo agrego a la cola de news
						agregarProcesoLista(&procesosNew, pcb);
						procesosNew->cantidadProcesos =
								procesosNew->cantidadProcesos + 1;
						// busco el primero de la cola de news para ponerlo en ready

						PCB primer_pbc_new = obtenerPrimerProceso(procesosNew);
						log_warning(logger, "HAY UN PROCESO NUEVO");
									
						procesosNew = eliminarPrimerProceso(procesosNew);

						sem_post(&procesosNewMutex);

						log_info(logger, "Enviando PCB a memoria para crear las estructuras necesarias");
						enviar_pcb(&primer_pbc_new, conexionMemoria, true, CREAR);
						log_info(logger, "Envio el pcb %li a memoria",primer_pbc_new.pid);
						primer_pbc_new = *recibir_pcb(conexionMemoria);
						log_info(logger, "Recibi el pcb %li de memoria",primer_pbc_new.pid);
						if (primer_pbc_new.tabla_paginas > 0) {
							sem_wait(&procesosReadyMutex);
							primer_pbc_new.estado = 1;
							agregarProcesoLista(&procesosReady, primer_pbc_new);
							procesosReady->cantidadProcesos =
								procesosReady->cantidadProcesos + 1;
							sem_wait(&gradoMultiProgramacion);
							disminuirGradoMultiprogramacion();
							sem_post(&procesosReadyMutex);
						} else {
							log_error(logger,"No hay memoria suficiente, el proceso quedara en estado new");
							sem_wait(&procesosNewMutex);
							agregarProcesoLista(&procesosNew, primer_pbc_new);
							procesosNew->cantidadProcesos =
								procesosNew->cantidadProcesos + 1;
							sem_post(&procesosNewMutex);
						}

					}
					if (procesosReady != NULL) {
						sem_wait(&libreDeEjecutar);

						sem_wait(&procesosReadyMutex);
						
						PCB pcbConPrioridad = obtenerPrimerProceso(procesosReady);
						procesosReady = eliminarPrimerProceso(procesosReady);
						
						sem_wait(&mutexEjecutando);
						ejecutando++;
						sem_post(&mutexEjecutando);

						sem_post(&procesosReadyMutex);
						
						log_info(logger,"\033[38;5;205m Mando el pcb %ld a ejecutar\n",pcbConPrioridad.pid);

						
						//log_info(logger, "Entre a este manejar conexion FIFO");
						int terminar = manejarConexionConCpuFIFO(pcbConPrioridad,cliente_socket,&seguirEnHilo, conexionMemoria, configKernelDatos);
						 if(terminar){
							 return;
						 }
					}
					  // sale del primeraVEZ
				} else if (primeraVez == 0) { // no es la primera vez
					//log_info(logger, "Yo loopeo?");
					
					if (procesosSuspendReady != NULL) {
						
						sem_wait(&procesosSuspendReadyMutex);
						PCB pbc_nuevo = obtenerPrimerProceso(procesosSuspendReady);
						procesosSuspendReady = eliminarPrimerProceso(procesosSuspendReady);

						sem_post(&procesosSuspendReadyMutex);
						sem_wait(&procesosReadyMutex);
						pbc_nuevo.estado = 1;
						agregarProcesoLista(&procesosReady, pbc_nuevo);
						log_info(logger, "Agrego Proceso suspendido a ready");
						procesosReady->cantidadProcesos = procesosReady->cantidadProcesos + 1;
						sem_wait(&gradoMultiProgramacion);
						disminuirGradoMultiprogramacion();
						sem_post(&procesosReadyMutex);
					}
					if (procesosNew != NULL) {
						sem_wait(&procesosNewMutex);
						PCB primer_pbc_new = obtenerPrimerProceso(procesosNew);
						//log_warning(logger, "HAY UN PROCESO NUEVO 2");
						procesosNew = eliminarPrimerProceso(procesosNew);
						sem_post(&procesosNewMutex);

						log_info(logger, "Enviando PCB a memoria para crear las estructuras necesarias");
						enviar_pcb(&primer_pbc_new, conexionMemoria, true, CREAR);
						log_info(logger, "Enviado el pcb %ld a memoria",primer_pbc_new.pid);
						primer_pbc_new = *recibir_pcb(conexionMemoria);
						log_info(logger, "Recibi el pcb %ld de memoria",primer_pbc_new.pid);
						
						if (primer_pbc_new.tabla_paginas > 0) {
							sem_wait(&procesosReadyMutex);
							primer_pbc_new.estado = 1;
							agregarProcesoLista(&procesosReady, primer_pbc_new);
							procesosReady->cantidadProcesos = procesosReady->cantidadProcesos + 1;
							sem_wait(&gradoMultiProgramacion);
							disminuirGradoMultiprogramacion();
							sem_post(&procesosReadyMutex);
							//pongo a ejecutar a alguno
						} else {
							log_error(logger,"No hay memoria suficiente, el proceso quedara en estado new");
							sem_wait(&procesosNewMutex);
							agregarProcesoLista(&procesosNew, primer_pbc_new);
							procesosNew->cantidadProcesos = procesosNew->cantidadProcesos + 1;
							sem_post(&procesosNewMutex);
						}

					}
					if (procesosReady != NULL) {
						sem_wait(&libreDeEjecutar);

						log_info(logger,"No hay nadie en la CPU !!");

						sem_wait(&procesosReadyMutex);
						PCB pcbConPrioridad = obtenerPrimerProceso(procesosReady);
						procesosReady = eliminarPrimerProceso(procesosReady);
						
						sem_wait(&mutexEjecutando);
						ejecutando++;
						sem_post(&mutexEjecutando);

						sem_post(&procesosReadyMutex);

						log_info(logger,"\033[38;5;205m Mando el pcb %ld a ejecutar",pcbConPrioridad.pid);


						//log_info(logger, "Entre al otro manejar conexion FIFO");
						int terminar = manejarConexionConCpuFIFO(pcbConPrioridad,cliente_socket,&seguirEnHilo, conexionMemoria, configKernelDatos);
						if(terminar){
							 return;
						 }
						//log_info(logger, "Sali del otro manejar conex FIFO");
					}
					//log_info(logger, "Estoy aca 1")
				
				}
				//log_info(logger, "Estoy aca 2");	
											
			}
			
			 // sale del while
			log_info(logger, "Salgo del while");
			log_info(logger, "Grado: %d",grado_multiprogramacion);
			/*if(grado_multiprogramacion == 0){
				sem_wait(&procesosNewMutex);
				agregarProcesoLista(&procesosNew, pcb);
				procesosNew->cantidadProcesos = procesosNew->cantidadProcesos + 1;
				sem_post(&procesosNewMutex);

			}*/

		} else {
			sem_wait(&procesosNewMutex);
			agregarProcesoLista(&procesosNew, pcb);
			procesosNew->cantidadProcesos = procesosNew->cantidadProcesos + 1;
			sem_post(&procesosNewMutex);
			//log_info(logger, "Estoy aca 3");			
		}
		//terminar_programa(cliente_socket);
		//free(pcb);
	} else {
		log_error(logger, "Error enviando proceso, desconectado de %s...",server_name);
		return;
	}
	log_info(logger, "Termino 1 thread");	
	return;
}

int  manejarConexionConCpuFIFO(PCB pcb_a_ejecutar, int cliente_socket,
		int* seguirEnHilo, int conexionMemoria, CONFIGURACION_KERNEL* configKernelDatos) {

	pcb_a_ejecutar.estado = RUNNING;

	//log_info(logger, "Envio pcb al cpu");

	enviar_pcb(&pcb_a_ejecutar, conexion_Dispatch, false, NINGUNA);
	log_info(logger, "ENVIO PCB %ld A CPU",pcb_a_ejecutar.pid);
	PCB pcb = *(recibir_pcb(conexion_Dispatch));
	log_info(logger, "RECIBO PCB %ld DE CPU",pcb_a_ejecutar.pid);

	log_info(logger, "Recibo el PCB de vuelta con estado: %d", pcb.estado);

	if (pcb.estado == BLOCKED) { // es porque se bloqueo por un E/S y tengo que decidir si suspenderlo o solo bloquearlo
		log_info(logger,"El pcb tiene estado bloquado");
		
		if (pcb.milisegundos_bloqueo > configKernelDatos->tiempo_maximo_bloqueado) { // si el tiempo del E/S es mayor al maximo lo tengo que suspender
			//log_warning(logger, "Queda suspendi2");

			//saco del ejecutando

			sem_wait(&mutexEjecutando);
			ejecutando--;
			sem_post(&mutexEjecutando);
			sem_post(&libreDeEjecutar);

			/*//lo agrego a la cola de suspended blocked
			sem_wait(&procesosSuspendBlockedMutex);
			log_info(logger,"Suspendiendo proceso por exceso de tiempo limite");
			agregarProcesoLista(&procesosSuspendBlocked, pcb);
			procesosSuspendBlocked->cantidadProcesos = procesosSuspendBlocked->cantidadProcesos + 1;
			sem_post(&procesosSuspendBlockedMutex);*/
			
			pcb.estado = BLOCKEDSUSPENDED;

			//aumento grado de multiprogramacion
			sem_post(&gradoMultiProgramacion);
			aumentarGradoMultiprogramacion();

			//mando a dormir a memoria
			sem_wait(&libreDeIO);

			
			enviar_pcb(&pcb, conexionMemoria, true, SUSPENDER);
			log_info(logger, "ENVIO PCB %ld A MEMORIA",pcb.pid);


			//espero a que despierte de memoria
			pcb = *(recibir_pcb(conexionMemoria));
			log_info(logger, "RECIBO PCB %ld DE MEMORIA",pcb.pid);

			sem_post(&libreDeIO);

			//lo saco de suspended blocked

			/*procesosBlocked = eliminarProceso(&procesosSuspendBlocked, pcb);
			procesosSuspendBlocked->cantidadProcesos = procesosSuspendBlocked->cantidadProcesos - 1;*/

			//lo agrego en suspended ready
			sem_wait(&procesosSuspendReadyMutex);
			log_info(logger,"Recibi el proceso %ld y queda en suspended ready", pcb.pid);
			agregarProcesoLista(&procesosSuspendReady, pcb);
			procesosSuspendReady->cantidadProcesos = procesosSuspendReady->cantidadProcesos + 1;
			pcb.estado = SUSPENDEDREADY;
			sem_post(&procesosSuspendReadyMutex);


		} else if (pcb.milisegundos_bloqueo <= configKernelDatos->tiempo_maximo_bloqueado) { // porque se tiene que bloquear un tiempo
			
			//log_info(logger, "Milisegundos bloqueo del pcb: %d", pcb.milisegundos_bloqueo);

			log_info(logger, "\033[38;5;160m PCB %ld BLOQUEADO POR %ld SEGS", pcb.pid, pcb.milisegundos_bloqueo/1000);
			pcb.estado = BLOCKED;
			
			sem_wait(&procesosBlockedMutex);
			agregarProcesoLista(&procesosBlocked, pcb);
			procesosBlocked->cantidadProcesos = procesosBlocked->cantidadProcesos + 1;
			sem_post(&procesosBlockedMutex);
			
			sem_wait(&mutexEjecutando);
			ejecutando--;
			sem_post(&mutexEjecutando);
			sem_post(&libreDeEjecutar);

			sem_wait(&libreDeIO);


			//log_info(logger, "Bloqueado por %d seg",pcb.milisegundos_bloqueo * 1000 / 1000000);
			usleep(pcb.milisegundos_bloqueo * 1000); // lo bloqueo el tiempo en milisegundos que tarda el E/S
			log_info(logger, "\033[38;5;49m PCB %ld DESBLOQUEADO", pcb.pid);

			sem_post(&libreDeIO);


			sem_wait(&procesosBlockedMutex);
			procesosBlocked = eliminarPrimerProceso(procesosBlocked);
			sem_post(&procesosBlockedMutex);
			
			//log_info(logger, "Elimine primer proceso de lista de blocked");

			pcb.milisegundos_bloqueo = 0;
			
			sem_wait(&procesosReadyMutex);
			pcb.estado = READY;
			agregarProcesoLista(&procesosReady, pcb);
			procesosReady->cantidadProcesos = procesosReady->cantidadProcesos + 1;
			sem_post(&procesosReadyMutex);

			//log_info(logger, "Agregue proceso a lista ready");
		}

	} else if (pcb.estado == EXIT_STATUS) { // ES EXIT
		
		//log_info(logger, "Estado exit, pido a Memoria que libere las estructuras");
		enviarTablaPaginasAMemoria(conexionMemoria, pcb.tabla_paginas, BORRAR);
		
		log_info(logger, "Finalizacion de proceso, comunicando con consola...");
		
		sem_wait(&mutexEjecutando);
		ejecutando--;
		sem_post(&mutexEjecutando);
		sem_post(&libreDeEjecutar);
		sem_post(&gradoMultiProgramacion);
		aumentarGradoMultiprogramacion();
		
		enviar_texto("finalice proceso", pcb.socketTerminal);
		
		aEjecutar--;
		if (aEjecutar>0 && cantidadHilos > 1 ){
			*seguirEnHilo=1;
			cantidadHilos--;
		}
		return 1;

	} else {
		log_error(logger, "No se reconocio el estado del pcb");
	}
	return 0;
}

void planificarSRT(void* void_args) {
	int aEjecutar = 0;
	int primeraVez = 1;
	// aca tengo que crear el pcb, por cada instanciacion de consola => un nuevo pcb
	t_procesar_conexion_args* args = (t_procesar_conexion_args*) void_args;
	t_log* logger = args->log;
	int cliente_socket = args->fd;
	char* server_name = args->server_name;
	CONFIGURACION_KERNEL* configKernelDatos = malloc(
			sizeof(CONFIGURACION_KERNEL));
	configKernelDatos = args->configKernelDatos;
	int conexionMemoria = args->conexionMemoria;

	free(args);
	op_code cop;
	recv(cliente_socket, &cop, sizeof(op_code), 0);
	if (cop != -1) {
		PROCESO* procesoDeserializado = malloc(sizeof(PROCESO));
		PROCESO_SERIALIZADO* procesoSerializado = malloc(
				sizeof(PROCESO_SERIALIZADO));
		procesoSerializado = recibir_paquete(cliente_socket);
		deserializar_proceso(procesoSerializado, procesoDeserializado);
		INSTRUCCION* instruccion = procesoDeserializado->instrucciones;

		PCB pcb = *(crearPCB(procesoDeserializado, configKernelDatos, cliente_socket));
		aEjecutar++;
log_info(logger,"el grado de multi es %d", grado_multiprogramacion);
sem_wait(&gradoMultiProgramacion);
sem_post(&gradoMultiProgramacion);
		if (grado_multiprogramacion > 0) {
			while (aEjecutar > 0) {

				if (primeraVez == 1) {
					primeraVez = 0;
					if (procesosSuspendReady != NULL && grado_multiprogramacion > 0 ) {
						sem_wait(&gradoMultiProgramacion);
						disminuirGradoMultiprogramacion();
						sem_wait(&procesosSuspendReadyMutex);

						PCB pcb_nuevo = obtenerPrimerProceso(
								procesosSuspendReady);

							procesosSuspendReady = eliminarPrimerProceso(
									procesosSuspendReady);
							sem_post(&procesosSuspendReadyMutex);

							sem_wait(&procesosReadyMutex);
							pcb_nuevo.estado = READY;
							agregarProcesoLista(&procesosReady, pcb_nuevo);

							procesosReady->cantidadProcesos =
									procesosReady->cantidadProcesos + 1;
							sem_post(&procesosReadyMutex);

							// al nuevo lo agrego a la cola de news
							sem_wait(&procesosNewMutex);
							agregarProcesoLista(&procesosNew, pcb);
							procesosNew->cantidadProcesos =
									procesosNew->cantidadProcesos + 1;
							sem_post(&procesosNewMutex);

							if (ejecutando > 0) {
								log_info(logger, "Se desaloja PCB %ld", pcb.pid);													
								desalojarPCB();
							}
					}
					if (procesosNew == NULL && grado_multiprogramacion > 0 ) { // si no hay procesos en new, quiero que el nuevo proceso este en ready
						disminuirGradoMultiprogramacion();
						sem_wait(&gradoMultiProgramacion);
						enviar_pcb(&pcb, conexionMemoria, true, CREAR);
						log_info(logger, "ENVIO PCB %ld A MEMORIA",pcb.pid);
						pcb = *recibir_pcb(conexionMemoria);
						log_info(logger, "RECIBO PCB %ld DE MEMORIA",pcb.pid);

						if (pcb.tabla_paginas >= 0) {

							//pcb->tabla_paginas=recv memoria;
							sem_wait(&procesosReadyMutex);
							pcb.estado = READY;
							agregarProcesoLista(&procesosReady, pcb);
							procesosReady->cantidadProcesos =
									procesosReady->cantidadProcesos + 1;

							sem_post(&procesosReadyMutex);
							if (ejecutando > 0) {
								log_info(logger, "Se desaloja PCB %ld", pcb.pid);													
								desalojarPCB();
							}

						} else {
							log_error(logger, "no hay memoria suficiente");
							sem_wait(&procesosNewMutex);
							agregarProcesoLista(&procesosNew, pcb);
							procesosNew->cantidadProcesos =
									procesosNew->cantidadProcesos + 1;
							sem_post(&procesosNewMutex);
						}

					}
					if (procesosNew != NULL && grado_multiprogramacion > 0) {
						disminuirGradoMultiprogramacion();
						sem_wait(&gradoMultiProgramacion);
						sem_wait(&procesosNewMutex);

							// al nuevo lo agrego a la cola de news
							agregarProcesoLista(&procesosNew, pcb);
							procesosNew->cantidadProcesos =
									procesosNew->cantidadProcesos + 1;

							// busco el primerp de la cola de news para ponerlo en ready

							PCB primer_pbc_new = obtenerPrimerProceso(
									procesosNew);
							procesosNew = eliminarPrimerProceso(procesosNew);

						sem_post(&procesosNewMutex);
						enviar_pcb(&primer_pbc_new, conexionMemoria, true, CREAR);
						log_info(logger, "ENVIO un PCB A MEMORIA %li",primer_pbc_new.pid);
						primer_pbc_new = *recibir_pcb(conexionMemoria);
						log_info(logger, "RECIBO un PCB DE MEMORIA %li",primer_pbc_new.pid);

						if (primer_pbc_new.tabla_paginas >= 0) {
							sem_wait(&procesosReadyMutex);
							primer_pbc_new.estado = READY;
							agregarProcesoLista(&procesosReady, primer_pbc_new);
							procesosReady->cantidadProcesos =
									procesosReady->cantidadProcesos + 1;

							sem_post(&procesosReadyMutex);
							if (ejecutando > 0) {
								log_info(logger, "Se desaloja PCB %ld", pcb.pid);													
								desalojarPCB();
							}

						} else {
							log_error(logger, "no hay memoria suficiente");
							sem_wait(&procesosNewMutex);
							agregarProcesoLista(&procesosNew, primer_pbc_new);
							procesosNew->cantidadProcesos =
									procesosNew->cantidadProcesos + 1;
							sem_post(&procesosNewMutex);
						}

					}
					if (procesosReady != NULL) {
						sem_wait(&libreDeEjecutar);

						sem_wait(&mutexEjecutando);
						ejecutando++;
						sem_post(&mutexEjecutando);

						int terminar = manejarConexionConCpuSRT(cliente_socket, aEjecutar, configKernelDatos, conexionMemoria);
						if(terminar){
							 return;
						 }
					}

				} else if (primeraVez == 0) { // si no es la primera vez
					if (procesosSuspendReady != NULL && grado_multiprogramacion > 0) {
						sem_wait(&gradoMultiProgramacion);
						disminuirGradoMultiprogramacion();
						sem_wait(&procesosSuspendReadyMutex );
						PCB pcb_nuevo = obtenerPrimerProceso(
								procesosSuspendReady);

							procesosSuspendReady = eliminarPrimerProceso(
									procesosSuspendReady);
							sem_post(&procesosSuspendReadyMutex);

							sem_wait(&procesosReadyMutex);
							pcb_nuevo.estado = READY;
							agregarProcesoLista(&procesosReady, pcb_nuevo);

							procesosReady->cantidadProcesos =
									procesosReady->cantidadProcesos + 1;
							sem_post(&procesosReadyMutex);

							if (ejecutando > 0) {
								log_info(logger, "Se desaloja PCB %ld", pcb.pid);													
								desalojarPCB();
							}
					}

					if (procesosNew != NULL && grado_multiprogramacion > 0) {
						sem_wait(&gradoMultiProgramacion);
						disminuirGradoMultiprogramacion();
						// busco el primerp de la cola de news para ponerlo en ready
						sem_wait(&procesosNewMutex);
							PCB primer_pbc_new = obtenerPrimerProceso(
									procesosNew);
							procesosNew = eliminarPrimerProceso(procesosNew);
						sem_post(&procesosNewMutex);

						enviar_pcb(&primer_pbc_new, conexionMemoria, true, CREAR);
						log_info(logger, "ENVIO un PCB A MEMORIA %li",primer_pbc_new.pid);
						primer_pbc_new = *recibir_pcb(conexionMemoria); //ACA ESTA BIEN PCB??
						log_info(logger, "RECIBO un PCB DE MEMORIA %li",primer_pbc_new.pid);

						if (primer_pbc_new.tabla_paginas >= 0) {
							sem_wait(&procesosReadyMutex);
							primer_pbc_new.estado = READY;
							agregarProcesoLista(&procesosReady, primer_pbc_new);
							procesosReady->cantidadProcesos =
									procesosReady->cantidadProcesos + 1;

							sem_post(&procesosReadyMutex);
							if (ejecutando > 0) {
								log_info(logger, "Se desaloja PCB %ld", pcb.pid);					
								desalojarPCB();
							}
						} else {
							log_error(logger, "no hay memoria suficiente");
						}

					}
					if (procesosReady != NULL) {
						sem_wait(&libreDeEjecutar);

						sem_wait(&mutexEjecutando);
						ejecutando++;
						sem_post(&mutexEjecutando);

						int terminar = manejarConexionConCpuSRT(cliente_socket, aEjecutar, configKernelDatos, conexionMemoria);
						if(terminar){
							 return;
						 }
					}

				}

			} // sale del while

		}

//terminar_programa(cliente_socket);
	}

	else {
		log_error(logger, "error enviando proceso, desconectado de %s...",
				server_name);
		return;
	}
}

void desalojarPCB() {
	//printf("Se desaloja un PCB\n");

	int senial_Desalojo = 1;

	void* senial_a_enviar = malloc(sizeof(int));
	memcpy(senial_a_enviar, &senial_Desalojo, sizeof(int));

	send(conexion_Interrupt, senial_a_enviar, sizeof(int), MSG_WAITALL);
	log_info(logger, "Mande una interrupcion");

	return;
}

int manejarConexionConCpuSRT(int cliente_socket, int aEjecutar, CONFIGURACION_KERNEL* configKernelDatos, int conexionMemoria) {



		sem_wait(&procesosReadyMutex);
		procesosReady = ordenarPorMenorRafaga(procesosReady);
		PCB masCorto = obtenerPrimerProceso(procesosReady);
		procesosReady = eliminarPrimerProceso(procesosReady);
		sem_post(&procesosReadyMutex);

		enviar_pcb(&masCorto, conexion_Dispatch, false, NINGUNA);
		log_info(logger, "ENVIO un PCB A CPU %li",masCorto.pid);

	PCB pcb = *(recibir_pcb(conexion_Dispatch));
	log_info(logger, "RECIBO un PCB DE CPU %li con el estado %d",pcb.pid,pcb.estado );







     if(pcb.estado == READY){
    	 log_info(logger, "llego por desalojo");
    	 sem_wait(&procesosReadyMutex);
    	 				agregarProcesoLista(&procesosReady, pcb);
    	 				procesosReady->cantidadProcesos = procesosReady->cantidadProcesos + 1;
    	 				sem_post(&procesosReadyMutex);
    	 				sem_wait(&mutexEjecutando);
    	 				ejecutando--;
    	 				sem_post(&mutexEjecutando);
    	 				sem_post(&libreDeEjecutar);
     }
     else if (pcb.estado == BLOCKED) { // es porque se bloqueo por un E/S y tengo que decidir si suspenderlo o solo bloquearlo
	 	log_info(logger, "Llego en estado bloqueado con unos milisegundos de: %ld", pcb.milisegundos_bloqueo);
		if (pcb.milisegundos_bloqueo != 0) {
			if (pcb.milisegundos_bloqueo > configKernelDatos->tiempo_maximo_bloqueado) { // si el tiempo del E/S es mayor al maximo lo tengo que suspender
				log_info(logger, "se suspende");
				pcb.estado = BLOCKEDSUSPENDED;

				sem_wait(&mutexEjecutando);
							ejecutando--;
							sem_post(&mutexEjecutando);
							sem_post(&libreDeEjecutar);
				//aumento grado de multiprogramacion
							log_info(logger, "todavia no se suspendio aumenta el grado de multi a   %d",grado_multiprogramacion);
							sem_post(&gradoMultiProgramacion);
						aumentarGradoMultiprogramacion();
						log_info(logger, "Como se suspendio aumenta el grado de multi a   %d",grado_multiprogramacion);


				//mando a dormir a memoria
				sem_wait(&libreDeIO);

				enviar_pcb(&pcb, conexionMemoria, true, SUSPENDER);
				log_info(logger, "ENVIO un PCB A MEMORIA PARA QUE SE DUERMA %ld",pcb.pid);



				//espero a que despierte de memoria
				pcb = *(recibir_pcb(conexionMemoria));
				log_info(logger, "YA DURMIO  %ld",pcb.pid);

				sem_post(&libreDeIO);


				//lo agrego en suspended ready
				sem_wait(&procesosSuspendReadyMutex);
				log_info(logger,"Recibi el proceso y queda en suspended ready");
				agregarProcesoLista(&procesosSuspendReady, pcb);
				procesosSuspendReady->cantidadProcesos =
						procesosSuspendReady->cantidadProcesos + 1;
				pcb.estado = SUSPENDEDREADY;
				sem_post(&procesosSuspendReadyMutex);

			} else if (pcb.milisegundos_bloqueo <= configKernelDatos->tiempo_maximo_bloqueado) {
				// fin del if donde suspendo al proceso
				// porque se tiene que bloquear un tiempo
				log_info(logger,"SE TIENE QUE BLOQUEAR UN TIEMPO");
				sem_wait(&procesosBlockedMutex);
				agregarProcesoLista(&procesosBlocked, pcb);
				procesosBlocked->cantidadProcesos =
						procesosBlocked->cantidadProcesos + 1;
				pcb.estado = 3;
				sem_post(&procesosBlockedMutex);
				sem_wait(&mutexEjecutando);
				ejecutando--;
				sem_post(&mutexEjecutando);
				sem_post(&libreDeEjecutar);

				sem_wait(&libreDeIO);

				usleep(pcb.milisegundos_bloqueo * 1000); // lo bloqueo el tiempo en milisegundos que tarda el E/S
				
				sem_post(&libreDeIO);


				sem_wait(&procesosReadyMutex);
				pcb.estado = READY;
				agregarProcesoLista(&procesosReady, pcb);
				procesosReady->cantidadProcesos =
						procesosReady->cantidadProcesos + 1;
				sem_post(&procesosReadyMutex);
				


				if (ejecutando > 0) {
					log_info(logger, "Se desaloja PCB %ld", pcb.pid);					
					desalojarPCB();
				}
				
			}
		}
	} else if (pcb.estado == EXIT_STATUS) { // ES EXIT
		aEjecutar--;
		enviarTablaPaginasAMemoria(conexionMemoria, pcb.tabla_paginas, BORRAR);
		log_info(logger, "finalizacion de proceso, comunicando con consola...");
		log_info(logger, "el grado de ejecutando es %d",ejecutando);
		sem_wait(&mutexEjecutando);
		ejecutando--;
		sem_post(&mutexEjecutando);
		sem_post(&libreDeEjecutar);
		sem_post(&gradoMultiProgramacion);
		aumentarGradoMultiprogramacion();
		enviar_texto("finalice proceso", pcb.socketTerminal);
		return 1;
	} else {
		printf("\nNo entre a ningun if xd\n");
	}
	return 0;
}

int kernel_escuchar(t_log* logger, char* server_name, int server_socket,
		int conexion_Dispatch, CONFIGURACION_KERNEL* configKernelDatos, int conexionMemoria) {
	int cliente_socket = esperar_cliente(logger, server_name, server_socket);

	log_error(logger, "ITERO");


	if (cliente_socket != -1) {
		cantidadHilos++;
		log_info(logger, "Se conecto una consola!");
		pthread_t hilo;
		t_procesar_conexion_args* args = malloc(
				sizeof(t_procesar_conexion_args));
		args->log = logger;
		args->fd = cliente_socket;
		args->server_name = server_name;
		args->conexion_Dispatch = conexion_Dispatch;
		args->configKernelDatos = configKernelDatos;
		args->conexionMemoria = conexionMemoria;

		if (strcmp(configKernelDatos->algoritmo_planificacion, "FIFO") == 0) {
			pthread_create(&hilo, NULL, (void*) planificarFifo, (void*) args);
			pthread_detach(hilo);
		} else if (strcmp(configKernelDatos->algoritmo_planificacion, "SRT")
				== 0) {
			printf("\n El algoritmo es SRT\n");

			pthread_create(&hilo, NULL, (void*) planificarSRT, (void*) args);
			pthread_detach(hilo);
		} else {
			log_error(logger, "No se reconocio el archivo de configuracion!");
		}
		return 1;
	}
	return 0;
}
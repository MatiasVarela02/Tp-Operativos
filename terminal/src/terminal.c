/*
 ============================================================================
 Name        : terminal.c
 Author      : theBug
 Version     :
 Copyright   : Your copyright notice
 Description : El tp de operativos in C, Ansi-style
 ============================================================================
 */

#include "terminal.h"
#include<commons/log.h>
#include<commons/config.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>

int main(int argc, char *argv[]) {

	char* ipKernel;
	char* puertoKernel;
	logger = log_create("terminal.log", "Terminal", 1, LOG_LEVEL_DEBUG);
	/*char *config_dir = getcwd(NULL, 0);
	config_dir = realloc(config_dir,
			strlen(config_dir) + strlen("/terminal.config"));
	strcat(config_dir, "/terminal.config");*/
	t_config* configConsola = config_create("/home/utnso/Escritorio/terminal.config");

	ipKernel = config_get_string_value(configConsola, "IP_KERNEL");
	puertoKernel = config_get_string_value(configConsola, "PUERTO_KERNEL");

	if (argc == 3) {
		log_info(logger, "Parametros OK");
		//printf("EL path del archivo es %s\n", argv[1]);
		//printf("Y su tamanio es  %d\n", atoi(argv[2]));
	} else if (argc > 3) {
		log_error(logger, "Se recibieron argumentos de mas.");
		log_error(logger, "Por favor enviar solamente archivo de proceso y tamanio en dicho orden.");
		return EXIT_FAILURE;

	} else {
		log_error(logger, "Faltan argumentos.");
		log_error(logger, "Por favor enviar archivo de proceso y tamanio en dicho orden.");
		return EXIT_FAILURE;
	}

	char nombreInstruccion[20];
	int parametros[2];
	int cantidadInstrucciones = 0;
	char *stringseparado;
	INSTRUCCION *listaInstrucciones = NULL;
	int cantParametros;
	int orden = 0;
	FILE* archivo;

	archivo = fopen(argv[1], "r");
	char texto[20];

	if (archivo == NULL) {
		log_error(logger, "El archivo no se pudo abrir.");
		return EXIT_FAILURE;
	}

	do {
		fgets(texto, 20, archivo);

		stringseparado = strtok(texto, " ");

		strcpy(nombreInstruccion, stringseparado); // aca ya tengo el nombre de la instruccion

		if (instruccionValida(nombreInstruccion)) {
			orden = orden + 1;
			stringseparado = strtok(NULL, " ");
			cantParametros = 0;

			if (stringseparado != NULL) { // es porque existe un parametro
				parametros[0] = atoi(stringseparado);
				cantParametros = 1;
				stringseparado = strtok(NULL, " ");

				if (stringseparado != NULL) { // es porque existe otro parametro
					cantParametros = 2;
					parametros[1] = atoi(stringseparado);
				}
			}
			//aca ya tengo cargada la instruccion y los parametros de solo UNA INSTRUCCION
			insertarInstruccion(&listaInstrucciones, nombreInstruccion, parametros, cantParametros, orden);
			cantidadInstrucciones++;
			//aca tengo q enviar los datos de las instrucciones a memoria dinamica
		}

	} while (!feof(archivo));

	fclose(archivo);

	/* Serializacion del proceso parseado por el archivo */
	PROCESO* proceso = malloc(sizeof(PROCESO));
	proceso->cantidadInstrucciones = cantidadInstrucciones;
	proceso->instrucciones = listaInstrucciones;
	proceso->tamanioProceso = atoi(argv[2]);

	INSTRUCCION* instruccion = listaInstrucciones;

	PROCESO_SERIALIZADO* procesoSerializado = inicializar_proceso_serializado(
			proceso->tamanioProceso);
	serializar_proceso(proceso, procesoSerializado);
	PAQUETE* paquete = crear_paquete(procesoSerializado);

	/* Se crea la conexion con el kernel */

	int socket_consola = crear_conexion(ipKernel, puertoKernel);
	/*	 Handshake con kernel

	 char* msj="Hola Kernel";
	 enviar_texto(msj,socket_consola);
	 printf("Envie mensaje a kernel: %s \n", msj);
	 char* rtaKernel = recibir_texto(socket_consola);
	 printf("recibi respuesta del kernel: %s  \n",rtaKernel);
	 */
	enviar_paquete(paquete, socket_consola);
	//printf("\n\nEsperando una respuesta del kernel\n");
	log_info(logger,"\033[38;5;183m  Arranco a esperar");
	// esto es lo que hace el kernel:	enviar_texto("finalizar proceso terminado,cliente_socket);
	char* rtaKernel = recibir_texto(socket_consola);

	//printf("\nMensaje: %s\n\n",rtaKernel);
	if((strcmp(rtaKernel,"finalice proceso")==0)){
		log_info(logger, "\033[38;5;194m  Proceso finalizado correctamente, cerrando conexion con exito");

	}else{
		log_error(logger," Hubo un fallo al finalizar proceso, cerrando conexion sin exito");
	}

	liberar_conexion(socket_consola);

	eliminar_paquete(paquete);

	free(proceso->instrucciones);
	free(proceso);
	free(procesoSerializado);
	config_destroy(configConsola);

	return EXIT_SUCCESS;
}


#ifndef PROCESADOR_H_
#define PROCESADOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include "../../utils/src/utils.h"
#include <pthread.h>



typedef struct configuracion_procesador{
	int entradas_tlb;
	char* reemplazo_tlb;
	int retardo_noop;
	char* ip_memoria;
	char* puerto_memoria;
	char* puerto_escucha_dispatch;
	char* puerto_escucha_interrupt;

}CONFIGURACION_PROCESADOR;

typedef struct{
	t_log* log;
	int fd;
	char* server_name;
	int conexion_Interrupt;
}t_hiloInterrupt_args;

typedef struct{
	t_log* log;
	int fd;
	char* server_name;
	int conexion_Dispatch;
    PROCESSOR_ARQ* procesador;
	char* reemplazo_tlb;
	int conexion_memoria;
}t_hiloProcesador_args;


typedef struct config_memoria{
	int tam_pagina;
	int entradas_por_tabla;
}CONFIG_MEMORIA;

bool noHayInterrupciones;
bool huboSyscall;


#endif /* PROCESADOR_H_ */

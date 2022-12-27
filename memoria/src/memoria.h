
#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include "../../utils/src/utils.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdint.h>

typedef struct configuracion_memoria{
	char* puerto_escucha;
	int tam_memoria;
	int tam_pagina;
	int entradas_por_tabla;
	int retardo_memoria;
	char* algoritmo_reemplazo;
	int marcos_por_proceso;
	int retardo_swap;
	char* path_swap;

}CONFIGURACION_MEMORIA;

typedef struct args_hilos_conexiones{
	int cliente_socket;
	int sever_fd;
}ARGS_HILOS_CONEXIONES;

typedef struct campos_segundo_nivel{
	int marco;
	int presencia;
	int uso;
	int modificado;
	int id_tabla_primer_nivel;
	int posicion_swap;
	t_list* framesAsignados;
}CAMPOS_SEGUNDO_NIVEL;

typedef struct campos_frames_asignados{
	int marco;
	int uso;
	int modificado;
}CAMPO_FRAMES_ASIGNADOS;

typedef struct puntero_clock{
	int tamanio_proceso;
	int index;
	int cantidad_tablas_segundo_nivel;
	int cantidadFramesAsignados;
	int cantidad_de_paginas;
	t_list* framesAsignados;
} PUNTERO_CLOCK;


typedef struct{
	int cliente_socket;
	int numero_pagina_1er_nivel;
	int entrada_tabla_1er_nivel;
	int numero_pagina_2do_nivel;
	int entrada_tabla_2do_nivel;
	int marco;
	int desplazamiento;
	int valor;
	PCB* pcb;
}t_hilosMemoria_args;

#endif /* MEMORIA_H_ */

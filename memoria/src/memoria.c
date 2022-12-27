#include "memoria.h"
#include <sys/mman.h>
#include <stdlib.h>
#include <commons/string.h>
#include <errno.h>

void *espacioMemoria;
int cantidadFramesLibres; //read and write
int cantidadTotalFrames;
int cantTablasPrimerNivel;
int indexFramesLibres = 0;
int marcoAComparar = -1;
int* framesMap;
sem_t framesLibresMutex;
sem_t diccionarioPunterosMutex;
sem_t diccionarioPidsMutex;
sem_t espacioMemoriaMutex;
sem_t swapMutex;
CONFIGURACION_MEMORIA* configMemoriaDatos;
t_list* tablasPrimerNivel;
t_list* tablasSegundoNivel;
t_dictionary* diccionarioDePunteros;
t_dictionary* diccionarioPidPorPagina;
t_dictionary* diccionarioCampoSegundoNivelPorMarco;

int realizarBusquedaClock(t_list *tablaPrimerNivel, int indexTablaPrimerNivel, int indexTablaSegundoNivel,
						   PUNTERO_CLOCK *puntero_clock);
int realizarBusquedaClockModificado(t_list *tablaPrimerNivel, int indexTablaPrimerNivel, int indexTablaSegundoNivel,
									PUNTERO_CLOCK *puntero_clock);
int realizarBusquedaIdeal(t_list *tablaPrimerNivel, int indexTablaPrimerNivel, int indexTablaSegundoNivel,
						  PUNTERO_CLOCK *puntero_clock);
int algoritmoClock(int idTablaSegundoNivel, int numero_pagina_2do_nivel, int indexTablaSegundoNivel, PUNTERO_CLOCK *puntero_clock, int id_tabla_primer_nivel);
int algoritmoClockModificado(int idTablaSegundoNivel, int numero_pagina_2do_nivel, int indexTablaSegundoNivel,
							 PUNTERO_CLOCK *puntero_clock, int id_tabla_primer_nivel);
void reemplazarPagina(CAMPOS_SEGUNDO_NIVEL *pagina, t_list *tablaPrimerNivel, int numero_pagina_2do_nivel, int indexTablaSegundoNivel, int  tamanio_proceso);
void cambiarBitPresencia(PUNTERO_CLOCK *puntero_clock, int numero_tabla_primer_nivel);
void crear_proceso(void* args);
void dar_segunda_tabla(int numero_tabla_1er_nivel, int entrada_tabla_1er_nivel, int cliente_socket);
void eliminar_proceso(void* args);
void suspender_proceso(void* args);
int dar_marco(int cliente_socket, int numero_pagina_2do_nivel, int entrada_tabla_2do_nivel);
void hacer_lectura(int cliente_socket,int  marco, int desplazamiento);
void hacer_escritura(int cliente_socket,int marco,int desplazamiento , int valor);
void eliminarTablasDeUnProceso(PUNTERO_CLOCK *puntero_clock, int numero_pagina_1er_nivel);
void enviarDatosACpu(int entradasPorTabla, int tamanioPagina,int socket_cliente);
void crearDirectorioSwap(char* path);
void cargarProcesoASwap(char* pid, int tam_proceso);
void writePaginaSwap(char* pid,int tam_proceso, int nro_marco,int posicion_swap);
void* readPaginaSwap(char* pid, int tam_proceso, int nro_marco,int posicionSwap);
void liberarProcesoDeMemoria(PCB* pcb);
void escuchar_Kernel(void* args);
void escuchar_Procesador(void* args);
void campoFramesAsignadosDestroyer(void* campo);

int main(void) {
	// char *config_dir = getcwd(NULL, 0);
	// config_dir = realloc(config_dir,
	// 		strlen(config_dir) + strlen("/memoria.config"));
	// strcat(config_dir, "/memoria.config");

	logger = log_create("memoria.log", "Memoria", 1, LOG_LEVEL_DEBUG);

	configMemoriaDatos = malloc(sizeof(CONFIGURACION_MEMORIA));
	t_config* configMemoria = config_create("/home/utnso/Escritorio/memoria.config");

	configMemoriaDatos->puerto_escucha = config_get_string_value(configMemoria,
			"PUERTO_ESCUCHA");
	configMemoriaDatos->tam_memoria = config_get_int_value(configMemoria,
			"TAM_MEMORIA");
	configMemoriaDatos->tam_pagina = config_get_int_value(configMemoria,
			"TAM_PAGINA");
	configMemoriaDatos->entradas_por_tabla = config_get_int_value(configMemoria,
			"ENTRADAS_POR_TABLA");
	configMemoriaDatos->retardo_memoria = config_get_int_value(configMemoria,
			"RETARDO_MEMORIA");
	configMemoriaDatos->algoritmo_reemplazo = config_get_string_value(configMemoria,
			"ALGORITMO_REEMPLAZO");
	configMemoriaDatos->marcos_por_proceso = config_get_int_value(configMemoria,
			"MARCOS_POR_PROCESO");
	configMemoriaDatos->retardo_swap = config_get_int_value(configMemoria,
			"RETARDO_SWAP");
	configMemoriaDatos->path_swap = config_get_string_value(configMemoria,
			"PATH_SWAP");

	sem_init(&framesLibresMutex, 0, 1);
	sem_init(&diccionarioPunterosMutex, 0, 1);
	sem_init(&diccionarioPidsMutex, 0, 1);
	sem_init(&swapMutex, 0, 1);
	sem_init(&espacioMemoriaMutex, 0, 1);

	sem_wait(&swapMutex);
	crearDirectorioSwap(configMemoriaDatos->path_swap);
	sem_post(&swapMutex);


	//printf("el string es: %s \n", configMemoriaDatos->puerto_escucha);
	//printf("el string es: %s \n", configMemoriaDatos->path_swap);


	diccionarioDePunteros = dictionary_create();
	diccionarioPidPorPagina = dictionary_create();
	diccionarioCampoSegundoNivelPorMarco = dictionary_create();

	tablasPrimerNivel = list_create();
	tablasSegundoNivel = list_create();

	char* puertoEscucha = configMemoriaDatos->puerto_escucha;

	int server_fd = iniciar_servidor("0.0.0.0", puertoEscucha);

	log_info(logger, "Servidor listo para recibir al cliente");
	int conexionProcesador = esperar_cliente(logger, "Memoria", server_fd);

	char* handshakeProcesador = recibir_texto(conexionProcesador);

	if(strcmp(handshakeProcesador,"Handshake procesador-memoria") == 0){
		enviarDatosACpu(configMemoriaDatos->entradas_por_tabla,
		configMemoriaDatos->tam_pagina, conexionProcesador);
		log_info(logger, "Envio configuracion a la CPU");
	}else{
		log_error(logger, "No hubo handshake de la CPU");
	}

	//Estructuras de la memoria
	espacioMemoria = malloc(configMemoriaDatos->tam_memoria);
	cantidadFramesLibres = configMemoriaDatos->tam_memoria / configMemoriaDatos->tam_pagina;

	cantidadTotalFrames = cantidadFramesLibres;

	framesMap = (int*)malloc(sizeof(int)*cantidadFramesLibres);

	//un frame libre se representa por tener un valor de 0, todos arrancan vacios
	memset(framesMap, 0, sizeof(int) * cantidadFramesLibres);

	int conexionKernel = esperar_cliente(logger, "Memoria", server_fd);

	char* handshakeKernel = recibir_texto(conexionKernel);

	if(strcmp(handshakeKernel,"Handshake kernel-memoria") == 0){
		log_info(logger, "Recibi el handshake del kernel");
	}else{
		log_error(logger, "No hubo handshake del kernel");
	}

	pthread_t hiloKernel;
	pthread_t hiloProcesador;

	ARGS_HILOS_CONEXIONES* argsHiloKernel = malloc(sizeof(ARGS_HILOS_CONEXIONES));
	ARGS_HILOS_CONEXIONES* argsHiloProcesador = malloc(sizeof(ARGS_HILOS_CONEXIONES));

	argsHiloKernel->cliente_socket = conexionKernel;
	argsHiloProcesador->cliente_socket = conexionProcesador;

	pthread_create(&hiloKernel, NULL, (void*) escuchar_Kernel,
			(void*) argsHiloKernel);
	pthread_create(&hiloProcesador, NULL, (void*) escuchar_Procesador,
			(void*) argsHiloProcesador);

	pthread_join(hiloKernel,NULL);
	pthread_join(hiloProcesador, NULL);

	config_destroy(configMemoria);
	liberar_conexion(server_fd);

	sem_destroy(&framesLibresMutex);
	sem_destroy(&diccionarioPunterosMutex);
	sem_destroy(&diccionarioPidsMutex);
	sem_destroy(&swapMutex);
	sem_destroy(&espacioMemoriaMutex);

	return EXIT_SUCCESS;
}


void escuchar_Kernel(void* args){	
	ARGS_HILOS_CONEXIONES* argumentosHilo = (ARGS_HILOS_CONEXIONES*) args;
	int cliente_socket = argumentosHilo->cliente_socket;	

	log_info(logger, "\033[38;5;111mArranca el hilo de conexion kernel-memoria");
	
	while (cliente_socket > 0) {
		PeticionMemoria cod_op;
		recv(cliente_socket, &cod_op, sizeof(int), MSG_WAITALL);
		pthread_t hilo;
		t_hilosMemoria_args* hiloMemoriaArgs = malloc(sizeof(t_hilosMemoria_args));
		hiloMemoriaArgs->cliente_socket = cliente_socket;

		int numero_pagina_1er_nivel;
		int entrada_tabla_1er_nivel;
		int numero_pagina_2do_nivel;
		int entrada_tabla_2do_nivel;
		int marco;
		int desplazamiento;
		int valor;
		PCB* pcb;

		switch (cod_op) {
		case CREAR:
			pcb = recibir_pcb(cliente_socket);
			log_info(logger, "Recibi un pcb del kernel %li",pcb->pid);
			hiloMemoriaArgs->pcb = pcb;

			pthread_create(&hilo, NULL, (void*) crear_proceso,
			(void*) hiloMemoriaArgs);
			pthread_detach(hilo);
	
			break;
		case SUSPENDER:

			pcb = recibir_pcb(cliente_socket);
			log_info(logger, "Recibi un pcb del kernel %li",pcb->pid);
			
			hiloMemoriaArgs->pcb = pcb;

			pthread_create(&hilo, NULL, (void*) suspender_proceso,
			(void*) hiloMemoriaArgs);
			pthread_detach(hilo);

			break;

		case BORRAR:
			recv(cliente_socket, &numero_pagina_1er_nivel, sizeof(int), MSG_WAITALL);
			
			hiloMemoriaArgs->numero_pagina_1er_nivel = numero_pagina_1er_nivel;

			pthread_create(&hilo, NULL, (void*) eliminar_proceso,
					(void*) hiloMemoriaArgs);
			pthread_detach(hilo);
			break;
		default:
			log_error(logger, "No se reconocio el mensaje del kernel");
			break;
		}
	}
}

void escuchar_Procesador(void* args){	
	ARGS_HILOS_CONEXIONES* argumentosHilo = (ARGS_HILOS_CONEXIONES*) args;
	int cliente_socket = argumentosHilo->cliente_socket;	

	log_info(logger, "\033[38;5;111mArranca el hilo de conexion procesador-memoria");


	while (cliente_socket > 0){
		
		PeticionMemoria cod_op;
		recv(cliente_socket, &cod_op, sizeof(int), MSG_WAITALL);
		int numero_pagina_1er_nivel;
		int entrada_tabla_1er_nivel;
		int numero_pagina_2do_nivel;
		int entrada_tabla_2do_nivel;
		int marco;
		int desplazamiento;
		int valor;
		PCB* pcb;

		switch (cod_op) {

		case SEGUNDATABLA:

			recv(cliente_socket, &numero_pagina_1er_nivel, sizeof(int), MSG_WAITALL);
			recv(cliente_socket, &entrada_tabla_1er_nivel, sizeof(int), MSG_WAITALL);

			dar_segunda_tabla(numero_pagina_1er_nivel, entrada_tabla_1er_nivel, cliente_socket);

			break;
		case MARCO:

			recv(cliente_socket, &numero_pagina_2do_nivel, sizeof(int), MSG_WAITALL);
			recv(cliente_socket, &entrada_tabla_2do_nivel, sizeof(int), MSG_WAITALL);

			dar_marco(cliente_socket, numero_pagina_2do_nivel, entrada_tabla_2do_nivel);

			break;

		case LEER:

			recv(cliente_socket, &marco, sizeof(int), MSG_WAITALL);
			recv(cliente_socket, &desplazamiento, sizeof(int), MSG_WAITALL);

			hacer_lectura(cliente_socket, marco, desplazamiento);

			break;

		case ESCRIBIR:

			recv(cliente_socket, &marco, sizeof(int), MSG_WAITALL);
			recv(cliente_socket, &desplazamiento, sizeof(int), MSG_WAITALL);
			recv(cliente_socket, &valor, sizeof(int), MSG_WAITALL);

			hacer_escritura(cliente_socket, marco, desplazamiento , valor);

			break;

		default:
			log_error(logger, "No se reconocio el mensaje del procesador");
			break;
		}
	}
}


/* ------------------------------ FUNCIONES CLOCK ------------------------------*/

void mostrarAsignacionFrames(t_list* tabla, int entradasTabla){
	int i = 0;

	while(i<entradasTabla){
		CAMPO_FRAMES_ASIGNADOS *campoFrameAsignado = (CAMPO_FRAMES_ASIGNADOS *)list_get(tabla, i);
		
		log_info(logger, " \x1b[32m Frames asignados al proceso: \n Entrada %d, MARCO %d | USO %d | MODIFICADO %d", i, campoFrameAsignado->marco, campoFrameAsignado->uso, campoFrameAsignado->modificado );
		i++;
	}

	
}


int realizarBusquedaClock(t_list *tablaPrimerNivel, int numero_pagina_2do_nivel, int indexTablaSegundoNivel,
						   PUNTERO_CLOCK *puntero_clock){

	log_error(logger, "--------------- CLOCK ------------");

	int termine = 0;
	int marco;
	int i = puntero_clock->index;
	int paginasLiberadas = 0;
	while (i < puntero_clock->cantidadFramesAsignados && termine == 0)	{


		CAMPO_FRAMES_ASIGNADOS *campoFrameAsignado = (CAMPO_FRAMES_ASIGNADOS *)list_get(puntero_clock->framesAsignados, i);
			
		if (campoFrameAsignado->uso == 1){
			campoFrameAsignado->uso = 0;
		}
		else if (campoFrameAsignado->uso == 0)
		{ // tengo que reemplazarla
			log_info(logger, " \033[38;5;159m Encontre pagina para reemplazar");			

			termine = 1;
			
			CAMPOS_SEGUNDO_NIVEL* pagina = (CAMPOS_SEGUNDO_NIVEL*)dictionary_get(diccionarioCampoSegundoNivelPorMarco, string_itoa(campoFrameAsignado->marco));

			//Asigno el marco y actualizo el puntero
			marco = campoFrameAsignado->marco;

			puntero_clock->index = i+1;
			if(puntero_clock->index == puntero_clock->cantidadFramesAsignados){
				puntero_clock->index = 0;
			}	
			
			//reemplazo la pagina en las estructuras de las tablas			
			reemplazarPagina(pagina, tablaPrimerNivel, numero_pagina_2do_nivel, indexTablaSegundoNivel, puntero_clock->tamanio_proceso);
			//me parece q es innecesario pero para q sea uniforme con lo q hace reemplazar pag
			campoFrameAsignado->uso = 1;
			campoFrameAsignado->modificado = 0;

			break; //reemplace y salgo del while encapsulado
		}		
		i++;

		if(i == puntero_clock->cantidadFramesAsignados){
			i = 0;
		}
	}
	return marco;
}

int realizarBusquedaClockModificado(t_list *tablaPrimerNivel, int numero_pagina_2do_nivel, int indexTablaSegundoNivel,
									PUNTERO_CLOCK *puntero_clock)
{	
	log_error(logger, "--------------- CLOCK-M (Con modificado 1) ------------");

	int primeraVez = 1;
	int retorno = -1;
	int i = puntero_clock->index;

	while (i < puntero_clock->cantidadFramesAsignados)
	{
		mostrarAsignacionFrames(puntero_clock->framesAsignados, puntero_clock->cantidadFramesAsignados);

		CAMPO_FRAMES_ASIGNADOS *campoFrameAsignado = (CAMPO_FRAMES_ASIGNADOS *)list_get(puntero_clock->framesAsignados, i);

		if (campoFrameAsignado->uso == 0 && campoFrameAsignado->modificado == 1)
		{ // caso feliz, tengo que reemplazarlo y actualizar puntero
			log_info(logger, " \033[38;5;159m Encontre pagina para reemplazar");			

			CAMPOS_SEGUNDO_NIVEL* pagina = (CAMPOS_SEGUNDO_NIVEL*)dictionary_get(diccionarioCampoSegundoNivelPorMarco, string_itoa(campoFrameAsignado->marco));

			reemplazarPagina(pagina, tablaPrimerNivel, numero_pagina_2do_nivel, indexTablaSegundoNivel, puntero_clock->tamanio_proceso);
			
			campoFrameAsignado->uso = 1;
			campoFrameAsignado->modificado = 0;
			retorno = campoFrameAsignado->marco;
			puntero_clock->index = i+1;	
			if(puntero_clock->index == puntero_clock->cantidadFramesAsignados){
				puntero_clock->index = 0;
			}
			break;
		}
		else if (campoFrameAsignado->uso == 1)
		{ // tengo que actualizar bit uso
			campoFrameAsignado->uso = 0;
		}
		i++;
		if(i == puntero_clock->cantidadFramesAsignados){
			i = 0;
		}
		if (primeraVez != 1 && i == puntero_clock->index)
		{
			break;
		}
		primeraVez = 0;
	}
	return retorno;
}

int realizarBusquedaIdeal(t_list *tablaPrimerNivel, int numero_pagina_2do_nivel, int indexTablaSegundoNivel,
					PUNTERO_CLOCK *puntero_clock){
	int primeraVez = 1;
	int retorno = -1;
	int i = puntero_clock->index;

	log_error(logger, "--------------- CLOCK-M (Con modificado 0) ------------");

	while (i < puntero_clock->cantidadFramesAsignados){			
		mostrarAsignacionFrames(puntero_clock->framesAsignados, puntero_clock->cantidadFramesAsignados);

		CAMPO_FRAMES_ASIGNADOS *campoFrameAsignado = (CAMPO_FRAMES_ASIGNADOS *)list_get(puntero_clock->framesAsignados, i);
		if (campoFrameAsignado->uso == 0 && campoFrameAsignado->modificado == 0)
		{ // caso feliz, tengo que reemplazarlo y actualizar puntero
			log_info(logger, " \033[38;5;159m Encontre pagina para reemplazar");			

			CAMPOS_SEGUNDO_NIVEL* pagina = (CAMPOS_SEGUNDO_NIVEL*)dictionary_get(diccionarioCampoSegundoNivelPorMarco, string_itoa(campoFrameAsignado->marco));
			reemplazarPagina(pagina, tablaPrimerNivel, numero_pagina_2do_nivel, indexTablaSegundoNivel, puntero_clock->tamanio_proceso);
			campoFrameAsignado->uso = 1;
			campoFrameAsignado->modificado = 0;
			retorno = campoFrameAsignado->marco;
			puntero_clock->index = i+1;	
			if(puntero_clock->index == puntero_clock->cantidadFramesAsignados){
				puntero_clock->index = 0;
			}
			break;
		}
		i++;
		if(i == puntero_clock->cantidadFramesAsignados){
			i = 0;
		}
		if (primeraVez != 1 && i == puntero_clock->index)
		{
			break;
		}
		primeraVez = 0;
	}
	return retorno;
}

void reemplazarPagina(CAMPOS_SEGUNDO_NIVEL *pagina, t_list *tablaPrimerNivel, int numero_pagina_2do_nivel, int indexTablaSegundoNivel, int tamanio_proceso){
	log_info(logger, "Arranco a reemplazar pagina");

	//Primero escribo en swap si la pagina a sacar se habia modificado
	pagina->presencia = 0;
	log_error(logger, "Le saque presencia a la pag q estaba en el marco:%d", pagina->marco);

	if(pagina->modificado){
		long* pidProcesoAReemplazar = (long*)dictionary_get(diccionarioPidPorPagina, string_itoa(pagina->id_tabla_primer_nivel)  );

		char* stringPidAReemplazar = string_from_format("%ld",(long)pidProcesoAReemplazar);

		sem_wait(&swapMutex);
		writePaginaSwap(stringPidAReemplazar,tamanio_proceso ,pagina->marco,pagina->posicion_swap);
		sem_post(&swapMutex);
	}

	t_list *tablaSegundoNivel = list_get(tablasSegundoNivel, numero_pagina_2do_nivel);
	CAMPOS_SEGUNDO_NIVEL *reemplazopagina = list_get(tablaSegundoNivel, indexTablaSegundoNivel);

	//Leo la pagina con la que voy a reemplazar y la cargo en el marco en el espacio de memoria
	long* pid = (long*)dictionary_get(diccionarioPidPorPagina, string_itoa(reemplazopagina->id_tabla_primer_nivel)  );

	char* stringPid = string_from_format("%ld",(long)pid);
	sem_wait(&swapMutex);
	readPaginaSwap(stringPid, tamanio_proceso, pagina->marco, reemplazopagina->posicion_swap);
	sem_post(&swapMutex);
	reemplazopagina->presencia = 1;
	reemplazopagina->uso = 1;
	reemplazopagina->modificado = 0;
	reemplazopagina->marco = pagina->marco;
	dictionary_put(diccionarioCampoSegundoNivelPorMarco, string_itoa(pagina->marco), (void*) reemplazopagina);

	//no es necesario pero para no dejar basura, reseteo los campos
	pagina->uso = 0;
	pagina->modificado = 0;
	log_info(logger, "Termine de reemplazar pagina");
}

int algoritmoClock(int idTablaSegundoNivel, int numero_pagina_2do_nivel, int indexTablaSegundoNivel, PUNTERO_CLOCK *puntero_clock, int id_tabla_primer_nivel)
{
	t_list *tablaPrimerNivel = list_get(tablasPrimerNivel, id_tabla_primer_nivel);
	int marco = realizarBusquedaClock(tablaPrimerNivel, numero_pagina_2do_nivel, indexTablaSegundoNivel, puntero_clock);

	return marco;
}

int algoritmoClockModificado(int idTablaSegundoNivel, int numero_pagina_2do_nivel, int indexTablaSegundoNivel,
							 PUNTERO_CLOCK *puntero_clock, int id_tabla_primer_nivel)
{

	t_list *tablaPrimerNivel = list_get(tablasPrimerNivel, id_tabla_primer_nivel);

	int resultadoIdeal = realizarBusquedaIdeal(tablaPrimerNivel, numero_pagina_2do_nivel, indexTablaSegundoNivel, puntero_clock);

	if (resultadoIdeal >= 0)
	{
		return resultadoIdeal;
	}

	int resultado = realizarBusquedaClockModificado(tablaPrimerNivel, numero_pagina_2do_nivel, indexTablaSegundoNivel,
													puntero_clock);

	if (resultado >= 0)
	{
		return resultado;
	}

	int resultadoIdeal2 = realizarBusquedaIdeal(tablaPrimerNivel, numero_pagina_2do_nivel, indexTablaSegundoNivel, puntero_clock);

	if (resultadoIdeal2 >= 0)
	{
		return resultadoIdeal2;
	}

	int resultado2 = realizarBusquedaClockModificado(tablaPrimerNivel, numero_pagina_2do_nivel, indexTablaSegundoNivel,
													 puntero_clock);

	if (resultado2 >= 0)
	{
		return resultado2;
	} else
	{
		log_error(logger, "Hubo un error, retorno -1");
		return -1; // hubo un error
	}
}

/* ------------------------------ FUNCIONES SWAP ------------------------------*/

void crearDirectorioSwap(char* path){

	errno = 0;
	int ret = mkdir(path,S_IRWXU);
	if(ret == -1){
		if(errno == EEXIST){
			return;
		}
		else{
			log_error(logger, "No se pudo crear el directorio");
			exit(EXIT_FAILURE);
		}
	}
}

char* crearPathProcesoSwap(char* pid, char* pathSwap) {

	char*pathProceso = string_new();

	string_append(&pathProceso, pathSwap);
	string_append(&pathProceso, "/");
	string_append(&pathProceso, pid);
	string_append(&pathProceso, ".swap");

	return pathProceso;
}

void cargarProcesoASwap(char* pid, int tam_proceso){

	size_t filesize = tam_proceso;
	void *stream;

	char* nombreArchivo = crearPathProcesoSwap( pid,configMemoriaDatos->path_swap);

	int fd = open(nombreArchivo, O_CREAT | O_RDWR, S_IRWXU);

	if (fd == -1)
	{
		perror("Error al abrir archivo:");
		exit(1);
	}

	ftruncate(fd, filesize);

	stream = (void *)mmap(NULL, filesize, PROT_READ | PROT_WRITE,
						  MAP_SHARED, fd, 0);

	munmap(stream, filesize);

	close(fd);

	log_info(logger,"\033[38;5;122m Archivo creado!");
}

void writePaginaSwap(char* pid,int tam_proceso, int nro_marco,int posicion_swap){

	log_info(logger,"\033[38;5;176m Arranco escritura en swap");

	int fd;
	size_t filesize = tam_proceso;
	void *stream;

	int desplazamientoMemoria = nro_marco * configMemoriaDatos->tam_pagina;	
	void*pagina = malloc(configMemoriaDatos->tam_pagina);

	sem_wait(&espacioMemoriaMutex);
	memcpy(pagina, espacioMemoria + desplazamientoMemoria, configMemoriaDatos->tam_pagina);
	sem_post(&espacioMemoriaMutex);
	char *nombreArchivo = crearPathProcesoSwap(pid, configMemoriaDatos->path_swap);

	//log_info(logger,"Voy a abrir el archivo");

	fd = open(nombreArchivo, O_RDWR);

	if (fd == -1)
	{
		perror("Error al abrir archivo:\n");
		exit(1);
	}

	stream = (void *)mmap(NULL, filesize, PROT_WRITE, MAP_SHARED, fd, 0);

	if(stream == MAP_FAILED){
    	log_error(logger,"Mapping Failed");
	}

	int desplazamiento = posicion_swap * configMemoriaDatos->tam_pagina;

	sleep(configMemoriaDatos->retardo_swap / 1000);

	memcpy(stream + desplazamiento, pagina, configMemoriaDatos->tam_pagina);

	msync(stream, filesize, MS_SYNC);

	munmap(stream, filesize);

	close(fd);

	log_info(logger, "\033[38;5;176m Escritura en swap exitosa!");
}

void* readPaginaSwap(char* pid, int tam_proceso, int nro_marco,int posicionSwap){
	log_info(logger,"\033[38;5;176m Arranco lectura en swap");

	int fd;
	size_t filesize = tam_proceso;
	void *stream;
	void *buffer = malloc(configMemoriaDatos->tam_pagina);

	char *nombreArchivo = crearPathProcesoSwap(pid, configMemoriaDatos->path_swap);

	fd = open(nombreArchivo, O_RDWR);

	if (fd == -1)
	{
		perror("Error al abrir archivo:");
		exit(1);
	}

	stream = (void *)mmap ( NULL, filesize,
                     PROT_WRITE,
                     MAP_PRIVATE,
                     fd, 0 );

	if(stream == MAP_FAILED){
    	log_error(logger, "Mapping Failed");
	}

	int desplazamiento = posicionSwap * configMemoriaDatos->tam_pagina;

	sleep(configMemoriaDatos->retardo_swap / 1000);

	memcpy(buffer, stream + desplazamiento, configMemoriaDatos->tam_pagina);

	munmap(stream, filesize);
	close(fd);


	log_info(logger,"\033[38;5;176m Lectura en swap exitosa!");

	log_info(logger,"\033[38;5;189m Ahora escribo la pagina en memoria");

	int desplazamientoMemoria = nro_marco * configMemoriaDatos->tam_pagina;	
	sem_wait(&espacioMemoriaMutex);
	memcpy(espacioMemoria + desplazamientoMemoria, buffer, configMemoriaDatos->tam_pagina);
	sem_post(&espacioMemoriaMutex);

	log_info(logger,"\033[38;5;189m La pagina se escribio exitosamente!");

	return buffer;
}

void eliminarArchivoSwap(char* pid, CONFIGURACION_MEMORIA* configMemoriaDatos){

	int fd;

	char *nombreArchivo = crearPathProcesoSwap(pid, configMemoriaDatos->path_swap);

	fd = open(nombreArchivo, O_RDWR);

	if (fd == -1)
	{
		perror("Error al abrir archivo:");
		exit(1);
	}

	if (remove(nombreArchivo) == 0) // Eliminamos el archivo
		log_info(logger, "\033[38;5;123m El archivo de swap fue eliminado satisfactoriamente");
	else
		log_error(logger, "No se pudo eliminar el archivo");
}

/* ------------------------------ FUNCIONES ACCESO A MEMORIA ------------------------------*/

void escribirMemoria(CONFIGURACION_MEMORIA *configMemoriaDatos, int marco, int desplazamiento, int valor)
{
	int offset = 0;

	offset = marco * configMemoriaDatos->tam_pagina;
	offset += desplazamiento;

	log_info(logger, "\033[38;5;105m Debo escribir en el marco %d con offset: %d el valor %d", marco, offset, valor);

	sem_wait(&espacioMemoriaMutex);
	memcpy(espacioMemoria + offset, &valor, sizeof(int));
	sem_post(&espacioMemoriaMutex);
}

int leerMemoria(CONFIGURACION_MEMORIA *configMemoriaDatos, int marco, int desplazamiento)
{

	int offset = 0;
	int valor = 0;

	offset = marco * configMemoriaDatos->tam_pagina;
	offset += desplazamiento;

	log_info(logger, "\033[38;5;105m Debo leer en el marco %d con offset: %d ", marco, offset);


	sem_wait(&espacioMemoriaMutex);
	memcpy(&valor, espacioMemoria + offset, sizeof(int));
	sem_post(&espacioMemoriaMutex);

	return valor;
}

/* ------------------------------ FUNCIONES ADMINISTRACION ------------------------------*/

void enviarDatosACpu(int entradasPorTabla, int tamanioPagina,
					 int socket_cliente)
{
	t_buffer *buffer = malloc(sizeof(t_buffer));

	buffer->size = sizeof(int) * 2;

	void *stream = malloc(buffer->size);
	int offset = 0;

	memcpy(stream + offset, &entradasPorTabla, sizeof(int));
	offset += sizeof(int);
	memcpy(stream + offset, &tamanioPagina, sizeof(int));
	offset += sizeof(int);

	buffer->stream = stream;

	send(socket_cliente, stream, buffer->size, 0);
	free(stream);
}

int crearTablas(int cantPaginas, t_list* tablaDeFrames) {
    int cantTablasSegundoNivel = ceil(cantPaginas/configMemoriaDatos->entradas_por_tabla);

	log_info(logger, "Tengo %d paginas y por ende creo %d tablas de segundo nivel", cantPaginas, cantTablasSegundoNivel);	

    //Semaforo HAY Q AHREHAR SEMAFOROS DE TABLAS?
	t_list* tPrimerNivel = list_create();
	int entradas2NCreadas = 0;
	int posicionSwap = 0;
	for(int i = 0;i<cantTablasSegundoNivel; i++) {

		t_list* tSegundoNivel = list_create();
		for(int i = 0; i<configMemoriaDatos->entradas_por_tabla && entradas2NCreadas < cantPaginas; i++) {

			CAMPOS_SEGUNDO_NIVEL* tabla = malloc(sizeof(CAMPOS_SEGUNDO_NIVEL));
			tabla->marco = -1;
			tabla->presencia = 0;
			tabla->uso = 0;
			tabla->modificado = 0;
			tabla->id_tabla_primer_nivel = cantTablasPrimerNivel;
			tabla->posicion_swap = posicionSwap;
			tabla->framesAsignados = tablaDeFrames;

			list_add(tSegundoNivel, (void*)tabla);
			entradas2NCreadas++;
			posicionSwap++;
		}
		int indexSegundoNivel = list_add(tablasSegundoNivel, (void*)tSegundoNivel);

		list_add(tPrimerNivel, (void*)indexSegundoNivel);
	}

	return list_add(tablasPrimerNivel, (void*)tPrimerNivel);
    //Semaforo
}

void cambiarBitPresencia(PUNTERO_CLOCK *puntero_clock, int numero_tabla_primer_nivel){

	//Borro del final de la lista al principio para que no se corrmpa al eliminar el item
	//Se borran todos los elementos de la lista, pero no la lista en si
	int i = puntero_clock->cantidadFramesAsignados - 1;

	while ( 0 <= i ){

		CAMPO_FRAMES_ASIGNADOS *campoFrameAsignado = (CAMPO_FRAMES_ASIGNADOS *)list_get(puntero_clock->framesAsignados, i);
			
		CAMPOS_SEGUNDO_NIVEL* c_segundo_nivel = (CAMPOS_SEGUNDO_NIVEL*)dictionary_get(diccionarioCampoSegundoNivelPorMarco, string_itoa(campoFrameAsignado->marco));
		
		c_segundo_nivel->presencia = 0;

		//por las dudas para q no quede basura, limpio el campo
		c_segundo_nivel->modificado = 0;
		c_segundo_nivel->uso = 0;

		//Libero el marco en las estructuras relacionadas con el mismo
		*(framesMap + campoFrameAsignado->marco) = 0;
		cantidadFramesLibres += 1;
		dictionary_remove(diccionarioCampoSegundoNivelPorMarco, string_itoa(campoFrameAsignado->marco));
		//aca destruyo el elemento para que al desuspender el proceso, esta estructura este vacia
		list_remove_and_destroy_element(puntero_clock->framesAsignados, i, campoFramesAsignadosDestroyer);

		i--;
		puntero_clock->cantidadFramesAsignados = 0;
	}
	//CREO Q ACA SI HAY QUE HACER UN FREE DE puntero_clock->framesAsignados
}

void campoFramesAsignadosDestroyer(void* campo){
	CAMPO_FRAMES_ASIGNADOS* castCampo = (CAMPO_FRAMES_ASIGNADOS*)campo;
	free(castCampo);
}

void liberarProcesoDeMemoria(PCB* pcb){

	sem_wait(&diccionarioPunterosMutex);
	PUNTERO_CLOCK* puntero_clock = dictionary_get(diccionarioDePunteros, string_itoa(pcb->tabla_paginas));
	sem_post(&diccionarioPunterosMutex);

	//Borro del final de la lista al principio para que no se corrmpa al eliminar el item
	//Se borran todos los elementos de la lista, pero no la lista en si
	int i = puntero_clock->cantidadFramesAsignados-1;

	char* pid_string = string_itoa(pcb->pid);

	while ( 0 <= i ){

		CAMPO_FRAMES_ASIGNADOS *campoFrameAsignado = (CAMPO_FRAMES_ASIGNADOS *)list_get(puntero_clock->framesAsignados, i);
			
		CAMPOS_SEGUNDO_NIVEL* c_segundo_nivel = (CAMPOS_SEGUNDO_NIVEL*)dictionary_get(diccionarioCampoSegundoNivelPorMarco, string_itoa(campoFrameAsignado->marco));

		if(campoFrameAsignado->modificado == 1){
			//escribo en swap la pagina modificada
			log_info(logger, "\033[38;5;195m ESCRIBO PAGINA %d EN SWAP PQ FUE MOD", c_segundo_nivel->marco);

			sem_wait(&swapMutex);
			writePaginaSwap(pid_string,pcb->tamanio,c_segundo_nivel->marco,c_segundo_nivel->posicion_swap);
			sem_post(&swapMutex);
		}
		//por las dudas para q no quede basura, limpio el campo
		c_segundo_nivel->modificado = 0;
		c_segundo_nivel->uso = 0;
		c_segundo_nivel->presencia = 0;

		//Libero el marco en las estructuras relacionadas con el mismo
		*(framesMap + campoFrameAsignado->marco) = 0;
		cantidadFramesLibres += 1;
		dictionary_remove(diccionarioCampoSegundoNivelPorMarco, string_itoa(campoFrameAsignado->marco));
		//aca destruyo el elemento para que al desuspender el proceso, esta estructura este vacia
		list_remove_and_destroy_element(puntero_clock->framesAsignados, i, campoFramesAsignadosDestroyer);

		i--;
		puntero_clock->cantidadFramesAsignados = 0;
	}

}

/* ------------------------------ FUNCIONES PRINCIPALES ------------------------------*/


void crear_proceso(void* args) {
	log_info(logger, "\033[38;5;121m Recibi un pcb y voy a crear un proceso");

	t_hilosMemoria_args* argumentosHilo = (t_hilosMemoria_args*) args;
	int cliente_socket = argumentosHilo->cliente_socket;
	PCB* pcb = argumentosHilo->pcb;
	double cantPaginasDouble = (double)pcb->tamanio/configMemoriaDatos->tam_pagina;

	//si tengo espacio para guardar el proceso, lo acepto y creo sus estructuras
	int cantPaginas = (int)ceil(cantPaginasDouble);
	int tam_max_frames_proceso = configMemoriaDatos->marcos_por_proceso > cantPaginas ? cantPaginas : configMemoriaDatos->marcos_por_proceso;

	if (cantidadFramesLibres >= tam_max_frames_proceso) {

		//CREO LAS TABLAS DE PAGINAS DE PRIMER Y SEGUNDO NIVEL (el id de la primera tabla hay que mandarlo al kernel)

		t_list* tablaDeFrames = list_create();

		int idPrimeraTabla = crearTablas(cantPaginas, tablaDeFrames);
		cantTablasPrimerNivel++;

		pcb->tabla_paginas = idPrimeraTabla;

		//CREO EL PUNTERO PARA CUANDO SE NECESITE USAR EL CLOCK
		PUNTERO_CLOCK *puntero = malloc(sizeof(PUNTERO_CLOCK));
		puntero->cantidadFramesAsignados = 0;
		int cantTablasSegundoNivel = ceil(cantPaginas / configMemoriaDatos->entradas_por_tabla);
		puntero->index = 0; //ahora el puntero busca en una sola lista, no necesita 2 indexes
		puntero->cantidad_tablas_segundo_nivel = cantTablasSegundoNivel;
		puntero->tamanio_proceso = pcb->tamanio;
		puntero->cantidad_de_paginas = cantPaginas;
		puntero->framesAsignados = tablaDeFrames; 

		char *idTablaPrimerNivel = string_itoa(idPrimeraTabla);

		sem_wait(&diccionarioPunterosMutex);
		dictionary_put(diccionarioDePunteros, idTablaPrimerNivel, (void*) puntero);
		sem_post(&diccionarioPunterosMutex);

		sem_wait(&diccionarioPidsMutex);
		dictionary_put(diccionarioPidPorPagina, idTablaPrimerNivel, (void*) pcb->pid);
		sem_post(&diccionarioPidsMutex);

		sem_wait(&swapMutex);
		cargarProcesoASwap(string_itoa(pcb->pid), pcb->tamanio);
		sem_post(&swapMutex);

		log_info(logger, "\033[38;5;121m Se creo todo exitosamente, envio el pcb de vuelta al kernel ");

		enviar_pcb(pcb, cliente_socket, false, NINGUNA);
		log_info(logger, "Envio el pcb %li al kernel",pcb->pid);

		free(argumentosHilo);

	} else {
		log_error(logger, "No hay espacio para alocar el proceso, me rompo.");
		//return EXIT_FAILURE;
		exit(1);
	}
}

void suspender_proceso(void* args) {
	t_hilosMemoria_args* argumentosHilo = (t_hilosMemoria_args*) args;
	int cliente_socket = argumentosHilo->cliente_socket;
	PCB* pcb = argumentosHilo->pcb;

	log_info(logger, "\033[38;5;159m Tengo que suspender al proceso %ld", pcb->pid);

	//Por cada pagina con bit de presencia en 1, la libera de memoria	

	//Por cada pagina con bit de modificado en 1, la escribe en swap
	liberarProcesoDeMemoria(pcb);

	log_info(logger, "\033[38;5;159m Proceso suspendido por %ld seg", pcb->milisegundos_bloqueo / 1000);
	sleep(pcb->milisegundos_bloqueo / 1000);

	log_info(logger, "\033[38;5;159m Fin de la suspension");

	enviar_pcb(pcb, cliente_socket, false, NINGUNA);
	log_info(logger, "ENVIO un PCB A KERNEL %li",pcb->pid);
}

void dar_segunda_tabla(int numero_tabla_1er_nivel, int entrada_tabla_1er_nivel, int cliente_socket) {

	log_info(logger, "Me pidieron el index de la segunda tabla de paginas:");
	log_info(logger, "El numero de la tabla es: %d y la entrada es %d ", numero_tabla_1er_nivel,entrada_tabla_1er_nivel );

	//Busco la tabla de paginas del proceso
	t_list* tabla_paginas_primer_nivel = (t_list*)list_get(tablasPrimerNivel, numero_tabla_1er_nivel);
	
	int* index_tabla_segundo_nivel = (int*)list_get(tabla_paginas_primer_nivel, entrada_tabla_1er_nivel);

	send(cliente_socket, &index_tabla_segundo_nivel, sizeof(int), 0);
}

int asignarMarcoLibre(CAMPOS_SEGUNDO_NIVEL* campo_segundo_nivel, int tamanio_proceso){
	int esFrameLibre = 0;
	log_info(logger, "Busco un frame libre");

	for(int i = indexFramesLibres ; i < cantidadFramesLibres ; i++){
		if(*(framesMap+i) == 0){
			log_info(logger, "Encontre libre el frame %d", i);

			//Al encontrar, guardo el marco en el campo, le doy presencia
			// y lo marco como ocupado en framesMap
			campo_segundo_nivel->marco = i;
			campo_segundo_nivel->presencia = 1;
			*(framesMap + i) = 1;

			cantidadFramesLibres -= 1;

			long* pid = (long*)dictionary_get(diccionarioPidPorPagina, string_itoa(campo_segundo_nivel->id_tabla_primer_nivel)  );
			char* stringPid = string_from_format("%ld",(long)pid);

			sem_wait(&swapMutex);
			readPaginaSwap(stringPid, tamanio_proceso, i, campo_segundo_nivel->posicion_swap);
			sem_post(&swapMutex);

			dictionary_put(diccionarioCampoSegundoNivelPorMarco, string_itoa(i), (void*)campo_segundo_nivel);
			indexFramesLibres = i + 1;
			log_info(logger, "Termino de asignar el marco %d", i);

			//VER CUANDO HAY QUE ACTUALIZAR EL DICCIONARIO DE FRAMES

			return i;
		}

		if(i == (cantidadTotalFrames-1)){ //ESTA BIEN -1 ??
			i = 0;
		}
	}
}

bool compararMarcos(void* campo1, void* campo2){
	CAMPO_FRAMES_ASIGNADOS* castCampo1 = (CAMPO_FRAMES_ASIGNADOS*)campo1;
	CAMPO_FRAMES_ASIGNADOS* castCampo2 = (CAMPO_FRAMES_ASIGNADOS*)campo2;

	return castCampo2->marco > castCampo1->marco;
}

int dar_marco(int cliente_socket, int numero_pagina_2do_nivel, int entrada_tabla_2do_nivel) {
	int marco;

	log_info(logger,"Se pidio el marco de una pagina");

	t_list* tabla_paginas_segundo_nivel = (t_list*)list_get(tablasSegundoNivel, numero_pagina_2do_nivel);

	CAMPOS_SEGUNDO_NIVEL* campo_segundo_nivel = (CAMPOS_SEGUNDO_NIVEL*)list_get(tabla_paginas_segundo_nivel, entrada_tabla_2do_nivel);
	if(campo_segundo_nivel->presencia == 1){
		log_info(logger,"\033[38;5;121m El marco ya esta en memoria!");

		marco = campo_segundo_nivel->marco;

	}else {
		log_info(logger,"\033[38;5;229m El marco no esta en memoria");

		sem_wait(&diccionarioPunterosMutex);
		PUNTERO_CLOCK* puntero = dictionary_get(diccionarioDePunteros, string_itoa(campo_segundo_nivel->id_tabla_primer_nivel));
		sem_post(&diccionarioPunterosMutex);

		if(puntero->cantidadFramesAsignados < configMemoriaDatos->marcos_por_proceso){
			log_info(logger,"Asigno un marco libre");

			marco = asignarMarcoLibre(campo_segundo_nivel, puntero->tamanio_proceso );

			
			log_info(logger,"ASIGNE MARCO LIBRE %d para la tabla %d entrada %d", marco, numero_pagina_2do_nivel, entrada_tabla_2do_nivel);

			CAMPO_FRAMES_ASIGNADOS* campo = malloc(sizeof(CAMPO_FRAMES_ASIGNADOS));

			campo->marco = marco;
			//estos dos campos se modifican en la instr proxima q es read o write
			campo->uso = 0;
			campo->modificado = 0;

			list_add_sorted(puntero->framesAsignados, (void*) campo, compararMarcos);

			puntero->cantidadFramesAsignados += 1;
		}
		else{ //ya se asignaron todos los frames disponibles, aplico algoritmo reemplazo
		
			if(strcmp(configMemoriaDatos->algoritmo_reemplazo, "CLOCK")==0){
				log_info(logger,"No hay marcos libres, reemplazo por clock");

				marco = algoritmoClock(numero_pagina_2do_nivel,numero_pagina_2do_nivel,entrada_tabla_2do_nivel, puntero, campo_segundo_nivel->id_tabla_primer_nivel);

			}else if(strcmp(configMemoriaDatos->algoritmo_reemplazo, "CLOCK-M")==0){
				log_info(logger,"No hay marcos libres, reemplazo por clock modificado");

				marco = algoritmoClockModificado(numero_pagina_2do_nivel,numero_pagina_2do_nivel,entrada_tabla_2do_nivel, puntero ,campo_segundo_nivel->id_tabla_primer_nivel);

			}else{
				log_error(logger,"No se reconoce el algoritmo de reemplazo");
			}
		}
	}

	log_info(logger,"\033[38;5;193m Devuelvo el marco %d al procesador", marco);

	send(cliente_socket, &marco, sizeof(int), 0);

	return 0;
}

bool encontrarPorMarco(void* campo1){
	CAMPO_FRAMES_ASIGNADOS* castCampo1 = (CAMPO_FRAMES_ASIGNADOS*)campo1;
	return castCampo1->marco == marcoAComparar;
}

void hacer_escritura(int cliente_socket,int marco,int desplazamiento , int valor) {
	escribirMemoria(configMemoriaDatos,marco,desplazamiento,valor);
	CAMPOS_SEGUNDO_NIVEL* c_segundo_nivel = (CAMPOS_SEGUNDO_NIVEL*)dictionary_get(diccionarioCampoSegundoNivelPorMarco, string_itoa(marco));
	c_segundo_nivel->uso = 1;
	c_segundo_nivel->modificado = 1;

	marcoAComparar = marco;

	CAMPO_FRAMES_ASIGNADOS* campoFramesAsignados = list_find(c_segundo_nivel->framesAsignados, encontrarPorMarco);
	campoFramesAsignados->uso = 1;
	campoFramesAsignados->modificado = 1;

	enviar_texto("OK", cliente_socket);
}

void hacer_lectura(int cliente_socket,int  marco, int desplazamiento){

	int valor= leerMemoria(configMemoriaDatos,marco,desplazamiento);

	log_info(logger, "\033[38;5;210m Lei el valor %d", valor);

	CAMPOS_SEGUNDO_NIVEL* c_segundo_nivel = (CAMPOS_SEGUNDO_NIVEL*)dictionary_get(diccionarioCampoSegundoNivelPorMarco, string_itoa(marco));
	c_segundo_nivel->uso = 1;

	marcoAComparar = marco;

	CAMPO_FRAMES_ASIGNADOS* campoFramesAsignados = list_find(c_segundo_nivel->framesAsignados, encontrarPorMarco);
	campoFramesAsignados->uso = 1;

	send(cliente_socket, &valor, sizeof(int), 0);
}

void eliminar_proceso(void* args) {
	log_info(logger, "\033[38;5;283m Finalizo un proceso, hay que eliminarlo");

	t_hilosMemoria_args* argumentosHilo = (t_hilosMemoria_args*) args;
	int cliente_socket = argumentosHilo->cliente_socket;
	int numero_pagina_1er_nivel = argumentosHilo->numero_pagina_1er_nivel;

	char* tablaDePaginasToString = string_itoa(numero_pagina_1er_nivel);

	sem_wait(&diccionarioPunterosMutex);
	PUNTERO_CLOCK* puntero = dictionary_get(diccionarioDePunteros, tablaDePaginasToString);
	sem_post(&diccionarioPunterosMutex);

	//Le saca los frames al proceso y los libera
	cambiarBitPresencia(puntero, numero_pagina_1er_nivel);

	log_info(logger, "Se libero su espacio en memoria");

	//Lo saco del diccionario de punteros
	
	sem_wait(&diccionarioPunterosMutex);
	dictionary_remove(diccionarioDePunteros, tablaDePaginasToString);
	sem_post(&diccionarioPunterosMutex);

	//log_info(logger, "Voy a eliminar su archivo de swap");


	long* pid = (long*)dictionary_get(diccionarioPidPorPagina, tablaDePaginasToString);

	char* stringPid = string_from_format("%ld",(long)pid);

	eliminarArchivoSwap(stringPid,configMemoriaDatos);

	//Lo saco del diccionario de pids
	sem_wait(&diccionarioPidsMutex);
	dictionary_remove(diccionarioPidPorPagina, tablaDePaginasToString);
	sem_post(&diccionarioPidsMutex);

	log_info(logger, "\033[38;5;121m Termine de eliminar al proceso!\n"); //Lo q esta adelante de la C de Control hace q el texto se printee en verde XDD		

}

void eliminarTablasDeUnProceso(PUNTERO_CLOCK *puntero_clock, int numero_pagina_1er_nivel){
	t_list *tablaPrimerNivel = (t_list *)list_remove(tablasPrimerNivel, numero_pagina_1er_nivel);
	
	int i = 0,j = 0, paginasLiberadas = 0;

	for(int i = 0; i < puntero_clock->cantidad_tablas_segundo_nivel ; i++ ){
		int *idTablaSegundoNivel = (int *)list_get(tablaPrimerNivel, i);
		t_list *tablaSegundoNivel = (t_list *)list_remove(tablasSegundoNivel,(int)idTablaSegundoNivel);
		while (j < configMemoriaDatos->entradas_por_tabla && paginasLiberadas < puntero_clock->cantidad_de_paginas  && j < list_size(tablaSegundoNivel)){
			//vacio la tabla de segundo nivel y libero sus campos
			CAMPOS_SEGUNDO_NIVEL *pagina = (CAMPOS_SEGUNDO_NIVEL *)list_remove(tablaSegundoNivel, j);
			dictionary_remove(diccionarioCampoSegundoNivelPorMarco, string_itoa(pagina->marco));
			free(pagina); //puede que rompa en la ultima tabla ?
			j++;
		}
		free(tablaSegundoNivel); //hay que liberar la lista?
	}
	free(tablaPrimerNivel);
}
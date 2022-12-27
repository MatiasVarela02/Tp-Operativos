#ifndef UTILS_H_
#define UTILS_H_
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include <commons/config.h>
#include<string.h>
#include<assert.h>
#include<signal.h>
#include<stdint.h>
#include<pthread.h>
#include<inttypes.h>
#include <semaphore.h>
#include <math.h>


#define IP "0.0.0.0"
#define PUERTO_CPU_DISPATCH "8001"
#define PUERTO_CPU_INTERRUPT "8005"

t_log* logger;
int nro_proceso;


/* ------------------------------ Enums ------------------------------*/

typedef enum{
	NO_OP,
	IO,
	READ,
	WRITE,
	COPY,
	EXIT,
}op_code;

typedef enum{
	PACKAGE,
	MESSAGE,
}message_code;

typedef enum{
    NEW,
    READY,
    RUNNING,
    BLOCKED,
    BLOCKEDSUSPENDED,
    SUSPENDEDREADY,
    EXIT_STATUS, 
}status;

typedef enum{
    SYSIO,
    SYSEXIT,
    SYSREPEAT,
    NONE,
}syscalls; 

typedef enum{
	CREAR, /* K */
	SUSPENDER,/* K */
    SEGUNDATABLA, /* P */
	MARCO, /* P */
    LEER, /* P */
	ESCRIBIR, /* P */
	BORRAR, /* K */
	NINGUNA,
	TERMINARCONEXION,
}PeticionMemoria; 

/* ------------------------------ Structs ------------------------------*/


typedef struct instruccion {
	int orden;
	op_code code;
	int cant;
	int* parametros;
	struct instruccion* sig;
} INSTRUCCION;

typedef struct proceso {
    int cantidadInstrucciones;
	int tamanioProceso;
    struct instruccion* instrucciones;
} PROCESO;

typedef struct proceso_serializado {
	int tamanioStream;
	int tamanioProceso;
	int cantidadInstrucciones;
	void* streamInstrucciones;
} PROCESO_SERIALIZADO;

typedef struct buffer {
	int size;
	void* stream;
} t_buffer;

typedef struct paquete {
	message_code codigo_operacion;
	struct proceso_serializado* buffer;
} PAQUETE;

typedef struct tipo_paquete {
	int message_code;
	t_buffer* buffer;
} t_paquete;

typedef struct process_control_block {
    long pid;
    int tamanio;
    int tabla_paginas;
    float estimacion_rafaga;
    status estado;
    long milisegundos_bloqueo;
	int cantidadInstrucciones;
    int cantInstruccionesEje;
    INSTRUCCION* program_counter;
    INSTRUCCION* instrucciones;
	double ultima_rafaga;
	int parametros_pendientes;
	int socketTerminal;
} PCB;

typedef struct listaProcesos {
	PCB pcb;
	int cantidadProcesos;
	struct listaProcesos* sig;
} LISTAPROCESOS;

typedef struct configuracion_kernel {
		char* ip_memoria;
		char* puerto_memoria;
		char* ip_cpu;
		char* puerto_cpu_dispatch;
		char* puerto_cpu_interrupt;
		char* puerto_escucha;
		char* algoritmo_planificacion;
		int estimacion_inicial;
		int alfa;
		int grado_multiprogramacion;
		int tiempo_maximo_bloqueado;

} CONFIGURACION_KERNEL;

typedef struct{
	t_log* log;
	int fd;
	char* server_name;
	int conexion_Dispatch;
	CONFIGURACION_KERNEL* configKernelDatos;
	int conexionMemoria;

}t_procesar_conexion_args;

typedef struct processor_architecture {
    INSTRUCCION* program_counter;
    int instruction_executed_register;
    int memory_address_register;
    int memory_buffer_register; 
    bool nuevoPCB;
    int tiempo_NOOP;
    syscalls tipo_syscall;
    int param_syscall;
	int memoria_tamanio_pagina;
	int memoria_entradas_por_tabla;	
	int tabla_primer_nivel;
	int filasTLB;
	int (*TLB) [4];
} PROCESSOR_ARQ;

typedef struct direccion_fisica {
    int entrada_tabla_1er_nivel;
	int entrada_tabla_2do_nivel;
	int desplazamiento;
} DIRECCION_FISICA;

void enviar_texto(char* mensaje, int socket_cliente);
char* recibir_texto(int socket_cliente);
void* recibir_buffer(int* size, int socket_cliente);
int recibir_operacion(int socket_cliente);
void liberar_conexion(int socket_cliente);
int crear_conexion(char *ip, char* puerto);
void enviar_mensaje(char* mensaje, int socket_cliente);
void terminar_programa(int conexion);
void recibir_mensaje(int socket_cliente);
int recibir_operacion(int socket_cliente);
int esperar_cliente(t_log* logger, const char* name, int socket_servidor);
int esperar_cliente_procesador(int socket_servidor);
int iniciar_servidor(char* ip,char* puerto);
int server_escuchar(t_log* logger, char* server_name, int server_socket,int conexion_Dispatch,CONFIGURACION_KERNEL* configKernelDatos);
static void procesar_conexion(void* void_args);
//int server_escuchar(t_log* logger, char* server_name, int server_socket,int conexion_Dispatch);


void enviar_texto(char* mensaje, int socket_cliente);
char* recibir_texto(int socket_cliente);
void enviar_mensaje(char* mensaje, int socket_cliente);


PCB* recibir_pcb(int socket_cliente);
INSTRUCCION* deserializar_instruccion_pcb(int socket_cliente);
void insertar_instrucciones_pcb(PCB* pcb_recibido,int socket_cliente);
void mostrar_pcb(PCB* pcb);
void enviar_pcb(PCB* pcb,int socket_cliente, bool esConexionConMemoria, PeticionMemoria codigo);
void* serializar_instruccion_pcb(INSTRUCCION* instruccion, void* stream, int desplazamiento, int size);
void liberar_pcb(PCB* pcb);


int instruccionValida(char* codigoOperacion);


int tamanio_instruccion(INSTRUCCION* instruccion);
void mostrarInstrucciones(INSTRUCCION *listaInstrucciones);
void insertarInstruccion(INSTRUCCION **listaInstrucciones, char *texto,int parametros[],int cantidad ,int orden);
void serializar_instruccion(INSTRUCCION* instruccion, PROCESO_SERIALIZADO* procesoSerializado);
int deserializar_instruccion(INSTRUCCION* instruccion, void* stream, int desplazamientoInicial);
void mostrarInstruccion(INSTRUCCION* instruccion);

void serializar_proceso(PROCESO* proceso, PROCESO_SERIALIZADO* procesoSerializado);
PROCESO_SERIALIZADO* inicializar_proceso_serializado(int tamanioProceso);
void deserializar_proceso(PROCESO_SERIALIZADO* procesoSerializado,PROCESO* procesoDeserializado);


void* serializar_paquete(PAQUETE* paquete, int bytes);
PAQUETE* crear_paquete(PROCESO_SERIALIZADO* procesoSerializado);
void enviar_paquete(PAQUETE* paquete, int socket_cliente);
void eliminar_paquete(PAQUETE* paquete);
PROCESO_SERIALIZADO* recibir_paquete(int socket_cliente);


PROCESSOR_ARQ* inicializarArqProcesador(void);
void fetch(PCB* pcb_recibido, PROCESSOR_ARQ* procesador);
void decode_and_fetch_operands(PCB* pcb_recibido,PROCESSOR_ARQ* procesador, char* reemplazo_tlb, int conexion_memoria);
void execute(PROCESSOR_ARQ* procesador, char* reemplazo_tlb, int conexion_memoria);
int pedirValorAMemoria(int tlb [][4], int filasTLB, int direccion_logica, int tamanio_pagina, int entradas_por_tabla, char* reemplazo_tlb, int conexion_memoria, int tabla_primer_nivel);
void escribirEnMemoria(int tlb [][4], int filasTLB, int direccion_logica, int valor, int tamanio_pagina, int entradas_por_tabla, char* reemplazo_tlb, int conexion_memoria, int tabla_primer_nivel);
void actualizar_PCB(PCB* pcb_recibido, PROCESSOR_ARQ* procesador, double tiempo_rafaga, int interrupciones);
int recibirInterrupcion(int socket_cliente);
DIRECCION_FISICA* MMU(int direccion_logica, int tamanio_pagina, int entradas_por_tabla);

void vaciarTLB (int tlb [][4], int filasTLB);
int buscarPorPagina(int tlb [][4], int filasTLB, int numero_pagina);
int buscarEnTLB (int tlb [][4], int filasTLB, int numero_pagina, DIRECCION_FISICA* direccion_fisica, char* reemplazo_tlb, int conexion_memoria, int tabla_primer_nivel);
void insertarEnTLB (int tlb [][4], int filasTLB, int numero_pagina, int numero_marco, char* reemplazo_tlb);
void reemplazoPorAlgoritmo(int tlb [][4], int filasTLB, int numero_pagina, int numero_marco, int columna);
void pedirLecturaPorMarco(int conexion_memoria, int marco, int desplazamiento);
void pedirEscrituraPorMarco(int conexion_memoria,int marco, int desplazamiento, int valor);
void pedirRegistroAMemoria(int conexion_memoria,int numero_pagina, int entrada, PeticionMemoria codigoPeticion);
int recibirRegistroDeMemoria(int conexion_memoria);

t_config* iniciar_config(char* direccion);
PCB* crearPCB(PROCESO* proceso, CONFIGURACION_KERNEL* configKernelDatos, int socket);
void agregarProcesoLista(LISTAPROCESOS** listaProceso, PCB pcb);
PCB obtenerPrimerProceso(LISTAPROCESOS* listaProcesos);
LISTAPROCESOS* eliminarPrimerProceso(LISTAPROCESOS* listaProcesos);
LISTAPROCESOS* eliminarProceso(LISTAPROCESOS* lista,PCB pcb);
int compararRafagas(float a, float b);
LISTAPROCESOS* ordenarPorMenorRafaga(LISTAPROCESOS* lista);
void inicializarListas();
void inicializarSemaforos();

int memoria_escuchar(t_log* logger, char* server_name, int server_socket);
void mostrarTLB(int TLB [][4], int filasTLB);

#endif /* UTILS_H_ */

#include "../../utils/src/utils.h"


typedef struct tConfProcesos{
	char* ip_memoria;
	char* puerto_memoria;
	char* ip_cpu;
	char* puerto_cpu_dispatch;
	char* puerto_cpu_interrupt;
	char* puerto_escucha;
	char* algoritmo_planificacion;
	int estimacion_inicial;
	float alfa;
	int grado_multiprogramacion;
	int tiempo_maximo_bloqueado;
    
}tproceso;


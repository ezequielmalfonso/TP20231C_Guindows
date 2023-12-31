/*
 * planificacion.h
 *
 *  Created on: 16 abr 2023
 *      Author: utnso
 */

#ifndef PLANIFICACION_H_
#define PLANIFICACION_H_

#include<pthread.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include<semaphore.h>
#include<commons/collections/queue.h>
#include<commons/collections/list.h>
#include<commons/collections/dictionary.h>
#include<commons/temporal.h>
#include "protocolo.h"
#include "kernel.h"


typedef struct {
    uint32_t pid;
    int64_t tiempo_in_exec;
    int64_t tiempo_out_exec;
    t_list* recursosAsignados; // Llevar este registro solo sirve para poder liberarlos si el proceso termino sin SIGNAL (por instrucciones sin signal o exit por error)
}t_tiempos_rafaga_anterior;

typedef struct{
	char* nombre;
	t_queue* c_proc_bloqueados;
}t_archivo_abierto;

extern pthread_mutex_t mx_cola_new;
extern pthread_mutex_t mx_cola_ready;
extern pthread_mutex_t mx_lista_block;
extern pthread_mutex_t mx_log;
extern pthread_mutex_t mx_cpu_desocupado;
extern pthread_mutex_t mx_memoria;

extern sem_t s_pasaje_a_ready, s_io,s_ready_execute,s_cpu_desocupado,s_cont_ready,s_multiprogramacion_actual,s_esperar_cpu,s_pcb_desalojado,s_blocked;
extern sem_t s_blocked_rec;
extern sem_t s_fs_compacta;
extern t_queue* cola_new;
extern t_queue* cola_ready;
extern t_queue* cola_ready_sec;
extern t_list* list_blocked;
extern t_tiempos_rafaga_anterior raf_anterior;
extern t_list* list_rafa_anterior;
extern t_list* tabla_global_archivos;

//t_dictionary* iteracion_blocked; no se que chota es

void esperar_cpu();
void bloqueando(PCB_t*);
void bloqueando_por_filesystem(PCB_t*);
void inicializarPlanificacion();
void execute_a_exit(PCB_t*, char*);
bool menor(PCB_t* a,PCB_t* b);
void ordenar_hrrn(t_queue *cola_ready);
double obtenerEstimadoRafaga(PCB_t* a,uint32_t estimadoInicial, double alfa);
//void ejecutar_io(PCB_t*,int);
char* procesosEnReady(t_queue*);
bool criterio_nombre_archivo(t_archivo_abierto* archivo);
bool criterio_nombre_archivo_proceso(t_archivoAbierto* archivo);
void esperar_filesystem(PCB_t* pcb);
void liberar_archivos(PCB_t* pcb);
void ejecutar_io(PCB_t* pcb);
bool criterio_nombre_recurso(char* recurso);
void liberar_recursos(PCB_t* pcb);
bool criterio_nombre_recurso_lista_recursos(t_recurso* recurso);



#endif /* PLANIFICACION_H_ */

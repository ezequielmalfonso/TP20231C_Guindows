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

extern pthread_mutex_t mx_cola_new;
extern pthread_mutex_t mx_cola_ready;
extern pthread_mutex_t mx_lista_block;
extern pthread_mutex_t mx_log;
extern pthread_mutex_t mx_cpu_desocupado;
extern pthread_mutex_t mx_memoria;

extern sem_t s_pasaje_a_ready, s_io,s_ready_execute,s_cpu_desocupado,s_cont_ready,s_multiprogramacion_actual,s_esperar_cpu,s_pcb_desalojado,s_blocked;
extern sem_t s_blocked_rec;
extern t_queue* cola_new;
extern t_queue* cola_ready;
extern t_queue* cola_ready_sec;
extern t_list* list_blocked;
//t_dictionary* iteracion_blocked; no se que chota es

void esperar_cpu();
void bloqueando(PCB_t*);
void inicializarPlanificacion();
void execute_a_exit(PCB_t*);
bool menor(PCB_t* a,PCB_t* b, uint32_t, double);
void ordenar_hrrn(t_queue *cola_ready,uint32_t estimacion, double alpha);
//void ejecutar_io(PCB_t*,int);



#endif /* PLANIFICACION_H_ */

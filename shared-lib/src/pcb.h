/*
 * pcb.h
 *
 * Created on: 09 abr 2023
 *      Author: utnso
 */

#ifndef SRC_PCB_H_
#define SRC_PCB_H_

#include<string.h>
#include<stdlib.h>
#include<stdint.h>
#include<commons/collections/list.h>
//#include "estructuras.h"

typedef struct{
  uint32_t tamSegmento;
  uint32_t id_tabla_pagina;
}TABLA_SEGMENTO;

struct s_registros{	// No deberian tener \0 pero por las dudas sumo uno
	char ax[5];
	char bx[5];
	char cx[5];
	char dx[5];
	char eax[9];
	char ebx[9];
	char ecx[9];
	char edx[9];
	char rax[17];
	char rbx[17];
	char rcx[17];
	char rdx[17];
}__attribute__((packed));	// Para que no use padding
typedef struct s_registros registros_t;


typedef struct {
	uint16_t pid;
	t_list* instrucciones;
	uint32_t pc; 				// Program counter
	registros_t registro_cpu;
	t_list* tabla_de_segmentos;
	double estimado_proxima_rafaga;
	int tiempo_llegada_a_ready;
	t_list* archivos_abiertos;
	int cliente_fd;              // consola cliente cuando hace de cliente de kernel
}PCB_t;


typedef struct{
	char nombre_archivo[20];
	uint32_t puntero;
}t_archivoAbierto;




PCB_t* pcb_create();

//void pcb_set(PCB_t* pcb,uint16_t pid, t_list* instrucciones, uint32_t pc, uint32_t registro_cpu[4], t_list* segmentos, int cliente);
void pcb_set(PCB_t*   pcb,
		     uint16_t pid,
			 t_list* instrucciones,
			 uint32_t pc,
			 registros_t registros_cpu,
			 //t_list* tabla_de_segmentos,
			// t_list* archivos_abiertos,
			 double   estimado_proxima_rafaga,
			 uint32_t tiempo_llegada_a_ready,
			 int cliente);

int pcb_find_index(t_list* lista, uint16_t pid);

void pcb_destroy(PCB_t* pcb);


#endif /* SRC_PCB_H_ */

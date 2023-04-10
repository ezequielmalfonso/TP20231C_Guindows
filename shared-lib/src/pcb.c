/*
 * pcb.c
 *
 *  Created on: 09 abr 2023
 *      Author: utnso
 */

#include "pcb.h"

PCB_t* pcb_create(){
	PCB_t* pcb 		 		= malloc(sizeof(PCB_t));
	pcb->instrucciones  	= list_create();
	pcb->segmentos			= list_create();
	pcb->nros_segmentos		= list_create();   // sino recuerdo mal de la teoria es unal ista por eso ya lo seteo como lista
	pcb->archivos_abiertos 	= list_create();

	//TODO ver como iniciar los valores de REG_USO_GRAL_CPU y TABLA_SEGMENTOS
	return pcb;
}

void pcb_set(PCB_t*   pcb,
			 uint16_t pid,
			 t_list*  instrucciones,
			 uint32_t pc,
			 uint32_t registro_cpu[4],
			 //t_list*  segmentos,    /// TODO preguntar de donde saldria este dato?????
			 t_list*  archivos_abiertos,
			 double   estimado_rafaga_inicial,
			 int      cliente){

	//list_destroy_and_destroy_elements(pcb->instrucciones,free);
	list_add_all(pcb->instrucciones,instrucciones);
	pcb->pid=pid;
	pcb->pc = pc;
	//antes de cambiar de puntero, destruyo toda existencia de la anterior
	pcb->registro_cpu[0] = registro_cpu[0];
	pcb->registro_cpu[1] = registro_cpu[1];
	pcb->registro_cpu[2] = registro_cpu[2];
	pcb->registro_cpu[3] = registro_cpu[3];
	//list_destroy_and_destroy_elements(pcb->segmentos,free);
    //list_add_all(pcb->segmentos,segmentos);
 //   TODO la parte de segmentos cuando lo aclaren
    pcb->estimado_rafaga_inicial = estimado_rafaga_inicial;
    pcb->cliente_fd=cliente;
}


int pcb_find_index(t_list* lista, uint16_t pid){
	for (int i = 0; i < list_size(lista); i++){
		PCB_t* otro_pcb = list_get(lista,i);
		if (otro_pcb->pid == pid)
			return i;
	}
	return -1;
}

void pcb_destroy(PCB_t* pcb){
	list_destroy(pcb->instrucciones); //ROMPE ESTO
	list_destroy(pcb->segmentos);
	list_destroy(pcb->nros_segmentos);
	free(pcb);
}


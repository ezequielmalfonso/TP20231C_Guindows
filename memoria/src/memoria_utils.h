/*
 * memoriaUtils.h
 *
 *  Created on: 12 abr 2023
 *      Author: utnso
 */

#ifndef MEMORIA_UTILS_H_
#define MEMORIA_UTILS_H_

#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include <stdint.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/string.h>
#include<readline/readline.h>
#include<semaphore.h>
#include<commons/collections/queue.h>
#include<commons/collections/list.h>
#include<commons/collections/dictionary.h>
#include "memoriaConfig.h"

extern t_segmento* segmento;
void inicializar_memoria();
void apagar_memoria();
uint64_t firstFit(int tam);
t_list* buscarTabla(uint32_t pid);
t_segmento* buscarSegmento(t_list* tabla, uint32_t id);
void crearSegmento(uint32_t pid, uint32_t id_seg, int tam);
t_list* crearTabla();
t_list* cargarProceso(uint32_t pid);
void eliminarSegmentoProceso(uint32_t pid, uint32_t sid);
void agregarHueco(t_segmento* seg);
bool noHayEspacio(int tam);
void* leerMemoria(uint32_t id_seg, uint32_t desplazamiento, uint32_t pid, int tam);

typedef struct{
	uint32_t id_proceso;
	t_list* tablaDelProceso;
}t_nodoDePagina;


#endif /* MEMORIA_UTILS_H_ */

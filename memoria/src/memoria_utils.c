/*
 * memoriaUtils.c
 *
 *  Created on: 12 abr 2023
 *      Author: utnso
 */


#include "memoria_utils.h"

uint32_t puerto_escucha;
uint32_t tam_memoria;
uint32_t tam_segmento_0;
uint32_t cant_segmentos;
uint32_t retardo_memoria;
uint32_t retardo_compactacion;
char* algortimo_asignacion;

void* memoria;
t_list* lista_tablas_de_procesos;

//t_log * logger;

/*
 * Inicializacion de Estructuras administrativas de Memoria
 */

void inicializar_memoria(){

	tam_memoria 		 = configuracion->TAM_MEMORIA;
	tam_segmento_0 		 = configuracion->TAM_SEGMENTO_0;
	cant_segmentos 		 = configuracion->CANT_SEGMENTOS;
	retardo_memoria 	 = configuracion->RETARDO_MEMORIA;
	retardo_compactacion = configuracion->RETARDO_COMPACTACION;
	algortimo_asignacion = configuracion->ALGORITMO_ASIGNACION;


	log_info(logger,"Algoritmo %s configurado", configuracion->ALGORITMO_ASIGNACION);

	//log_info(logger, "Enviando a CPU: tam_pagina=%d - cant_ent_paginas=%d", configuracion->TAM_PAGINA, configuracion->ENTRADAS_POR_TABLA);
	//send(cliente_cpu, &entradas_por_tabla, sizeof(uint16_t),0);
	//send(cliente_cpu, &tam_pagina, sizeof(uint16_t),0);

	memoria = malloc(configuracion->TAM_MEMORIA);
	//tabla_de_paginas = list_create();
	lista_tablas_de_procesos = list_create();



}


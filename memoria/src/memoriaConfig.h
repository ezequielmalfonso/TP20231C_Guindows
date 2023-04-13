/*
 * memoriaConfig.h
 *
 *  Created on: 12 abr 2023
 *      Author: utnso
 */

#ifndef MEMORIACONFIG_H_
#define MEMORIACONFIG_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "commons/log.h"
#include "commons/config.h"

typedef struct {
	uint32_t PUERTO_ESCUCHA;
	uint32_t TAM_MEMORIA;
	uint32_t TAM_SEGMENTO_0;
	uint32_t CANT_SEGMENTOS;
	uint32_t RETARDO_MEMORIA;
	uint32_t RETARDO_COMPACTACION;
	char* ALGORITMO_ASIGNACION;
} t_config_memoria;

extern t_config_memoria * configuracion;
extern t_config * fd_configuracion;
extern t_log * logger;

int cargarConfiguracion();

#endif /* MEMORIACONFIG_H_ */

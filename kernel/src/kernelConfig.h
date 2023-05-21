/*
 * kernelConfig.h
 *
 *  Created on: 6 abr 2023
 *      Author: utnso
 */

#ifndef KERNELCONFIG_H_
#define KERNELCONFIG_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "commons/log.h"
#include "commons/config.h"
#include "commons/collections/list.h"

typedef struct {
	char* IP_MEMORIA;
	uint32_t PUERTO_MEMORIA;
	char* IP_FILESYSTEM;
	uint32_t PUERTO_FILESYSTEM;
	char* IP_CPU;
	uint32_t PUERTO_CPU;
	uint32_t PUERTO_ESCUCHA;
	uint32_t ESTIMACION_INICIAL;
	double HRRN_ALFA;
	char* ALGORITMO_PLANIFICACION;
	uint32_t GRADO_MAX_MULTIPROGRAMACION;
	char** RECURSOS;
	char** INSTANCIAS_RECURSOS;
} t_config_kernel;


extern t_config_kernel * configuracion;
extern t_config * fd_configuracion;
extern t_log * logger;
extern t_list* t_list_recursos;

int cargarConfiguracion();


#endif /* KERNELCONFIG_H_ */

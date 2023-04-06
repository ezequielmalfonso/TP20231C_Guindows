/*
 * consolaConfig.h
 *
 *  Created on: 6 abr 2023
 *      Author: utnso
 */

#ifndef CONSOLACONFIG_H_
#define CONSOLACONFIG_H_
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "commons/log.h"
#include "commons/config.h"
#include "commons/collections/list.h"

typedef struct {
	char* IP_KERNEL;
	uint32_t PUERTO_KERNEL;
	//char** SEGMENTOS;
	//uint32_t TIEMPO_PANTALLA;
} t_config_consola;

extern t_config_consola * configuracion;
extern t_config * fd_configuracion;
extern t_log * logger;

int cargarConfiguracion();
void limpiarConfiguracion();


#endif /* CONSOLACONFIG_H_ */

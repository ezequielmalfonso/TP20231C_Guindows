/*
 * fileSystemConfig.h
 *
 *  Created on: 13 abr 2023
 *      Author: utnso
 */

#ifndef FILESYSTEMCONFIG_H_
#define FILESYSTEMCONFIG_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "commons/log.h"
#include "commons/config.h"

typedef struct {
	char* IP_MEMORIA;
	uint32_t PUERTO_MEMORIA;
	uint32_t PUERTO_ESCUCHA;
	char* PATH_SUPERBLOQUE;
	char* PATH_BITMAP;
	char* PATH_BLOQUES;
	char* PATH_FCB;
	uint32_t RETARDO_ACCESO_BLOQUE;
} t_config_file_system;

extern t_config_file_system * configuracion;
extern t_config * fd_configuracion;
extern t_log * logger;

int cargarConfiguracion();

#endif /* FILESYSTEMCONFIG_H_ */

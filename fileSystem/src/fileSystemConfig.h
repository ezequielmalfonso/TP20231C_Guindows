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
#include "commons/bitarray.h"


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

typedef struct {
	char nombre_archivo[20];
	uint32_t tamanio_archivo;
	uint32_t puntero_directo;
	uint32_t puntero_indirecto;
} t_FCB;//repetido

extern t_config_file_system * configuracion;
extern t_config * fd_configuracion;
extern t_log * logger;
extern t_config* FCB;
//extern void* fileData;
extern t_FCB* FCB_archivo;//repetido


int cargarConfiguracion();

#endif /* FILESYSTEMCONFIG_H_ */

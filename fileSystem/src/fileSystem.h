/*
 * fileSystem.h
 *
 *  Created on: 13 abr 2023
 *      Author: utnso
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <fcntl.h>

#include "fileSystemComunicacion.h"
#include "socket.h"
#include "fileSystemConfig.h"
#include "sys/mman.h"

typedef struct {
	uint32_t BLOCK_SIZE;
	uint32_t BLOCK_COUNT;
} t_super_bloque;

typedef struct {
	char* nombre_archivo;
	uint32_t tamanio_archivo;
	uint32_t puntero_directo;
	uint32_t puntero_indirecto;
} t_FCB;

extern t_super_bloque* configuracionSuperBloque;

int fileExiste(char* nombreArchivo);

extern int fileSystemServer;

#endif /* FILESYSTEM_H_ */

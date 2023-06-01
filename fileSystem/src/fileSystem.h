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

#include "fileSystemComunicacion.h"
#include "socket.h"
#include "fileSystemConfig.h"

typedef struct {
	uint32_t BLOCK_SIZE;
	uint32_t BLOCK_COUNT;
} t_super_bloque;



extern int fileSystemServer;

#endif /* FILESYSTEM_H_ */

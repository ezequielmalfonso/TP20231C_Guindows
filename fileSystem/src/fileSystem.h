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
#include "protocolo.h"

//#include "fileSystemComunicacion.h"
#include "socket.h"
#include "fileSystemConfig.h"
#include "sys/mman.h"

typedef struct {
	uint32_t BLOCK_SIZE;
	uint32_t BLOCK_COUNT;
} t_super_bloque;


extern t_super_bloque* configuracionSuperBloque;

int fileExiste(char* nombreArchivo);
int cargarArchivoBloques(char *path, int BLOCK_SIZE, int BLOCK_COUNT);
int cargarSuperBloque(char *path);
int iniciarBitmap (char* path ,uint32_t block_count );

int buscarPrimerBloqueVacio (t_bitarray* s_bitmap, uint32_t BLOCK_SIZE);


extern int fileSystemServer;


#endif /* FILESYSTEM_H_ */

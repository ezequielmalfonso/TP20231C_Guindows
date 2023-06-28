/*
 * comunicacion.h
 *
 *  Created on: 13 abr 2023
 *      Author: utnso
 */

#ifndef FILESYSTEMCOMUNICACION_H_
#define FILESYSTEMCOMUNICACION_H_

#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <inttypes.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include "protocolo.h"
#include "socket.h"
#include "fileSystemConfig.h"
#include "fileSystem.h"
#include <semaphore.h>
#include <dirent.h>
#include <math.h>

extern int cliente_socket;
extern t_FCB* FCB_archivo;
//extern void* fileData;
extern t_bitarray* s_bitmap;
int max(int a, int b);
int min(int a, int b);
extern char* bloqueIndirectoBuffer;


#endif /* FILESYSTEMCOMUNICACION_H_ */

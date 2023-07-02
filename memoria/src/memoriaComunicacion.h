/*
 * comunicacion.h
 *
 *  Created on: 12 abr 2023
 *      Author: utnso
 */

#ifndef MEMORIACOMUNICACION_H_
#define MEMORIACOMUNICACION_H_

#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <inttypes.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include "protocolo.h"
#include "socket.h"
#include "memoriaConfig.h"
#include "memoria_utils.h"
#include <semaphore.h>

extern t_list* tabla_de_huecos;
extern void* memoria;
extern t_list* tabla_de_paginas;
//KERNEL
int kernel_escuchar(char* server_name, int server_socket);
//CPU
int cpu_escuchar(char* server_name, int server_socket);

#endif /* MEMORIACOMUNICACION_H_ */

/*
 * comunicacion.h
 *
 *  Created on: 12 abr 2023
 *      Author: utnso
 */

#ifndef COMUNICACION_H_
#define COMUNICACION_H_

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


//KERNEL
int kernel_escuchar(char* server_name, int server_socket);
//CPU
int cpu_escuchar(char* server_name, int server_socket);

#endif /* COMUNICACION_H_ */

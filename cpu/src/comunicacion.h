/*
 * comunicacion.h
 *
 *  Created on: 11 abr 2023
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
#include "cpuConfig.h"
//#include "main.h"
#include <semaphore.h>

//sem_t sem;
extern uint16_t pid_actual;

int server_escuchar(char* server_name, int server_socket);


extern int cliente_socket;


#endif /* COMUNICACION_H_ */

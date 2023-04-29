/*
 * comunicacion.h
 *
 *  Created on: 13 abr 2023
 *      Author: utnso
 */

#ifndef CPUCOMUNICACION_H_
#define CPUCOMUNICACION_H_

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
#include "cpu.h"
#include <semaphore.h>

extern int cliente_socket;


#endif /* CPUCOMUNICACION_H_ */

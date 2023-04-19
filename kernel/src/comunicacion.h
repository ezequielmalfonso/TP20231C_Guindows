/*
 * comunicacion.h
 *
 *  Created on: 6 abr 2023
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
#include "socket.h"
#include "kernelConfig.h"
#include "protocolo.h"
#include "planificacion.h"
#include "pcb.h"
#include "estructuras.h"

extern int cliente_socket;

extern uint16_t pid_nuevo;
//SERVIDOR
int server_escuchar(char* server_name, int server_socket);
//CLIENTE
int generar_conexiones(int* cpu_fd, t_config_kernel* configuracion);
//int generar_conexion_memoria(int* memoria_fd, t_config_kernel* configuracion);

#endif /* COMUNICACION_H_ */

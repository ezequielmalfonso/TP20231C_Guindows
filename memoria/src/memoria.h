/*
 * memoria.h
 *
 *  Created on: 12 abr 2023
 *      Author: utnso
 */

#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include "socket.h"
#include "memoriaConfig.h"
#include "protocolo.h"

#include "memoriaComunicacion.h"

int memoriaServer;

void escuchar_kernel();
void escuchar_cpu();
void escuchar_fileSystem();

#endif /* MEMORIA_H_ */

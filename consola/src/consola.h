/*
 * consola.h
 *
 *  Created on: 6 abr 2023
 *      Author: utnso
 */

#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include "socket.h"
#include "protocolo.h"
#include "estructuras.h"
#include "consolaConfig.h"

#define LONGITUD_MAXIMA_LINEA 30   // Para el archivo a leer

void parseo_instrucciones(char* path_instrucciones, t_list* listaIntrucciones);



#endif /* CONSOLA_H_ */

/*
 * estructuras.h
 *
 *  Created on: 6 apr 2022
 *      Author: utnso
 */

#include<commons/collections/queue.h>

/*
  PREGUNTAR POR LONGITUD DE CHAR DE la estructura de instruccion????
 */

typedef struct instruccion{
	char comando[20];
	char parametro1[20];
	char parametro2[20];
	char parametro3[20];
} INSTRUCCION;

typedef struct{
	uint32_t id_segmento;
	uint64_t direccion_base;
	uint32_t tamanio_segmento;
}t_segmento;


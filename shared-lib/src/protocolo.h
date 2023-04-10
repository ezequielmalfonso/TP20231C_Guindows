/*
 * protocolo.h
 *
 *  Created on: 6 abr 2023
 *      Author: utnso
 */
#ifndef SRC_PROTOCOLO_H_
#define SRC_PROTOCOLO_H_

#include <inttypes.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include "pcb.h"


typedef struct {
    uint32_t size; // Tamaño del payload
    void* stream; // Payload
} t_buffer;

typedef struct{
	uint32_t elementosLista;
	t_list* listaInstrucciones;
   // uint32_t cantSegmentos;
   // t_list* listaTamSegmentos;
} t_instrucciones;


//INSTRUCCIONES CONSOLA-KERNEL
void* serializar_instrucciones_tam(uint32_t size, t_list* lista);
t_instrucciones* deserializar_instrucciones(t_buffer* buffer);
void enviar_instrucciones(int socket_fd, t_list* lista);
t_instrucciones* recibir_instrucciones(int socket_fd);
uint32_t calcular_instrucciones_buffer_size(t_list* lista);

#endif /* SRC_PROTOCOLO_H_ */

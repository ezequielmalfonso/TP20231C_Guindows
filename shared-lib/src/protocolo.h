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

typedef struct {
    uint32_t size; // Tamaño del payload
    void* stream; // Payload
} t_buffer;

void enviar_instrucciones(int socket_fd, t_list* lista);

#endif /* SRC_PROTOCOLO_H_ */

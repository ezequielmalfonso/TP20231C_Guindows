/*
 * comunicacion.c
 *
 *  Created on: 6 abr 2023
 *      Author: utnso
 */
#include "comunicacion.h"

uint16_t pid_nuevo=0;

typedef struct {
    int fd;
    char* server_name;
} t_procesar_conexion_args;
//pthread_mutex_t pid_xd = PTHREAD_MUTEX_INITIALIZER;
int cliente_socket;

static void procesar_conexion(void* void_args) {
	t_procesar_conexion_args* args = (t_procesar_conexion_args*) void_args;
	int cliente_socket = args->fd;
	char* server_name = args->server_name;
	free(args);

	t_instrucciones* mensaje=malloc(sizeof(t_instrucciones));
	mensaje=recibir_instrucciones(cliente_socket);

	return ;
}
int server_escuchar(char* server_name, int server_socket) {
    cliente_socket = esperar_cliente(logger, server_name, server_socket);


    if (cliente_socket != -1) {
        pthread_t hilo;
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        args->fd = cliente_socket;
        args->server_name = server_name;
        pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) args);
        pthread_detach(hilo);
        return 1;
    }
    return 0;
}

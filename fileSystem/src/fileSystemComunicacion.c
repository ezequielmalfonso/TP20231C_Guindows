/*
 * comunicacion.c
 *
 *  Created on: 13 abr 2023
 *      Author: utnso
 */

#include "fileSystemComunicacion.h"

int cliente_socket;
typedef struct {
    int fd;
    char* server_name;
} t_procesar_conexion_args;


static void procesar_conexion(void* void_args) {
 t_procesar_conexion_args* args = (t_procesar_conexion_args*) void_args;
 int cliente_socket = args->fd;
 char* server_name = args->server_name;
 free(args);

 op_code cop;
 while (cliente_socket != -1) {

	 if (recv(cliente_socket, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
		 log_info(logger, "Se ha finalizado la conexion");
		 return;
	 }

	 char parametro1[20], parametro2[20], parametro3[20];

	 switch (cop) {
	 	 int fd_archivo;
	 	 case DEBUG:
	 		 log_info(logger, "debug");
	 		 break;

		 case F_OPEN:
			 recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_OPEN con parametros %s, %s y %s", parametro1, parametro2, parametro3);
			 if(recorrerFCBs(parametro1) == -1){
				 send(cliente_socket, sizeof(op_code), F_OPEN_FAIL);
				 log_info(logger, "El archivo %s no existe", parametro1);
			 }	else {
				 send(cliente_socket, sizeof(op_code), F_OPEN_OK);
				 log_info(logger, "Abrir archivo: %s", parametro1);
			 }

			 break;

		 case F_CREATE:
			 recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Crear archivo: %s", parametro1);
			 fd_archivo = open(parametro1, O_CREAT);
			 fwrite(fd_archivo,"NOMBRE_ARCHIVO: %s", parametro1);
			 fwrite(fd_archivo,	"TAMANIO_ARCHIVO: %d", 0);
			 fwrite(fd_archivo, "PUNTERO_DIRECTO: ");
			 fwrite(fd_archivo, "PUNTERO_INDIRECTO: ");
			 break;

		 case F_CLOSE:
			 recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_CLOSE con parametros %s, %s y %s", parametro1, parametro2, parametro3);
			 break;

		 case F_SEEK:
			 recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_SEEK con parametros %s, %s y %s", parametro1, parametro2, parametro3);
			 break;

		 case F_TRUNCATE:
			 recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_TRUNCATE con parametros %s, %s y %s", parametro1, parametro2, parametro3);
			 break;

		 case F_READ:
			 recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_READ con parametros %s, %s y %s", parametro1, parametro2, parametro3);
			 break;

		 case F_WRITE:
			 recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_WRITE con parametros %s, %s y %s", parametro1, parametro2, parametro3);
			 break;

		 // Errores
		 case -1:
			 log_error(logger, "Cliente desconectado de %s...", server_name);
			 return;
		 default:
			 log_error(logger, "Algo anduvo mal en el server de %s", server_name);
			 log_info(logger, "Cop: %d", cop);
			 return;
	 }
 }

 log_warning(logger, "El cliente se desconecto de %s server", server_name);
 return;
 }


//CLIENTE
//MEMORIA
int generar_conexion(int* memoria_fd, t_config_file_system* configuracion) {
    char* port_memoria = string_itoa(configuracion->PUERTO_MEMORIA);

    *memoria_fd = crear_conexion(
            logger,
            "MEMORIA",
            configuracion->IP_MEMORIA,
            port_memoria
    );

    free(port_memoria);

    return *memoria_fd != 0;
}

int server_escuchar(char* server_name, int server_socket) {
    cliente_socket = esperar_cliente(logger, server_name, server_socket);

    if (cliente_socket != -1) {
        pthread_t hilo;
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        args->fd = cliente_socket;
        args->server_name = server_name;
        pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) args);
        //pthread_detach(hilo);
        return 1;
    }
    return 0;
}

int recorrerFCBs(){

	return 0;
}

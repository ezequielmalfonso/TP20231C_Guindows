/*
 * comunicacion.c
 *
 *  Created on: 13 abr 2023
 *      Author: utnso
 */

#include "cpuComunicacion.h"

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

	 switch (cop) {
		 case DEBUG:
					 log_info(logger, "debug");
					 break;
		 case DISPATCH:
			 	    	PCB_t* proceso= pcb_create();
			 	    	//log_info(logger,"Recibiendo PCB desde %s ", cop );
			 	    	recv_proceso(cliente_socket,proceso);
			 	    	log_info(logger, "Recibi PCB id: %d", proceso->pid);
			 	    	// Cargar contexto de ejecucion
						memcpy(regAX, proceso->registro_cpu.ax, 4);		memcpy(regBX, proceso->registro_cpu.bx, 4);
						memcpy(regCX, proceso->registro_cpu.cx, 4);		memcpy(regDX, proceso->registro_cpu.dx, 4);
						memcpy(regEAX, proceso->registro_cpu.eax, 8);	memcpy(regEBX, proceso->registro_cpu.ebx, 8);
						memcpy(regECX, proceso->registro_cpu.ecx, 8);	memcpy(regEDX, proceso->registro_cpu.edx, 8);
						memcpy(regRAX, proceso->registro_cpu.rax, 16);	memcpy(regRBX, proceso->registro_cpu.rbx, 16);
						memcpy(regRCX, proceso->registro_cpu.rcx, 16);	memcpy(regRDX, proceso->registro_cpu.rdx, 16);

			 	    	op_code codigo=iniciar_ciclo_instruccion(proceso);
			 	    	//log_info(logger, "Cop: %d ", codigo);
			 	    	// Guardo contexto de ejecucion del proceso
			 	    	memcpy(proceso->registro_cpu.ax, regAX, 4);		memcpy(proceso->registro_cpu.bx, regBX, 4);
			 	    	memcpy(proceso->registro_cpu.cx, regCX, 4);		memcpy(proceso->registro_cpu.dx, regDX, 4);
			 	    	memcpy(proceso->registro_cpu.eax, regEAX, 8);	memcpy(proceso->registro_cpu.ebx, regEBX, 8);
			 	    	memcpy(proceso->registro_cpu.ecx, regECX, 8);	memcpy(proceso->registro_cpu.edx, regEDX, 8);
			 	    	memcpy(proceso->registro_cpu.rax, regRAX, 16);	memcpy(proceso->registro_cpu.rbx, regRBX, 16);
			 	    	memcpy(proceso->registro_cpu.rcx, regRCX, 16);	memcpy(proceso->registro_cpu.rdx, regRDX, 16);
			 	    	send_proceso(cliente_socket,proceso,codigo);			// envio proceso al kenel
			 	    	break;
		 // Errores
		 case -1:
				 log_error(logger, "Cliente desconectado de %s...", server_name);
				 return;
		 default:
				 log_error(logger, "Algo anduvo mal en el server de %s", server_name);
				 //log_info(logger, "Cop: %d", cop);
				 return;
	 }
 }

 log_warning(logger, "El cliente se desconecto de %s server", server_name);
 return;
 }



//CLIENTE
//MEMORIA
int generar_conexion(int* memoria_fd, t_config_cpu* configuracion) {
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

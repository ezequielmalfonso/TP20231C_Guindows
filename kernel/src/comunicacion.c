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
	double   estimado_rafaga_inicial;
	free(args);

	t_instrucciones* mensaje=malloc(sizeof(t_instrucciones));
	mensaje=recibir_instrucciones(cliente_socket);

	// Una vez recibidas las instruccion desde consola creo el PCB
 /*   PCB_t* proceso = malloc(sizeof(PCB_t));
	proceso = pcb_create();

	// Aca inicializo los valores para el PCB
	//TODO pasarle los valores de inicializacion al PCB
	uint32_t registros[4];
	registros[0]=0;
	registros[1]=0;
	registros[2]=0;
	registros[3]=0;

	//pthread_mutex_lock(&pid_xd);

	uint32_t archivos_abiertos[4];
	archivos_abiertos[0]=0;
	archivos_abiertos[1]=0;
	archivos_abiertos[2]=0;
	archivos_abiertos[3]=0;

	estimado_rafaga_inicial = configuracion->ESTIMACION_INICIAL;

	pcb_set(proceso, pid_nuevo, mensaje->listaInstrucciones, 0
			,registros
			//, lista de segmento PREGUNTAR ????
			, archivos_abiertos
			, estimado_rafaga_inicial
			, cliente_socket);

	//pid_nuevo++;
	//pthread_mutex_unlock(&pid_xd);
*/
	list_destroy(mensaje->listaInstrucciones);
	free(mensaje);

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
        //pthread_detach(hilo);
        return 1;
    }
    return 0;
}

//CLIENTE
//CPU
int generar_conexiones(int* cpu_fd, t_config_kernel* configuracion) {
    char* port_cpu = string_itoa(configuracion->PUERTO_CPU);

    *cpu_fd = crear_conexion(
            logger,
            "CPU",
            configuracion->IP_CPU,
            port_cpu
    );

    free(port_cpu);

    return *port_cpu != 0;
}




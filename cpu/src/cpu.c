/*
 * cpu.c
 *
 *  Created on: 13 abr 2023
 *      Author: utnso
 */
#include "cpu.h"

int cpuServer;
int memoria_fd;
pthread_mutex_t mx_memoria = PTHREAD_MUTEX_INITIALIZER;

int main(){
	cargarConfiguracion();
	t_config* config_ips = config_create("../ips.conf");
	char* ip = config_get_string_value(config_ips,"IP_CPU");

	//CLIENTE Conexion a MEMORIA
	generar_conexion(&memoria_fd, configuracion);
	// ENVIO y RECEPCION A MEMORIA
	op_code op=CPU;
	//pthread_mutex_lock(&mx_memoria);
	send(memoria_fd,&op,sizeof(op_code),0);
	//pthread_mutex_unlock(&mx_memoria);
	// INICIO CPU SERVIDOR
	char* puertoCPU = string_itoa(configuracion->PUERTO_ESCUCHA);
	int cpuServer= iniciar_servidor(logger,"kernel server",ip,puertoCPU);//ACA IP PROPIA
	while (server_escuchar("CPU_SV", cpuServer));

	limpiarConfiguracion();
	return 0;
}


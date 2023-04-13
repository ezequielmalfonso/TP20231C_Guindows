/*
 * cpu.c
 *
 *  Created on: 13 abr 2023
 *      Author: utnso
 */
#include "cpu.h"

int cpuServer;
int memoria_fd;

int main(){
	cargarConfiguracion();
	t_config* config_ips = config_create("../ips.conf");
	char* ip = config_get_string_value(config_ips,"IP_CPU");

	//CLIENTE Conexion a MEMORIA
	generar_conexion(&memoria_fd, configuracion);
	// ENVIO y RECEPCION A MEMORIA
	op_code op=INICIALIZAR;

	// INICIO CPU SERVIDOR
	char* puertoCPU = string_itoa(configuracion->PUERTO_ESCUCHA);
	int cpuServer= iniciar_servidor(logger,"kernel server",ip,puertoCPU);//ACA IP PROPIA
	while (server_escuchar("CPU_SV", cpuServer));

	limpiarConfiguracion();
	return 0;
}


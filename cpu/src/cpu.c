/*
 ============================================================================
 Name        : cpu.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "cpu.h"

int memoria_fd;
//int cpuServer;

int main(void) {

	cargarConfiguracion();
	t_config* config_ips = config_create("../ips.conf");
	char* ip = config_get_string_value(config_ips,"IP_CPU");
	//log_info(logger,"\nPUERTO_ESCUCHA: %d\n", configuracion->PUERTO_ESCUCHA);

	char* puerto = string_itoa(configuracion->PUERTO_ESCUCHA);
	//TODO Conexion con MEMORIA y ENVIO Y RECEPCION DE DATOS CON MEMORIA


	//INICIO SERVIDOR CPU
	int cpuServer= iniciar_servidor(logger,"CPU server",ip,puerto);//ACA IP PROPIA
	free(puerto);
	while (server_escuchar("CPU_SV", cpuServer));

	limpiarConfiguracion();

	return 0;
}

void serverCPU() {

	//cargarConfiguracion();

	//char* puertoDispatch = string_itoa(configuracion->PUERTO_ESCUCHA_DISPATCH);

	//cpuServerDispatch = iniciar_servidor(logger,"dispatch server","127.0.0.1",(char*) puerto);

	//free(puerto);

	//while(server_escuchar("CPU_SV",cpuServer));

}



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
int cpuServer;

int main(void) {

	cargarConfiguracion();
	t_config* config_ips = config_create("../ips.conf");
	char* ip = config_get_string_value(config_ips,"IP_CPU");

	char* puerto = string_itoa(8001);

	printf("PORT CPU: %d", puerto);
	//INICIO SERVIDOR
	cpuServer = iniciar_servidor(logger,"Server CPU",ip,puerto);//ACA IP PROPIA

	free(puerto);

	pthread_t cpu_id;

	pthread_create(&cpu_id,NULL,(void*) serverCPU,NULL);

	pthread_join(cpu_id,0);

	limpiarConfiguracion();

	return 0;
}

void serverCPU() {

	//cargarConfiguracion();

	//char* puertoDispatch = string_itoa(configuracion->PUERTO_ESCUCHA_DISPATCH);

	//cpuServerDispatch = iniciar_servidor(logger,"dispatch server","127.0.0.1",(char*) puerto);

	//free(puerto);

	while(server_escuchar("CPU_SV",cpuServer));

}



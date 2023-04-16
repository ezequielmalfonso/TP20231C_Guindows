
#include <stdio.h>
#include <stdlib.h>
#include "kernel.h"

int cpu_fd, memoria_fd, file_system_fd;

int main(void) {
	cargarConfiguracion();
	t_config* config_ips = config_create("../ips.conf");

	char* ip = config_get_string_value(config_ips,"IP_KERNEL");
	inicializarPlanificacion();
	char* puerto = string_itoa(configuracion->PUERTO_ESCUCHA);

	//CLIENTE
	//CPU
	op_code op=DEBUG;
	generar_conexiones(&cpu_fd, configuracion);
	//send(cpu_fd,&op,sizeof(op_code),0);
	//MEMORIA
	generar_conexion_memoria(&memoria_fd, configuracion);
	// Es para probar el envio a memoria desde kernel
	 op=KERNEL;
	send(memoria_fd,&op,sizeof(op_code),0);
	//FILESYSTEM
	generar_conexion_fileSystem(&file_system_fd, configuracion);

	//INICIO SERVIDOR
	int kernelServer= iniciar_servidor(logger,"kernel server",ip,puerto);//ACA IP PROPIA
	free(puerto);
	while (server_escuchar("KERNEL_SV", kernelServer));

	limpiarConfiguracion();
	return 0;
}

/*
 ============================================================================
 Name        : fileSystem.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "fileSystem.h"

int fileSystemServer;
int memoria_fd;

int main(void) {
	cargarConfiguracion();
	t_config* config_ips = config_create("../ips.conf");
	char* ip = config_get_string_value(config_ips,"IP_FILESYSTEM");

	//CLIENTE Conexion a MEMORIA
	generar_conexion(&memoria_fd, configuracion);
	// ENVIO y RECEPCION A MEMORIA
	op_code op=INICIALIZAR;
	send(memoria_fd,&op,sizeof(op_code),0);
	// INICIO CPU SERVIDOR
	char* puertoFileSystem = string_itoa(configuracion->PUERTO_ESCUCHA);
	int FileSystemServer= iniciar_servidor(logger,"FileSystem server",ip,puertoFileSystem);//ACA IP PROPIA
	while (server_escuchar("FILESYSTEM_SV", FileSystemServer));

	limpiarConfiguracion();
	return 0;


}

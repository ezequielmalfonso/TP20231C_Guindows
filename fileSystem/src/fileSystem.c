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
	// Configuracion gral
	cargarConfiguracion();
	// Aca deberia ir cargarSUperBloque?
	//log_warning(logger, "PATH: %s",configuracion->PATH_SUPERBLOQUE );
	cargarSuperBloque(configuracion->PATH_SUPERBLOQUE);

	t_config* config_ips = config_create("../ips.conf");
	char* ip = config_get_string_value(config_ips,"IP_FILESYSTEM");

	//CLIENTE Conexion a MEMORIA
	generar_conexion(&memoria_fd, configuracion);
	// ENVIO y RECEPCION A MEMORIA
	op_code op=FS;
	send(memoria_fd,&op,sizeof(op_code),0);

	// Aca deberia ir cargarBitmap de bloque
	FILE *bitmap;
	bitmap = fopen("bitmap.dat","w");

	// Hay que usar mmap, msync, y munmap en ese orden
	//int *ptr = mmap (NULL,(configuracionSuperBloque->BLOCK_COUNT / 8), PROT_READ | PROT_WRITE,MAP_PRIVATE,fileno(bitmap),0);


	// INICIO CPU SERVIDOR
	char* puertoFileSystem = string_itoa(configuracion->PUERTO_ESCUCHA);
	int FileSystemServer= iniciar_servidor(logger,"FileSystem server",ip,puertoFileSystem);//ACA IP PROPIA
	while (server_escuchar("FILESYSTEM_SV", FileSystemServer));


	limpiarConfiguracion();
	return 0;


}


// No sabia donde ponerlo ajsjas
// Cargar archivo superbloque
int configValidaSuperBloque(t_config* superBloque){
	return (config_has_property(superBloque,"BLOCK_SIZE") && config_has_property(superBloque,"BLOCK_COUNT"));
}

int cargarSuperBloque(char *path){
	log_warning(logger, "PATH: %s",configuracion->PATH_SUPERBLOQUE );
	t_config* superBloque;
	t_super_bloque* configuracionSuperBloque = malloc(sizeof(t_super_bloque));
	//logger = log_create("superBloque.log", "SuperBLoque", 1, LOG_LEVEL_INFO);

		superBloque = config_create(path);
		if (superBloque == NULL) {
			superBloque = config_create(path);
		}

		if (superBloque == NULL || !configValidaSuperBloque(superBloque)) {
			log_error(logger,"Archivo de SuperBloque inválido.");
			return -1;
		}
		log_error(logger, "PATH: %s",configuracion->PATH_SUPERBLOQUE );
		configuracionSuperBloque->BLOCK_SIZE = config_get_int_value(superBloque, "BLOCK_SIZE");
		configuracionSuperBloque->BLOCK_COUNT = config_get_int_value(superBloque, "BLOCK_COUNT");


		log_info(logger,
			"\nBLOCK_SIZE: %d\n"
			"BLOCK_COUNT: %d",
			configuracionSuperBloque->BLOCK_SIZE,
			configuracionSuperBloque->BLOCK_COUNT);

	return 0;
}

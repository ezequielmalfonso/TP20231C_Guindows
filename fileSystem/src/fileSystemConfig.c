/*
 * fileSystemConfig.c
 *
 *  Created on: 13 abr 2023
 *      Author: utnso
 */


#include "fileSystemConfig.h"

t_config_file_system * configuracion;
t_config * fd_configuracion;
t_log * logger;

int configValida(t_config* fd_configuracion) {
	return (config_has_property(fd_configuracion,  "IP_MEMORIA")
		&& config_has_property(fd_configuracion,  "PUERTO_MEMORIA")
		&& config_has_property(fd_configuracion,  "PUERTO_ESCUCHA")
		&& config_has_property(fd_configuracion,  "PATH_SUPERBLOQUE")
		&& config_has_property(fd_configuracion,  "PATH_BITMAP")
		&& config_has_property(fd_configuracion,  "PATH_BLOQUES")
		&& config_has_property(fd_configuracion,  "PATH_FCB")
		&& config_has_property(fd_configuracion,  "RETARDO_ACCESO_BLOQUE"));
}

int cargarConfiguracion() {
	logger = log_create("LogFileSystem.log", "FileSystem", 1, LOG_LEVEL_INFO);
	configuracion = malloc(sizeof(t_config_file_system));

	fd_configuracion = config_create("fileSystem.conf");
	if (fd_configuracion == NULL) {
		fd_configuracion = config_create("fileSystem.conf");
	}

	if (fd_configuracion == NULL || !configValida(fd_configuracion)) {
		log_error(logger,"Archivo de configuración inválido.");
		return -1;
	}

	configuracion->IP_MEMORIA 				= config_get_string_value(fd_configuracion, "IP_MEMORIA");
	configuracion->PUERTO_MEMORIA 			= config_get_int_value(fd_configuracion, "PUERTO_MEMORIA");
	configuracion->PUERTO_ESCUCHA       	= config_get_int_value(fd_configuracion, "PUERTO_ESCUCHA");
	configuracion->PATH_SUPERBLOQUE 		= config_get_string_value(fd_configuracion, "PATH_SUPERBLOQUE");
	configuracion->PATH_BITMAP		 		= config_get_string_value(fd_configuracion, "PATH_BITMAP");
	configuracion->PATH_BLOQUES		 		= config_get_string_value(fd_configuracion, "PATH_BLOQUES");
	configuracion->PATH_FCB		 			= config_get_string_value(fd_configuracion, "PATH_FCB");
	configuracion->RETARDO_ACCESO_BLOQUE	= config_get_int_value(fd_configuracion, "RETARDO_ACCESO_BLOQUE");


	log_info(logger,
		"\nIP_MEMORIA: %s\n"
		"PUERTO_MEMORIA: %d\n"
	    "PUERTO_ESCUCHA: %d\n"
		"PATH_SUPERBLOQUE: %s\n"
		"PATH_BITMAP: %s\n"
		"PATH_BLOQUES: %s\n"
		"PATH_FCB: %s\n"
		"RETARDO_ACCESO_BLOQUE: %d\n",
		configuracion->IP_MEMORIA,
		configuracion->PUERTO_MEMORIA,
		configuracion->PUERTO_ESCUCHA,
		configuracion->PATH_SUPERBLOQUE,
		configuracion->PATH_BITMAP,
		configuracion->PATH_BLOQUES,
		configuracion->PATH_FCB,
		configuracion->RETARDO_ACCESO_BLOQUE
		);

	return 0;
}

void limpiarConfiguracion() {
	free(configuracion);
	config_destroy(fd_configuracion);
	log_destroy(logger);

}

/*
 * consolaConfig.c
 *
 *  Created on: 6 abr 2023
 *      Author: utnso
 */
#include "consolaConfig.h"

t_config_consola * configuracion;
t_config * fd_configuracion;
t_log * logger;

int configValida(t_config* fd_configuracion) {
	return (config_has_property(fd_configuracion, "IP_KERNEL")
		&& config_has_property(fd_configuracion, "PUERTO_KERNEL")
		);
}

int cargarConfiguracion(char* path) {
	int i=0;
	logger = log_create("LogConsola.log", "Consola", 1, LOG_LEVEL_INFO);
	configuracion = malloc(sizeof(t_config_consola));

	fd_configuracion = config_create(path);
	if (fd_configuracion == NULL) {
		fd_configuracion = config_create(path);
	}

	if (fd_configuracion == NULL || !configValida(fd_configuracion)) {
		log_error(logger,"Archivo de configuración inválido.");
		return -1;
	}

	configuracion->IP_KERNEL     = config_get_string_value(fd_configuracion, "IP_KERNEL");
	configuracion->PUERTO_KERNEL = config_get_int_value(fd_configuracion, "PUERTO_KERNEL");


	log_info(logger,
		"\nIP_KERNEL: %s\n"
		"PUERTO_KERNEL: %d",
		configuracion->IP_KERNEL,
		configuracion->PUERTO_KERNEL);
	/*TODO REVISAR LOGGER OBLIGATORIOS DESPUES HAY QUE ADAPTARLO PARA LOGGEAR LOS SEGMENTOS*/

	return 0;
}

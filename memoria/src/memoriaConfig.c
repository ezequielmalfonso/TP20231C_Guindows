/*
 * memoriaConfig.c
 *
 *  Created on: 12 abr 2023
 *      Author: utnso
 */
#include "memoriaConfig.h"

t_config_memoria * configuracion;
t_config * fd_configuracion;
t_log * logger;

int configValida(t_config* fd_configuracion) {
	return (config_has_property(fd_configuracion, "PUERTO_ESCUCHA")
		&& config_has_property(fd_configuracion, "TAM_MEMORIA")
		&& config_has_property(fd_configuracion, "TAM_SEGMENTO_0")
		&& config_has_property(fd_configuracion, "CANT_SEGMENTOS")
		&& config_has_property(fd_configuracion, "RETARDO_MEMORIA")
		&& config_has_property(fd_configuracion, "RETARDO_COMPACTACION")
		&& config_has_property(fd_configuracion, "ALGORITMO_ASIGNACION"));
}

int cargarConfiguracion() {
	logger = log_create("LogMemoria.log", "Memoria", 1, LOG_LEVEL_INFO);
	configuracion = malloc(sizeof(t_config_memoria));

	fd_configuracion = config_create("memoria.conf");
	if (fd_configuracion == NULL) {
		fd_configuracion = config_create("memoria.conf");
	}

	if (fd_configuracion == NULL || !configValida(fd_configuracion)) {
		log_error(logger,"Archivo de configuración inválido.");
		return -1;
	}

	configuracion->PUERTO_ESCUCHA 		= config_get_int_value(fd_configuracion, "PUERTO_ESCUCHA");
	configuracion->TAM_MEMORIA 			= config_get_int_value(fd_configuracion, "TAM_MEMORIA");
	configuracion->TAM_SEGMENTO_0 		= config_get_int_value(fd_configuracion, "TAM_SEGMENTO_0");
	configuracion->CANT_SEGMENTOS 		= config_get_int_value(fd_configuracion, "CANT_SEGMENTOS");
	configuracion->RETARDO_MEMORIA 		= config_get_int_value(fd_configuracion, "RETARDO_MEMORIA");
	configuracion->RETARDO_COMPACTACION = config_get_int_value(fd_configuracion, "RETARDO_COMPACTACION");
	configuracion->ALGORITMO_ASIGNACION = config_get_string_value(fd_configuracion, "ALGORITMO_ASIGNACION");


	log_info(logger,
		"\PUERTO_ESCUCHA: %d\n"
		"TAM_MEMORIA: %d\n"
		"TAM_SEGMENTO_0: %d\n"
		"CANT_SEGMENTOS: %d\n"
		"RETARDO_MEMORIA: %d\n"
		"RETARDO_COMPACTACION: %d\n"
		"ALGORITMO_ASIGNACION: %s\n",
		configuracion->PUERTO_ESCUCHA,
		configuracion->TAM_MEMORIA,
		configuracion->TAM_SEGMENTO_0,
		configuracion->CANT_SEGMENTOS,
		configuracion->RETARDO_MEMORIA,
		configuracion->RETARDO_COMPACTACION,
		configuracion->ALGORITMO_ASIGNACION
		);
	return 0;
}


void limpiarConfiguracion() {
	free(configuracion);
	config_destroy(fd_configuracion);
	log_destroy(logger);

}

/*
 * cpuConfig.c
 *
 *  Created on: Apr 06, 2023
 *      Author: utnso
 */

#include "cpuConfig.h"

t_config_cpu * configuracion;
t_config * fd_configuracion;
t_log * logger;

int configValida(t_config* fd_configuracion) {
	return (config_has_property(fd_configuracion, "RETARDO_INSTRUCCION")
		&& config_has_property(fd_configuracion,  "IP_MEMORIA")
		&& config_has_property(fd_configuracion,  "PUERTO_MEMORIA")
		&& config_has_property(fd_configuracion,  "PUERTO_ESCUCHA")
		&& config_has_property(fd_configuracion,  "TAM_MAX_SEGMENTO"));
}


int cargarConfiguracion() {
	logger = log_create("LogCpu.log", "Cpu", 1, LOG_LEVEL_INFO);
	configuracion = malloc(sizeof(t_config_cpu));

	//fd_configuracion = config_create("cpu.conf");
	log_warning(logger, "Cpu config pruebas");
	fd_configuracion = config_create("configPruebas/cpu_BASE.conf");

	if (fd_configuracion == NULL) {
		fd_configuracion = config_create("cpu.conf");
	}

	if (fd_configuracion == NULL || !configValida(fd_configuracion)) {
		log_error(logger,"Archivo de configuración inválido.");
		return -1;
	}

	configuracion->RETARDO_INSTRUCCION 	= config_get_int_value(fd_configuracion, "RETARDO_INSTRUCCION");
	configuracion->IP_MEMORIA 			= config_get_string_value(fd_configuracion, "IP_MEMORIA");
	configuracion->PUERTO_MEMORIA 		= config_get_int_value(fd_configuracion, "PUERTO_MEMORIA");
	configuracion->PUERTO_ESCUCHA       = config_get_int_value(fd_configuracion, "PUERTO_ESCUCHA");
	configuracion->TAM_MAX_SEGMENTO 	= config_get_int_value(fd_configuracion, "TAM_MAX_SEGMENTO");

	log_info(logger,
		"\nRETARDO_INSTRUCCION %d\n"
		"IP_MEMORIA: %s\n"
		"PUERTO_MEMORIA: %d\n"
	    "PUERTO_ESCUCHA: %d\n"
		"TAM_MAX_SEGMENTO: %d\n",
		configuracion->RETARDO_INSTRUCCION,
		configuracion->IP_MEMORIA,
		configuracion->PUERTO_MEMORIA,
		configuracion->PUERTO_ESCUCHA,
		configuracion->TAM_MAX_SEGMENTO
		);
	return 0;
}

void limpiarConfiguracion() {
	free(configuracion);
	config_destroy(fd_configuracion);
	log_destroy(logger);

}

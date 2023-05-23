/*
 * kernelConfig.c
 *
 *  Created on: 6 abr 2023
 *      Author: utnso
 */

#include "kernelConfig.h"

t_config_kernel * configuracion;
t_config * fd_configuracion;
t_log * logger;

t_list* lista_de_recursos;

int configValida(t_config* fd_configuracion) {
	return (config_has_property(fd_configuracion, "IP_MEMORIA")
		&& config_has_property(fd_configuracion, "PUERTO_MEMORIA")
		&& config_has_property(fd_configuracion, "IP_FILESYSTEM")
		&& config_has_property(fd_configuracion, "PUERTO_FILESYSTEM")
		&& config_has_property(fd_configuracion, "IP_CPU")
		&& config_has_property(fd_configuracion, "PUERTO_CPU")
		&& config_has_property(fd_configuracion, "PUERTO_ESCUCHA")
		&& config_has_property(fd_configuracion, "ALGORITMO_PLANIFICACION")
		&& config_has_property(fd_configuracion, "ESTIMACION_INICIAL")
		&& config_has_property(fd_configuracion, "HRRN_ALFA")
		&& config_has_property(fd_configuracion, "GRADO_MAX_MULTIPROGRAMACION")
		&& config_has_property(fd_configuracion, "RECURSOS")
		&& config_has_property(fd_configuracion, "INSTANCIAS_RECURSOS"));
}

int cargarConfiguracion() {
	int i = 0;
	int total_recursos = 0;
	logger = log_create("LogKernel.log", "Kernel", 1, LOG_LEVEL_INFO);
	configuracion = malloc(sizeof(t_config_kernel));

	fd_configuracion = config_create("kernel.conf");
	if (fd_configuracion == NULL) {
		fd_configuracion = config_create("kernel.conf");
	}

	if (fd_configuracion == NULL || !configValida(fd_configuracion)) {
		log_error(logger,"Archivo de configuración inválido.");
		return -1;
	}

	configuracion->IP_MEMORIA 					= config_get_string_value(fd_configuracion, "IP_MEMORIA");
	configuracion->PUERTO_MEMORIA 				= config_get_int_value(fd_configuracion, "PUERTO_MEMORIA");
	configuracion->IP_FILESYSTEM 				= config_get_string_value(fd_configuracion, "IP_FILESYSTEM");
	configuracion->PUERTO_FILESYSTEM 			= config_get_int_value(fd_configuracion, "PUERTO_FILESYSTEM");
	configuracion->IP_CPU 						= config_get_string_value(fd_configuracion, "IP_CPU");
	configuracion->PUERTO_CPU 					= config_get_int_value(fd_configuracion, "PUERTO_CPU");
	configuracion->PUERTO_ESCUCHA 				= config_get_int_value(fd_configuracion, "PUERTO_ESCUCHA");
	configuracion->ALGORITMO_PLANIFICACION 		= config_get_string_value(fd_configuracion, "ALGORITMO_PLANIFICACION");
	configuracion->ESTIMACION_INICIAL 			= config_get_int_value(fd_configuracion, "ESTIMACION_INICIAL");
	configuracion->HRRN_ALFA 					= config_get_double_value(fd_configuracion, "HRRN_ALFA");
	configuracion->GRADO_MAX_MULTIPROGRAMACION 	= config_get_int_value(fd_configuracion, "GRADO_MAX_MULTIPROGRAMACION");
	configuracion->RECURSOS 					= config_get_array_value(fd_configuracion, "RECURSOS");
	configuracion->INSTANCIAS_RECURSOS 			= config_get_array_value(fd_configuracion, "INSTANCIAS_RECURSOS");


	//Armo la lista de recursos con sus instancias iniciales.
	lista_de_recursos = list_create();
	//string_from_format(configuracion->RECURSOS[i]);
	// Agrego cada recurso en una posicion de la lista de recursos disponibles con su cantidad de instancias disponibles
	// por archivo de configuracion
	//string_from_format(configuracion->RECURSOS[i])

	while(configuracion->RECURSOS[i] != NULL)
	{
		t_recurso* aux_recurso = malloc(sizeof(t_recurso));
		//aux_recurso->recurso =  configuracion->RECURSOS[i];
		///log_info(logger, "Long %d  RECURSO: %s con %d instancias", strlen(configuracion->RECURSOS[i]), configuracion->RECURSOS[i],atoi(configuracion->INSTANCIAS_RECURSOS[i]) );

		strcpy(aux_recurso->recurso, configuracion->RECURSOS[i]);
		aux_recurso->instancias = atoi(configuracion->INSTANCIAS_RECURSOS[i]);
		aux_recurso->cola_bloqueados_recurso = queue_create();
		list_add(lista_de_recursos, aux_recurso);

		i++;
	}

	log_info(logger,
		"\nIP_MEMORIA: %s\n"
		"PUERTO_MEMORIA: %d\n"
		"IP_FILESYSTEM: %s\n"
		"PUERTO_FILESYSTEM: %d\n"
		"IP_CPU: %s\n"
		"PUERTO_CPU: %d\n"
		"PUERTO_ESCUCHA: %d\n"
		"ALGORITMO_PLANIFICACION: %s\n"
		"ESTIMACION_INICIAL: %d\n"
		"HRRN_ALFA: %.2f\n"
		"GRADO_MAX_MULTIPROGRAMACION: %d\n",
		//"RECURSOS: %s\n"
		//"INSTANCIAS_RECURSOS: %s\n",
		configuracion->IP_MEMORIA,
		configuracion->PUERTO_MEMORIA,
		configuracion->IP_FILESYSTEM,
		configuracion->PUERTO_FILESYSTEM,
		configuracion->IP_CPU,
		configuracion->PUERTO_CPU,
		configuracion->PUERTO_ESCUCHA,
		configuracion->ALGORITMO_PLANIFICACION,
		configuracion->ESTIMACION_INICIAL,
		configuracion->HRRN_ALFA,
		configuracion->GRADO_MAX_MULTIPROGRAMACION
		//configuracion->RECURSOS,
		//configuracion->INSTANCIAS_RECURSOS   //, HAY QUE VER COMO MOSTRARLOS PORQUE SON LISTAS
	    );


	while(configuracion->RECURSOS[total_recursos] != NULL)
	{
		log_info(logger, "RECURSO: %s con %d instancias", configuracion->RECURSOS[total_recursos],atoi(configuracion->INSTANCIAS_RECURSOS[total_recursos]) );
		total_recursos++;
	}
	log_info(logger, "Cantidad total de recursos disponibles: %d ", total_recursos);

	return 0;
}

void limpiarConfiguracion() {
	free(configuracion);
	config_destroy(fd_configuracion);
	log_destroy(logger);

}



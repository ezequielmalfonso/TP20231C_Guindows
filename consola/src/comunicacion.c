/*
 * comunicacion.c
 *
 *  Created on:
 *      Author: utnso
 */

#include "comunicacion.h"

int generar_conexion(int* kernel_fd, t_config_consola* configuracion) {
    char* port_kernel = string_itoa(configuracion->PUERTO_KERNEL);

    *kernel_fd = crear_conexion(
            logger,
            "KERNEL",
            configuracion->IP_KERNEL,
            port_kernel
    );

    free(port_kernel);

    return *kernel_fd != 0;
}


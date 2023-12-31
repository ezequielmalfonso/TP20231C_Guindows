/*
 * kernel.h
 *
 *  Created on: 6 abr 2023
 *      Author: utnso
 */

#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include "socket.h"

#include "kernelComunicacion.h"
#include "kernelConfig.h"


extern int kernelServer;

extern int cpu_fd, memoria_fd, file_system_fd;




#endif /* KERNEL_H_ */

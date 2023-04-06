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
#include "kernelConfig.h"
#include "comunicacion.h"

extern int kernelServer;

extern int interrupt_fd, dispatch_fd, memoria_fd;


#endif /* KERNEL_H_ */

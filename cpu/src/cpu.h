/*
 * cpu.h
 *
 *  Created on: 13 abr 2023
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include "socket.h"
#include "estructuras.h"

#include "cpuComunicacion.h"
#include "cpuConfig.h"

extern int cpuServer;

op_code iniciar_ciclo_instruccion(PCB_t* pcb);
INSTRUCCION* fetch(t_list* instrucciones, uint32_t pc);
int decode(INSTRUCCION* instruccion_ejecutar );
int execute(INSTRUCCION* instruccion_ejecutar, registros_t registros, uint16_t pid,uint32_t pc);



#endif /* CPU_H_ */

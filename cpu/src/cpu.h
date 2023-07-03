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
#include "mmu.h"


extern int cpuServer;
extern int memoria_fd;
extern void* base_memoria;
extern void *regAX, *regBX, *regCX, *regDX, *regEAX, *regEBX, *regECX, *regEDX, *regRAX, *regRBX, *regRCX, *regRDX;	// REGISTROS

op_code iniciar_ciclo_instruccion(PCB_t* pcb);
INSTRUCCION* fetch(t_list* instrucciones, uint32_t pc);
int decode(INSTRUCCION* instruccion_ejecutar );
int execute(INSTRUCCION* instruccion_ejecutar, registros_t registros, uint16_t pid,uint32_t pc, PCB_t* pcb);



#endif /* CPU_H_ */

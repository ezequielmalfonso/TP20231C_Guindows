/*
 * cpu.c
 *
 *  Created on: 13 abr 2023
 *      Author: utnso
 */
#include "cpu.h"

int cpuServer;
int memoria_fd;
pthread_mutex_t mx_memoria = PTHREAD_MUTEX_INITIALIZER;
void* base_memoria;
// REGISTROS
void* regAX;
void* regBX;
void* regCX;
void* regDX;
void* regEAX;
void* regEBX;
void* regECX;
void* regEDX;
void* regRAX;
void* regRBX;
void* regRCX;
void* regRDX;

int main(){
	cargarConfiguracion();
	t_config* config_ips = config_create("../ips.conf");
	char* ip = config_get_string_value(config_ips,"IP_CPU");

	// REGISTROS
	regAX = malloc(4);
	regBX = malloc(4);
	regCX = malloc(4);
	regDX = malloc(4);
	regEAX = malloc(8);
	regEBX = malloc(8);
	regECX = malloc(8);
	regEDX = malloc(8);
	regRAX = malloc(16);
	regRBX = malloc(16);
	regRCX = malloc(16);
	regRDX = malloc(16);

	//CLIENTE Conexion a MEMORIA
	generar_conexion(&memoria_fd, configuracion);
	// ENVIO y RECEPCION A MEMORIA
	op_code op=CPU;
	//pthread_mutex_lock(&mx_memoria);
	send(memoria_fd,&op,sizeof(op_code),0);
	recv(memoria_fd, &base_memoria, sizeof(base_memoria), 0);
	//pthread_mutex_unlock(&mx_memoria);
	// INICIO CPU SERVIDOR
	char* puertoCPU = string_itoa(configuracion->PUERTO_ESCUCHA);
	int cpuServer= iniciar_servidor(logger,"cpu server",ip,puertoCPU);//ACA IP PROPIA
	while (server_escuchar("CPU_SV", cpuServer));

	limpiarConfiguracion();
	return 0;
}


//EJECUCION DE INSTRUCCIONES
op_code iniciar_ciclo_instruccion(PCB_t* pcb){
	op_code estado = CONTINUE;
	while (estado == CONTINUE){ // Solo sale si hay una interrupcion, un pedido de I/O, o fin de ejecucion
		INSTRUCCION* instruccion_ejecutar = fetch(pcb->instrucciones, pcb->pc);

		if(decode(instruccion_ejecutar)){
			log_info(logger,"En CPU");
			usleep(configuracion->RETARDO_INSTRUCCION*1000);
		}
		estado = execute(instruccion_ejecutar,pcb->registro_cpu,pcb->pid, pcb->pc, pcb);
		if(estado == CONTINUE){
			//estado = check_interrupt();
		}
		pcb->pc++;
		/*if(estado == PAGEFAULT){
			pcb->pc--;
		}*/
	}
	return estado;
}

INSTRUCCION* fetch(t_list* instrucciones, uint32_t pc){
	return list_get(instrucciones,pc);
}

int decode(INSTRUCCION* instruccion_ejecutar ){
	return (!strcmp(instruccion_ejecutar->comando,"SET"));
}


int execute(INSTRUCCION* instruccion_ejecutar, registros_t registros, uint16_t pid, uint32_t pc, PCB_t* pcb){



	if(!strcmp(instruccion_ejecutar->comando,"SET") ){
		//log_info(logger,"PID: %d - Ejecutando SET parametro 1: %s parametro 2: %s", pid,instruccion_ejecutar->parametro1,instruccion_ejecutar->parametro2);
		log_info(logger,"PID: %d - Ejecutando SET parametro 1: %s parametro 2: %s", pid,instruccion_ejecutar->parametro1,instruccion_ejecutar->parametro2);
		set_registro(instruccion_ejecutar->parametro1, instruccion_ejecutar->parametro2, pcb);

		//log_error(logger,"Se seteo el registro: %s",pcb->registro_cpu.ax );	// NO solo el ax

	}else if(!strcmp(instruccion_ejecutar->comando,"MOV_OUT") ){

		log_info(logger,"PID: %d -  Listo para ejecutar MOV_OUT ", pid);
		log_info(logger,"PID: %d - Se ha ejecutado MOV_OUT parametro 1: %s parametro 2: %s", pid,instruccion_ejecutar->parametro1,instruccion_ejecutar->parametro2);
		mov_out(instruccion_ejecutar->parametro1, instruccion_ejecutar->parametro2, pcb);
		log_info(logger,"PID: %d - Se ha ejecutado MOV_OUT parametro 1: %s parametro 2: %s", pid,instruccion_ejecutar->parametro1,instruccion_ejecutar->parametro2);

	}else if(!strcmp(instruccion_ejecutar->comando,"WAIT") ){

		log_info(logger,"PID: %d - Ejecutando WAIT a Kernel por recurso: %s ", pid,instruccion_ejecutar->parametro1);

		return WAIT;

	}else if(!strcmp(instruccion_ejecutar->comando,"I/O") ){

		log_info(logger,"PID: %d - Ejecutando IO parametro 1: %s ", pid,instruccion_ejecutar->parametro1);
		return IO;

	}else if(!strcmp(instruccion_ejecutar->comando,"SIGNAL") ){

		log_info(logger,"PID: %d - Ejecutando SIGNAL a kernel por recurso: %s ", pid,instruccion_ejecutar->parametro1);
		return SIGNAL;

	}else if(!strcmp(instruccion_ejecutar->comando,"MOV_IN") ){
		log_info(logger,"PID: %d -  Listo para ejecutar MOV_IN ", pid);
		mov_in(instruccion_ejecutar->parametro2,instruccion_ejecutar->parametro1, pcb);
		log_info(logger,"PID: %d - Se ha ejecutado MOV_IN parametro 1: %s parametro 2: %s", pid,instruccion_ejecutar->parametro1,instruccion_ejecutar->parametro2);


	}else if(!strcmp(instruccion_ejecutar->comando,"F_OPEN") ){

		log_info(logger,"PID: %d - Ejecutando F_OPEN - Solicita a kernel abrir: %s ", pid,instruccion_ejecutar->parametro1);
		return F_OPEN;

	}else if(	!strcmp(strtok(instruccion_ejecutar->comando, "\n"),"YIELD") ||
				!strcmp(strtok(instruccion_ejecutar->comando, "\r"),"YIELD")){

		log_info(logger,"PID: %d - Ejecutando YIELD ", pid);

		return YIELD;

	}else if(!strcmp(instruccion_ejecutar->comando,"F_TRUNCATE") ){

		log_info(logger,"PID: %d - Listo para ejecutar F_TRUNCATE ", pid);
		return F_TRUNCATE;

	}else if(!strcmp(instruccion_ejecutar->comando,"F_SEEK") ){

		log_info(logger,"PID: %d - Listo para ejecutar F_SEEK ", pid);
		return F_SEEK;

	}else if(!strcmp(instruccion_ejecutar->comando,"F_WRITE") ){

		log_info(logger,"PID: %d - Listo para ejecutar F_WRITE ", pid);
		void* instruccion_fisica = traducirAFisica( atoi(instruccion_ejecutar->parametro2), pcb);
		log_warning(logger, "Dir Fisica: %d ", instruccion_fisica);
		strcpy(instruccion_ejecutar->parametro2, string_itoa(instruccion_fisica));

		return F_WRITE;

	}else if(!strcmp(instruccion_ejecutar->comando,"F_READ") ){

		log_info(logger,"PID: %d - Listo para ejecutar F_READ ", pid);

		void* instruccion_fisica = traducirAFisica( atoi(instruccion_ejecutar->parametro2), pcb);
		strcpy(instruccion_ejecutar->parametro2, string_itoa(instruccion_fisica));

		return F_READ;

	}else if(!strcmp(instruccion_ejecutar->comando,"F_CLOSE") ){

		log_info(logger,"PID: %d - Listo para ejecutar F_CLOSE ", pid);
		return F_CLOSE;

	}else if(!strcmp(instruccion_ejecutar->comando,"F_CREATE") ){
		log_info(logger,"PID: %d - Listo para ejecutar F_CREATE", pid);
		return F_CREATE;

	}else if(!strcmp(instruccion_ejecutar->comando,"EXIT") ){
		log_info(logger,"PID: %d - Ejecutando EXIT", pid);
		//limpiar_tlb();
		return EXIT;

	}else if(!strcmp(instruccion_ejecutar->comando,"CREATE_SEGMENT") ){
			log_info(logger,"PID: %d - Ejecutando CREATE_SEGMENT", pid);
			return CREATE_SEGMENT;

	}else if(!strcmp(instruccion_ejecutar->comando,"DELETE_SEGMENT") ){
			log_info(logger,"PID: %d - Ejecutando DELETE_SEGMENT", pid);
			return DELETE_SEGMENT;
	}else{
		//printf("Comando: %s | Par1: %s | Par2: %s | Par: %s\n\n", instruccion_ejecutar->comando, instruccion_ejecutar->parametro1, instruccion_ejecutar->parametro2 ,instruccion_ejecutar->parametro3);
		log_error(logger,"Hubo un error en el ciclo de instruccion, ");
	}
	return CONTINUE; // solo par aprobar la funcion
}


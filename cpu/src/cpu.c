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

int main(){
	cargarConfiguracion();
	t_config* config_ips = config_create("../ips.conf");
	char* ip = config_get_string_value(config_ips,"IP_CPU");

	//CLIENTE Conexion a MEMORIA
	generar_conexion(&memoria_fd, configuracion);
	// ENVIO y RECEPCION A MEMORIA
	op_code op=CPU;
	//pthread_mutex_lock(&mx_memoria);
	send(memoria_fd,&op,sizeof(op_code),0);
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
		estado = execute(instruccion_ejecutar,pcb->registro_cpu,pcb->pid, pcb->pc);
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


int execute(INSTRUCCION* instruccion_ejecutar,char* registros,uint16_t pid, uint32_t pc){

	if(!strcmp(instruccion_ejecutar->comando,"SET") && pc == 0){
			log_info(logger,"PID: %d - Ejecutando SET parametro 1: %s parametro 2: %s", pid,instruccion_ejecutar->parametro1,instruccion_ejecutar->parametro2);
			registros = instruccion_ejecutar->parametro2;

		}else if(!strcmp(instruccion_ejecutar->comando,"MOV_OUT") && pc == 1){

					log_info(logger,"PID: %d -  Listo para ejecutar MOV_OUT ", pid);

		}else if(!strcmp(instruccion_ejecutar->comando,"WAIT") && pc == 2){

			log_info(logger,"PID: %d -  Listo para ejecutar WAIT ", pid);

		}else if(!strcmp(instruccion_ejecutar->comando,"I/O") && pc == 3){

			log_info(logger,"PID: %d - Ejecutando IO parametro 1: %s ", pid,instruccion_ejecutar->parametro1);
			return IO;

		}else if(!strcmp(instruccion_ejecutar->comando,"SIGNAL") && pc == 4){

			log_info(logger,"PID: %d - Listo para ejecutar SIGNAL ", pid);

		}else if(!strcmp(instruccion_ejecutar->comando,"MOV_IN") && pc == 5){

			log_info(logger,"PID: %d - Listo para ejecutar MOV_IN ", pid);

		}else if(!strcmp(instruccion_ejecutar->comando,"F_OPEN") && pc == 6){

			log_info(logger,"PID: %d - Listo para ejecutar F_OPEN ", pid);

		}else if(!strcmp(strtok(instruccion_ejecutar->comando, "\n"),"YIELD") && pc == 7){

			log_info(logger,"PID: %d - Ejecutando YIELD ", pid);

			return YIELD;

		}else if(!strcmp(instruccion_ejecutar->comando,"F_TRUNCATE") && pc == 8){

			log_info(logger,"PID: %d - Listo para ejecutar F_TRUNCATE ", pid);

		}else if(!strcmp(instruccion_ejecutar->comando,"F_SEEK") && pc == 9){

			log_info(logger,"PID: %d - Listo para ejecutar F_SEEK ", pid);

		}else if(!strcmp(instruccion_ejecutar->comando,"CREATE_SEGMENT") && pc == 10){

			log_info(logger,"PID: %d - Listo para ejecutar CREATE_SEGMENT ", pid);

		}else if(!strcmp(instruccion_ejecutar->comando,"F_WRITE") && pc == 11){

			log_info(logger,"PID: %d - Listo para ejecutar F_WRITE ", pid);

		}else if(!strcmp(instruccion_ejecutar->comando,"F_READ") && pc == 12){

			log_info(logger,"PID: %d - Listo para ejecutar F_READ ", pid);

		}else if(!strcmp(instruccion_ejecutar->comando,"DELETE_SEGMENT") && pc == 13){

			log_info(logger,"PID: %d - Listo para ejecutar DELETE_SEGMENT ", pid);

		}else if(!strcmp(instruccion_ejecutar->comando,"F_CLOSE") && pc == 14){

			log_info(logger,"PID: %d - Listo para ejecutar F_CLOSE ", pid);

		}else if(!strcmp(instruccion_ejecutar->comando,"EXIT") && pc == 15){
			log_info(logger, "a morir proceso!!!!");
			//sleep(5);
			log_info(logger,"PID: %d - Ejecutando EXIT", pid);
			//limpiar_tlb();
			return EXIT;
		}else{
			//printf("Comando: %s | Par1: %s | Par2: %s | Par: %s\n\n", instruccion_ejecutar->comando, instruccion_ejecutar->parametro1, instruccion_ejecutar->parametro2 ,instruccion_ejecutar->parametro3);
			log_error(logger,"Hubo un error en el ciclo de instruccion, ");
		}
	return CONTINUE; // solo par aprobar la funcion
}


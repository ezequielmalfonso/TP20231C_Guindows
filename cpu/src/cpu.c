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
		estado = execute(instruccion_ejecutar,pcb->registro_cpu,pcb->pid);
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
	return (!strcmp(instruccion_ejecutar->comando,"SET")|| !strcmp(instruccion_ejecutar->comando,"ADD"));
}


int execute(INSTRUCCION* instruccion_ejecutar,char* registros,uint16_t pid){

	if(!strcmp(instruccion_ejecutar->comando,"SET")){
			log_info(logger,"Ejecutando SET parametro 1: %s parametro 2: %s",instruccion_ejecutar->parametro1,instruccion_ejecutar->parametro2);
	}
	return 1; // solo par aprobar la funcion
}


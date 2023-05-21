/*
 * planificacion.c
 *
 *  Created on: 16 abr 2023
 *      Author: utnso
 */


#include "planificacion.h"

pthread_mutex_t mx_cola_new 		= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_hay_interrupcion = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cola_ready 		= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cola_ready_sec	= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_lista_block 		= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_lista_new 		= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_log 				= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cola_blocked 	= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cpu_desocupado 	= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_memoria 			= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cpu 				= PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mx_pageFault 		= PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mx_interrupt 		= PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mx_hilo_pageFault 	= PTHREAD_MUTEX_INITIALIZER;

sem_t s_pasaje_a_ready, s_ready_execute,s_cpu_desocupado,s_cont_ready,s_multiprogramacion_actual,s_esperar_cpu,s_pcb_desalojado,s_blocked,s_io;
//sem_t s_ios;
t_queue* cola_new;
t_queue* cola_ready;
t_queue* cola_ready_sec;
t_list* list_blocked;

bool cpu_desocupado=true;

void fifo_ready_execute(){
	while(1){
	    //log_info(logger,"Entro al while");

	    sem_wait(&s_ready_execute);
	    sem_wait(&s_cpu_desocupado); // Para que no ejecute cada vez que un proceso llega a ready
	    sem_wait(&s_cont_ready); // Para que no intente ejecutar si la lista de ready esta vacia

	    // Pongo semaforo para asegurar que ningun otro proceso pueda acceder a la cola ready en este momento
	    pthread_mutex_lock(&mx_cola_ready);
	    PCB_t* proceso = queue_pop(cola_ready);
	    pthread_mutex_unlock(&mx_cola_ready);

	    log_info(logger,"PID: %d - Estado Anterior: READY - Estado Actual: EXECUTE", proceso->pid);

	   // pthread_mutex_lock(&mx_cpu);
	    send_proceso(cpu_fd, proceso,DISPATCH);
	  //  pthread_mutex_unlock(&mx_cpu);
	    pcb_destroy(proceso);
	    sem_post(&s_esperar_cpu);

	}
}

void inicializarPlanificacion(){
	cola_new		= queue_create();
	cola_ready		= queue_create();
	cola_ready_sec	= queue_create();

	sem_init(&s_ready_execute,0,0);
	sem_init(&s_cpu_desocupado, 0, 1);
	sem_init(&s_esperar_cpu, 0, 0);
	sem_init(&s_cont_ready,0,0);
	sem_init(&s_io, 0, 1);

/*	for(int i=0;i<10;i++){
		sem_init(&s_ios[i], 0, 1);
	}*/
	sem_init(&s_multiprogramacion_actual, 0, configuracion->GRADO_MAX_MULTIPROGRAMACION);
	pthread_t corto_plazo;

	if(!strcmp(configuracion->ALGORITMO_PLANIFICACION,"FIFO")){
		pthread_create(&corto_plazo, NULL, (void*) fifo_ready_execute, NULL);
		//pthread_mutex_lock(&mx_log);
		log_info(logger,"ALGORITMO_PLANIFICACION FIFOOO!!!!");
		//pthread_mutex_unlock(&mx_log);
	}else if(!strcmp(configuracion->ALGORITMO_PLANIFICACION,"HRRN")){
	//	pthread_create(&corto_plazo, NULL, (void*) hrrn_ready_execute, NULL);
		log_info(logger,"ALGORITMO_PLANIFICACION HRRN!!!!");
	}
	else{
		//pthread_mutex_lock(&mx_log);
		log_info(logger,"ALGORITMO_PLANIFICACION INVALIDO!!!!");
		//pthread_mutex_unlock(&mx_log);
	}
	pthread_t espera_CPU;
	pthread_create(&espera_CPU, NULL, (void*) esperar_cpu, NULL);
}


void esperar_cpu(){
	while(1){
		sem_wait(&s_esperar_cpu);
		op_code cop;
		PCB_t* pcb = pcb_create();
		//pthread_mutex_lock(&mx_cpu);
		//pthread_mutex_lock(&mx_pageFault);
		if (recv(cpu_fd, &cop, sizeof(op_code), 0) <= 0) {
			//pthread_mutex_lock(&mx_log);
			log_error(logger,"DISCONNECT FAILURE!");
			//pthread_mutex_unlock(&mx_log);
			exit(-1);
		}

		if (!recv_proceso(cpu_fd, pcb)) {
			//pthread_mutex_lock(&mx_log);
			log_error(logger,"Fallo recibiendo PROGRAMA %d", pcb->pid);
			//pthread_mutex_unlock(&mx_log);
			exit(-1);
		}
		//pthread_mutex_lock(&mx_cpu_desocupado);
		//cpu_desocupado = true;
		//pthread_mutex_unlock(&mx_cpu_desocupado);
		switch (cop) {
			case EXIT:
				send(pcb->cliente_fd,&cop,sizeof(op_code),0);
				execute_a_exit(pcb);
				sem_post(&s_cpu_desocupado);
				sem_post(&s_ready_execute);
				break;
			case WAIT:
				// preguntar si va estar siemrpe en el mismo orden la lista de instrucciones
				// SI NO usar un while
				 INSTRUCCION* instruccion = list_get(pcb->instrucciones, 2);
				 log_info(logger, "PID: %d - Recibo pedido de WAIT por RECURSO: %s", pcb->pid, instruccion->parametro1 );
				 break;

			case YIELD:
				 log_info(logger, "Recibi YIELD de CPU lo mandamos al final de la cola READY");
				 log_info("Valor del PC: %d", pcb->pc);
				 pthread_mutex_lock(&mx_cola_ready);
				 queue_push(cola_ready,pcb);
				 pthread_mutex_unlock(&mx_cola_ready);

				 sem_post(&s_cont_ready);
			//	 sem_post(&s_pcb_desalojado);
				 sem_post(&s_ready_execute);
				 sem_post(&s_cpu_desocupado);
				 break;

			case INTERRUPT: // para hrrn

					break;

			case IO:
				pthread_t hilo_bloqueado;
				sem_post(&s_blocked);
				pthread_create(&hilo_bloqueado,NULL,(void*)bloqueando,pcb);
				pthread_detach(hilo_bloqueado);								// Hace y me aseguro el hilo no se va a joinear con el hilo principal
				 //pthread_mutex_lock(&mx_log);
				log_info(logger, "PID: %d - Estado Anterior: EXECUTE - Estado Actual: BLOCKED", pcb->pid);
				 //pthread_mutex_unlock(&mx_log);
				sem_post(&s_cpu_desocupado);
				break;


			default:
				log_error(logger, "AAAlgo anduvo mal en el server del kernel\n Cop: %d",cop);
		}
	}
}

void execute_a_exit(PCB_t* pcb){
	//pthread_mutex_lock(&mx_log);
    log_info(logger,"PID: %d - Estado Anterior: EXECUTE - Estado Actual: EXIT", pcb->pid);
    //pthread_mutex_unlock(&mx_log);
    sem_post(&s_multiprogramacion_actual);//cuando se finaliza
    //liberar_espacio_de_memoria(PCB); Liberamos las estructructuras de memoria
    pcb_destroy(pcb);
    //avisar_consola_finalizacion(); Funcion que le avisa a la consola que se finalizo correctamente
}

void bloqueando(PCB_t* pcb){
	int i = 0;
	op_code cop;
	sem_wait(&s_blocked);
	INSTRUCCION* inst = list_get(pcb->instrucciones, pcb->pc - 1);
	//pthread_mutex_lock(&mx_log);
	log_info(logger, "instruccion numer %d",(pcb->pc-1));
	//pthread_mutex_unlock(&mx_log);
	if(!strcmp(inst->comando,"I/O")){ // HAY QUE VER COMO METERSE EN ESE CHAR ** DISPOSITIVOS IO PARA QUE HAGA EL STRCMP
		//pthread_mutex_lock(&mx_log);
		log_info(logger, " me meti al if");
		//pthread_mutex_unlock(&mx_log);
		//sem_wait(&s_ios[i]);
		sem_wait(&s_io);
		ejecutar_io(pcb,i);
	}
}

void ejecutar_io(PCB_t* pcb,int numero) {
		//pthread_mutex_lock(&mx_log);
		log_info(logger, " ejecutar io");
		//pthread_mutex_unlock(&mx_log);

		//pthread_mutex_lock(&mx_cola_blocked);
		/*if (list_size(cola_blocked) == 0){
			//pthread_mutex_lock(&mx_log);
			log_error(logger,"Blocked ejecutó sin un proceso bloqueado");
			//pthread_mutex_unlock(&mx_log);
		}*/

		INSTRUCCION* inst = list_get(pcb->instrucciones, pcb->pc - 1); //-1 porque ya se incremento el PC
		uint32_t tiempo = atoi(inst->parametro2);
		//pthread_mutex_lock(&mx_log);
		log_info(logger, " PID: %d - Bloqueado por: %s durante: %s", pcb->pid, inst->comando, inst->parametro1);
		//pthread_mutex_unlock(&mx_log);
		usleep(tiempo * 1000);
		//pthread_mutex_lock(&mx_log);
		log_info(logger, "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY", pcb->pid);
		//pthread_mutex_unlock(&mx_log);
		pthread_mutex_lock(&mx_cola_ready);
		queue_push(cola_ready, pcb);
		pthread_mutex_unlock(&mx_cola_ready);
		sem_post(&s_ready_execute);
		sem_post(&s_cont_ready);
		sem_post(&s_io);
}



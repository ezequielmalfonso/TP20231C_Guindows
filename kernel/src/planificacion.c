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
sem_t s_blocked_rec;
t_queue* cola_new;
t_queue* cola_ready;
t_queue* cola_ready_sec;
t_list* list_blocked;
t_tiempos_rafaga_anterior raf_anterior;
t_list* list_rafa_anterior;

bool cpu_desocupado=true;

void fifo_ready_execute(){
	while(1){
	    //log_info(logger,"PID: fifo_ready_execute");
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

void hrrn_ready_execute(){

	while(1){
		    log_info(logger,"PID: hrrn_ready_execute");
		    sem_wait(&s_ready_execute);
		    sem_wait(&s_cpu_desocupado); // Para que no ejecute cada vez que un proceso llega a ready
		    sem_wait(&s_cont_ready); // Para que no intente ejecutar si la lista de ready esta vacia

		    ordenar_hrrn(cola_ready);

		    pthread_mutex_lock(&mx_cola_ready);
		    PCB_t* proceso = queue_pop(cola_ready);
		    pthread_mutex_unlock(&mx_cola_ready);

		    t_link_element* aux_list_raf_ant = list_rafa_anterior->head;

		    while( aux_list_raf_ant!=NULL )
		   	{
		   	   t_tiempos_rafaga_anterior* aux_list_raf_ant2 = aux_list_raf_ant->data;

		   	   if( aux_list_raf_ant2->pid ==  proceso->pid)
		   	   {
		   		 aux_list_raf_ant2->tiempo_in_exec =  temporal_gettime(reloj_inicio);
		   		 break;

		   	   } else if (aux_list_raf_ant->next == NULL){

		   		   raf_anterior.pid = proceso->pid;
		   		   raf_anterior.tiempo_in_exec = temporal_gettime(reloj_inicio);
		   		   raf_anterior.tiempo_out_exec = 0;

		   		   list_add(aux_list_raf_ant2, &raf_anterior);
		   	   }
		   	   aux_list_raf_ant = aux_list_raf_ant->next;
		   	}

		    log_info(logger,"PID: %d - Estado Anterior: READY - Estado Actual: EXECUTE", proceso->pid);

		   // pthread_mutex_lock(&mx_cpu);
		    send_proceso(cpu_fd, proceso,DISPATCH);
		  //  pthread_mutex_unlock(&mx_cpu);
		    pcb_destroy(proceso);
		    sem_post(&s_esperar_cpu);
		}
}

void ordenar_hrrn(t_queue *cola_ready){

	t_list* listaReady = malloc(sizeof(t_list));
	listaReady = list_create();

	PCB_t* proceso;
	int i = 0;

	while(i < queue_size(cola_ready)){

		proceso = queue_pop(cola_ready);
		list_add(listaReady,proceso);

		i++;
	}

	list_sort(listaReady, menor);

	 t_link_element* aux_proc1 = listaReady->head;

	while( aux_proc1!=NULL )
	{
	   PCB_t* aux_proc = aux_proc1->data;
	   queue_push(cola_ready, aux_proc);
	   aux_proc1 = aux_proc1->next;
	}


}

bool menor(PCB_t* a, PCB_t* b){
	//TODO SJF FORMULA???????
	// S = α . estimadoAnterior + (1 - α) . ráfagaAnterior

	double sa = obtenerEstimadoRafaga(a, configuracion->ESTIMACION_INICIAL, configuracion->HRRN_ALFA);
	double sb = obtenerEstimadoRafaga(b, configuracion->ESTIMACION_INICIAL, configuracion->HRRN_ALFA);
    int wa = temporal_gettime(reloj_inicio) - a->tiempo_llegada_a_ready;
    int wb = temporal_gettime(reloj_inicio) - b->tiempo_llegada_a_ready;

	//TODO HRRN FORMULA??????
	//R.R. = (S + W(tiempo de espera en ready (el actual - el q esta en el pcb))) / S  = 1 + W/S - Donde S = Ráfaga estimada y W = Tiempo de espera
	//obtenerRatio();

	return 1 / (wa + sa) < 1 / (wb + sb);
}

double obtenerEstimadoRafaga(PCB_t* a, uint32_t estimadoInicial, double alfa){

	// S = α . estimadoAnterior + (1 - α) . ráfagaAnterior
	double s;

	if(a->estimado_proxima_rafaga == 0){
		s = alfa * estimadoInicial  + 0;
	}
	else{
		// Busco el nodo con el pid
		int tiempo_in_exec, tiempo_out_exec;
		 t_link_element* aux_list_raf_ant = list_rafa_anterior->head;

				    while( aux_list_raf_ant!=NULL )
				   	{
				   	   t_tiempos_rafaga_anterior* aux_list_raf_ant2 = aux_list_raf_ant->data;

				   	   if( aux_list_raf_ant2->pid == a->pid)
				   	   {
				   		 tiempo_in_exec = aux_list_raf_ant2->tiempo_in_exec;
				   		 tiempo_out_exec = aux_list_raf_ant2->tiempo_out_exec;
				   		 break;

				   	   }
				   	   aux_list_raf_ant = aux_list_raf_ant->next;
				   	}
		int duracionUltimaRafaga = tiempo_out_exec - tiempo_in_exec;
		s = alfa * a->estimado_proxima_rafaga + (1-alfa) * duracionUltimaRafaga;

	}
	return s;
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
		pthread_create(&corto_plazo, NULL, (void*) hrrn_ready_execute, NULL);
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
		sem_wait(&s_esperar_cpu); //--> se bloquea
		op_code cop;
		PCB_t* pcb         = pcb_create();
		PCB_t* pcb_blocked = pcb_create();
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

		//log_info(logger, "Pid: %d, %d ", pcb->pid, cop);
		INSTRUCCION* instruccion = malloc(sizeof(INSTRUCCION));
		t_link_element* aux_list_raf_ant = list_rafa_anterior->head;

		switch (cop) {
			case EXIT:
				send(pcb->cliente_fd,&cop,sizeof(op_code),0);
				execute_a_exit(pcb);
				sem_post(&s_cpu_desocupado);
				sem_post(&s_ready_execute);
				break;
			case WAIT:
				// preguntar si va estar siempre en el mismo orden la lista de instrucciones
				// SI NO usar un while
				 instruccion = list_get(pcb->instrucciones, 2);      // La instruccion 2 es el WAIT
				 log_info(logger, "PID: %d - Recibo pedido de WAIT por RECURSO: %s", pcb->pid, instruccion->parametro1 );

				 t_link_element* aux_rec1 = lista_de_recursos->head;
				 //uint16_t instancias;
				 int pos_recurso = 0;
				 //log_info(logger, "XXXXX RECURSO: %s", instruccion->parametro1 );
				 while( aux_rec1!=NULL )
				 {
					 t_recurso* aux_rec2 = aux_rec1->data;
					 if(strcmp(aux_rec2->recurso, instruccion->parametro1))
					 {
						 log_info(logger, "Para RECURSO %s hay %d instancias disponibles antes de ejecutar", aux_rec2->recurso, aux_rec2->instancias );
						 //instancias = aux_rec2->instancias;

						 if(aux_rec2->instancias > 0)
						 { // Si entra es pq va ejecutar la instancia del recurso y resto 1 a la instancia
							 aux_rec2->instancias -= 1;
							 log_info(logger,"PID: %d - Wait: %s - Instancias: %d ", pcb->pid, strtok(instruccion->parametro1, "\n"), aux_rec2->instancias  );

							 pcb->tiempo_llegada_a_ready = temporal_gettime(reloj_inicio);

							 pthread_mutex_lock(&mx_cola_ready);  // TODO hacer mas pruebas
							 send_proceso(cpu_fd, pcb,DISPATCH);
							 pthread_mutex_unlock(&mx_cola_ready);

							 sem_post(&s_ready_execute);
							 sem_post(&s_cpu_desocupado);
							 sem_post(&s_esperar_cpu);

						 }else{
							 aux_rec2->instancias -= 1;

							 //------
							 //t_link_element* aux_list_raf_ant = list_rafa_anterior->head;

							while( aux_list_raf_ant!=NULL )
							{
							   t_tiempos_rafaga_anterior* aux_list_raf_ant2 = aux_list_raf_ant->data;

							   if( aux_list_raf_ant2->pid ==  pcb->pid)
							   {
								 aux_list_raf_ant2->tiempo_out_exec =  temporal_gettime(reloj_inicio);
								 break;
							   }
							   aux_list_raf_ant = aux_list_raf_ant->next;
							}
							 //---

							 log_info(logger,"PID: %d - Estado Anterior: EXECUTE - Estado Actual: BLOCKED por  Wait: %s - Instancias: %d - PC: %d", pcb->pid, strtok(instruccion->parametro1, "\n"), aux_rec2->instancias , pcb->pc );

							 pthread_mutex_lock(&mx_cola_blocked);
							 queue_push(aux_rec2->cola_bloqueados_recurso,pcb);
							 pthread_mutex_unlock(&mx_cola_blocked);

							 sem_post(&s_ready_execute);
							 sem_post(&s_cpu_desocupado);
							 sem_post(&s_esperar_cpu);

						 }
						 break;
					 }

					 aux_rec1 = aux_rec1->next;
					 pos_recurso++;
				 }

				 break;

			case SIGNAL:
				instruccion = list_get(pcb->instrucciones, 4);      // La instruccion 4 es el SIGNAL
				log_info(logger, "PID: %d - Recibo pedido de SIGNAL por RECURSO: %s", pcb->pid, instruccion->parametro1 );

				t_link_element* aux_rec1_s = lista_de_recursos->head;
				// uint16_t instancias;
				int pos_recurso_s = 0;

				while(aux_rec1_s != NULL)
				{
					t_recurso* aux_rec2_s = aux_rec1_s->data;
					if(strcmp(aux_rec2_s->recurso, instruccion->parametro1))
					{
					  log_info(logger, "Para RECURSO %s hay %d instancias disponibles antes de ejecutar", aux_rec2_s->recurso, aux_rec2_s->instancias );
					  //instancias = aux_rec2->instancias;

					  aux_rec1_s = aux_rec1_s->next;
					  pos_recurso++;

					  if(aux_rec2_s->instancias < 0)
					  {
						 log_info(logger, "entro al if instancias");
						 aux_rec2_s->instancias += 1;
						 log_info(logger,"PID: %d - SIGNAL: %s - Instancias: %d ", pcb->pid, strtok(instruccion->parametro1, "\n"), aux_rec2_s->instancias  );

						 if(!queue_is_empty(aux_rec2_s->cola_bloqueados_recurso))
						 {
							 pthread_mutex_lock(&mx_cola_blocked);
							 pcb_blocked = queue_pop(aux_rec2_s->cola_bloqueados_recurso);
							 pthread_mutex_unlock(&mx_cola_blocked);
							 log_info(logger, "PID: %d que se  desbloqueo", pcb_blocked->pid );
						 }else{
							 log_error(logger, "Ocurrio un error con las instancias del recurso %d: ", pcb->pid);
						 }

						///******** pcb_blocked->pc --;   // TODO depende lo que responda se deja o se saca
						 log_info(logger, "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY - PROGRAM COINTER: %d", pcb_blocked->pid, pcb_blocked->pc );

						 pcb_blocked->tiempo_llegada_a_ready = temporal_gettime(reloj_inicio);

						 pthread_mutex_lock(&mx_cola_ready);
						 queue_push(cola_ready,pcb_blocked);
						 send_proceso(cpu_fd, pcb,DISPATCH);
						 pthread_mutex_unlock(&mx_cola_ready);

						 sem_post(&s_cont_ready);
					     sem_post(&s_ready_execute);
						 sem_post(&s_cpu_desocupado);
						 sem_post(&s_esperar_cpu);

					 }else{
						 aux_rec2_s->instancias += 1;
						 log_info(logger,"PID: %d - SIGNAL: %s - Instancias: %d ", pcb->pid, strtok(instruccion->parametro1, "\n"), aux_rec2_s->instancias  );



						 pthread_mutex_lock(&mx_cola_ready);  // TODO hacer mas pruebas
						 send_proceso(cpu_fd, pcb,DISPATCH);
					     pthread_mutex_unlock(&mx_cola_ready);

						 sem_post(&s_ready_execute);
						 sem_post(&s_cpu_desocupado);
						 sem_post(&s_esperar_cpu);

					   }
					}
					 break;
					 aux_rec1 = aux_rec1->next;
					 pos_recurso++;
				}

				break;

			case YIELD:
				 log_info(logger, "PID: %d - Recibi YIELD de CPU lo mandamos al final de la cola READY", pcb->pid);
				// log_info("Valor del PC: %d", pcb->pc);

				 pcb->tiempo_llegada_a_ready = temporal_gettime(reloj_inicio);

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

				 //------
				// t_link_element* aux_list_raf_ant = list_rafa_anterior->head;

				while( aux_list_raf_ant!=NULL )
				{
				   t_tiempos_rafaga_anterior* aux_list_raf_ant2 = aux_list_raf_ant->data;

				   if( aux_list_raf_ant2->pid ==  pcb->pid)
				   {
					 aux_list_raf_ant2->tiempo_out_exec =  temporal_gettime(reloj_inicio);
					 break;
				   }
				   aux_list_raf_ant = aux_list_raf_ant->next;
				}
				 //---

				//pthread_mutex_lock(&mx_log);
				log_info(logger, "PID: %d - Estado Anterior: EXECUTE - Estado Actual: BLOCKED por I/O", pcb->pid);
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
		//log_info(logger, " ejecutar io");
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

		pcb->tiempo_llegada_a_ready = temporal_gettime(reloj_inicio);

		pthread_mutex_lock(&mx_cola_ready);
		queue_push(cola_ready, pcb);
		pthread_mutex_unlock(&mx_cola_ready);
		sem_post(&s_ready_execute);
		sem_post(&s_cont_ready);
		sem_post(&s_io);
		//log_info(logger, "PID: %d - probando como queda", pcb->pid);
}



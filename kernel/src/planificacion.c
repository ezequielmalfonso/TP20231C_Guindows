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
pthread_mutex_t mx_instancias       = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mx_pageFault 		= PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mx_interrupt 		= PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mx_hilo_pageFault 	= PTHREAD_MUTEX_INITIALIZER;

sem_t s_pasaje_a_ready, s_ready_execute,s_cpu_desocupado,s_cont_ready,s_multiprogramacion_actual,s_esperar_cpu,s_pcb_desalojado,s_blocked,s_io;
sem_t s_blocked_rec;
t_queue* cola_new;
t_queue* cola_ready;
t_queue* cola_ready_sec;
t_list* list_blocked;
//t_tiempos_rafaga_anterior raf_anterior;
t_list* list_rafa_anterior;
int64_t reloj = 1;

bool cpu_desocupado=true;

t_list* tabla_global_archivos;
char* archivoABuscar;
t_archivo_abierto* archivo;
t_archivoAbierto* archivo_proceso;

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
		//log_info(logger,"PID: hrrn_ready_execute");
		sem_wait(&s_ready_execute);
		sem_wait(&s_cpu_desocupado); // Para que no ejecute cada vez que un proceso llega a ready
		sem_wait(&s_cont_ready); // Para que no intente ejecutar si la lista de ready esta vacia

		reloj = temporal_gettime(reloj_inicio);
		ordenar_hrrn(cola_ready);

		pthread_mutex_lock(&mx_cola_ready);
		PCB_t* proceso = queue_pop(cola_ready);
		pthread_mutex_unlock(&mx_cola_ready);

		// Busco el pid en la lista de rafagas y guardo el tiempo en el que pasa al cpu
		t_tiempos_rafaga_anterior* tiempos = list_get(list_rafa_anterior, proceso->pid);	// se agregan a la lista cuando se crean entonces estan ordenados por pcb
		tiempos->tiempo_in_exec = reloj;

		log_info(logger,"PID: %d - Estado Anterior: READY - Estado Actual: EXECUTE hrrn", proceso->pid);
	   // pthread_mutex_lock(&mx_cpu);
		send_proceso(cpu_fd, proceso,DISPATCH);
	  //  pthread_mutex_unlock(&mx_cpu);
		pcb_destroy(proceso);
		sem_post(&s_esperar_cpu);
	}
}


void ordenar_hrrn(t_queue *cola_ready){

	//log_info(logger, "estoy ordenando amigo");
	t_list* listaReady = malloc(sizeof(t_list));
	listaReady = list_create();

	PCB_t* proceso;
	int i = 0;
	//log_warning(logger, "lolo: %d", queue_size(cola_ready));

	//pthread_mutex_lock(&mx_cola_ready);
	int queuesize = queue_size(cola_ready);
	//pthread_mutex_lock(&mx_cola_ready);

	while(i < queuesize){

		pthread_mutex_lock(&mx_cola_ready);
		proceso = queue_pop(cola_ready);
		pthread_mutex_unlock(&mx_cola_ready);
		//log_warning(logger, "lolo: %d", queue_size(cola_ready));
		list_add(listaReady, proceso);
		//log_info(logger, "agrego");
		i++;
	}
	bool (*a)(void* x, void* y) = menor;
	list_sort(listaReady, a);

	 t_link_element* aux_proc1 = listaReady->head;

	while( aux_proc1 != NULL)
	{
	   PCB_t* aux_proc = aux_proc1->data;
	   pthread_mutex_lock(&mx_cola_ready);
	   queue_push(cola_ready, aux_proc);
	   pthread_mutex_unlock(&mx_cola_ready);
	   aux_proc1 = aux_proc1->next;
	}
	//list_destroy(listaReady);

}

bool menor(PCB_t* a, PCB_t* b){
	// S = α . estimadoAnterior + (1 - α) . ráfagaAnterior

	double sa = a->estimado_proxima_rafaga;	// El estimador se calcula cada vez que sale un proceso de execute (wait, yield, io)
	double sb = b->estimado_proxima_rafaga;
    int wa = (reloj - a->tiempo_llegada_a_ready);
    int wb = (reloj - b->tiempo_llegada_a_ready);

	//R.R. = (S + W(tiempo de espera en ready (el actual - el q esta en el pcb))) / S  = 1 + W/S - Donde S = Ráfaga estimada y W = Tiempo de espera
	//obtenerRatio();

	return (1 + (wa / sa)) > (1 + (wb / sb));
}

double obtenerEstimadoRafaga(PCB_t* a, uint32_t estimadoInicial, double alfa){
	// S = α . estimadoAnterior + (1 - α) . ráfagaAnterior
	double s;

	// Busco el nodo con el pid
	int tiempo_in_exec, tiempo_out_exec;
	t_tiempos_rafaga_anterior* tiempos = list_get(list_rafa_anterior, a->pid);
	tiempo_in_exec = tiempos->tiempo_in_exec;
	tiempo_out_exec = tiempos->tiempo_out_exec;

	int duracionUltimaRafaga = (tiempo_out_exec - tiempo_in_exec);
	s = alfa * a->estimado_proxima_rafaga + (1-alfa) * duracionUltimaRafaga;
	//log_warning(logger, "El s es: %f, el estimado anteiror era: %f y alfa= %f", s, a->estimado_proxima_rafaga, alfa);

	a->estimado_proxima_rafaga = s;
	return s;
}

void inicializarPlanificacion(){
	cola_new		= queue_create();
	cola_ready		= queue_create();
	cola_ready_sec	= queue_create();

	tabla_global_archivos = list_create();

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
		instruccion = list_get(pcb->instrucciones, pcb->pc-1);
		t_link_element* aux_list_raf_ant = list_rafa_anterior->head;	// deberia ir cada vez que la necesito y listo
		bool recurso_existe = false;
		char* motivoExit;
		op_code mensaje;

		switch (cop) {

			case WAIT:
				 log_info(logger, "PID: %d - Recibo pedido de WAIT por RECURSO: %s", pcb->pid, instruccion->parametro1 );


				 t_link_element* aux_rec1 = lista_de_recursos->head;
				 //uint16_t instancias;
				 int pos_recurso = 0;
				 //log_info(logger, "XXXXX RECURSO: %s", instruccion->parametro1 );
				 while( aux_rec1!=NULL )
				 {
					 t_recurso* aux_rec2 = aux_rec1->data;
					 //log_info(logger,"llegue: recurso 0%s0, parametro 0%s0, 0%s0", aux_rec2->recurso, strtok(instruccion->parametro1, "\n"), strtok(strtok(instruccion->parametro1, "\n"), "\n"));
					 if(	!strcmp(aux_rec2->recurso, strtok(instruccion->parametro1, "\r")) ||	// Soluciona error de recurso no encontrado. Habria que sanitizar mejor los datos en consola?
							!strcmp(aux_rec2->recurso, strtok(instruccion->parametro1, "\n")))
					 {
						 recurso_existe = true;
						 log_info(logger, "Para RECURSO %s hay %d instancias disponibles antes de ejecutar", aux_rec2->recurso, aux_rec2->instancias );
						 //instancias = aux_rec2->instancias;

						 if(aux_rec2->instancias > 0)
						 { // Si entra es pq va ejecutar la instancia del recurso y resto 1 a la instancia
							 pthread_mutex_lock(&mx_instancias);	// TODO: hacen falta estos semaforos? se ejecutan de a una
							 aux_rec2->instancias -= 1;
							 pthread_mutex_unlock(&mx_instancias);

							 log_info(logger,"PID: %d - Wait: %s - Instancias: %d ", pcb->pid, strtok(instruccion->parametro1, "\n"), aux_rec2->instancias  );

							 // no pasa por ready, vuelve de una al cpu
							 //pcb->tiempo_llegada_a_ready = temporal_gettime(reloj_inicio);

							 pthread_mutex_lock(&mx_cola_ready);  // TODO hacer mas pruebas
							 send_proceso(cpu_fd, pcb,DISPATCH);
							 pthread_mutex_unlock(&mx_cola_ready);

							 sem_post(&s_ready_execute);
							 sem_post(&s_cpu_desocupado);
							 sem_post(&s_esperar_cpu);

						 }else{
							 pthread_mutex_lock(&mx_instancias);
							 aux_rec2->instancias -= 1;  //TODO PREGUNTAR!!!
							 pthread_mutex_unlock(&mx_instancias);
							 //------

							 t_tiempos_rafaga_anterior * tiemposRafaga = list_get(list_rafa_anterior, pcb->pid);
							 tiemposRafaga->tiempo_out_exec = temporal_gettime(reloj_inicio);
							 double estimadorNuevo = obtenerEstimadoRafaga(pcb, configuracion->ESTIMACION_INICIAL, configuracion->HRRN_ALFA);
							 pcb->estimado_proxima_rafaga = estimadorNuevo;
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
				 if(recurso_existe) {	// Si es null no se encontro el recurso
					 break;
				 } else {
					 log_error(logger, "PID: %d - Recibo pedido de WAIT por recurso desconocido: %s", pcb->pid, instruccion->parametro1);
					 cop = EXIT;	// Lo mismo que un exit
					 send(pcb->cliente_fd,&cop,sizeof(op_code),0);
					 motivoExit = "WAIT por Recurso Inexistente";
					 execute_a_exit(pcb,motivoExit);
					 sem_post(&s_cpu_desocupado);
					 sem_post(&s_ready_execute);
					 break;
				 }

			case SIGNAL:
				log_info(logger, "PID: %d - Recibo pedido de SIGNAL por RECURSO: %s", pcb->pid, instruccion->parametro1 );

				t_link_element* aux_rec1_s = lista_de_recursos->head;
				// uint16_t instancias;
				int pos_recurso_s = 0;

				while(aux_rec1_s != NULL)
				{
					t_recurso* aux_rec2_s = aux_rec1_s->data;

					 if(	!strcmp(aux_rec2_s->recurso, strtok(instruccion->parametro1, "\r")) || // Soluciona error de recurso no encontrado. Habria que sanitizar mejor los datos en consola?
							!strcmp(aux_rec2_s->recurso, strtok(instruccion->parametro1, "\n")))
					 {
						 recurso_existe = 1;
						 log_info(logger, "Para RECURSO %s hay %d instancias disponibles antes de ejecutar", aux_rec2_s->recurso, aux_rec2_s->instancias );
						 //instancias = aux_rec2->instancias;

						 aux_rec1_s = aux_rec1_s->next;
						 pos_recurso++;

						 if(aux_rec2_s->instancias < 0)
						 {
							 log_info(logger, "entro al if instancias");

							 pthread_mutex_lock(&mx_instancias);
							 aux_rec2_s->instancias += 1;
							 pthread_mutex_unlock(&mx_instancias);

							 log_info(logger,"PID: %d - SIGNAL: %s - Instancias: %d ", pcb->pid, strtok(instruccion->parametro1, "\n"), aux_rec2_s->instancias  );

							 if(!queue_is_empty(aux_rec2_s->cola_bloqueados_recurso))
							 {
								 pthread_mutex_lock(&mx_cola_blocked);
								 pcb_blocked = queue_pop(aux_rec2_s->cola_bloqueados_recurso);
								 pthread_mutex_unlock(&mx_cola_blocked);
								 log_info(logger, "PID: %d que se  desbloqueo", pcb_blocked->pid );
							 }else {
								 log_error(logger, "Ocurrio un error con las instancias del recurso %d: ", pcb->pid);
							 }
/***********************************************************/
						 //pcb_blocked->pc--;   // TODO depende lo que responda se deja o se saca
/***********************************************************/
							 // TODO para listar los pids despues de entrar a ready
							 char* pids = procesosEnReady(cola_ready);
							 log_info(logger, "Ingreso a Ready algoritmo %s - PIDS: [%s] ", configuracion->ALGORITMO_PLANIFICACION, pids);

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

						 } else{
							 pthread_mutex_lock(&mx_instancias);
							 aux_rec2_s->instancias += 1;
							 pthread_mutex_unlock(&mx_instancias);
							 log_info(logger,"PID: %d - SIGNAL: %s - Instancias: %d ", pcb->pid, strtok(instruccion->parametro1, "\n"), aux_rec2_s->instancias  );

							 pthread_mutex_lock(&mx_cola_ready);
							 send_proceso(cpu_fd, pcb,DISPATCH);
							 pthread_mutex_unlock(&mx_cola_ready);

							 sem_post(&s_ready_execute);
							 sem_post(&s_cpu_desocupado);
							 sem_post(&s_esperar_cpu);

						   }
						 break;
					 }
					 aux_rec1_s = aux_rec1_s->next;
					 pos_recurso++;
				}

				if(recurso_existe) {	// Si es null no se encontro el recurso
					break;
				} else {
					log_error(logger, "PID: %d - Recibo pedido de SIGNAL por recurso desconocido: %s", pcb->pid, instruccion->parametro1);
					cop = EXIT;	// no hay break asi que sigue hasta que lo encuentra (sin comparar case)
					send(pcb->cliente_fd,&cop,sizeof(op_code),0);
					motivoExit = "SIGNAL por Recurso Inexistente";
					execute_a_exit(pcb,motivoExit);
					sem_post(&s_cpu_desocupado);
					sem_post(&s_ready_execute);
					break;
				}

			case EXIT:	// Segundo exit
				send(pcb->cliente_fd,&cop,sizeof(op_code),0);
				motivoExit = "POR FIN";
				execute_a_exit(pcb,motivoExit);
				sem_post(&s_cpu_desocupado);
				sem_post(&s_ready_execute);
				break;

			case YIELD:
				 log_info(logger, "PID: %d - Recibi YIELD de CPU lo mandamos al final de la cola READY", pcb->pid);
				// log_info("Valor del PC: %d", pcb->pc);
				 int tiempo = temporal_gettime(reloj_inicio);
				 t_tiempos_rafaga_anterior* lista = list_get(list_rafa_anterior, pcb->pid);
				 lista->tiempo_out_exec = tiempo;
				 double estimadorNuevo = obtenerEstimadoRafaga(pcb, configuracion->ESTIMACION_INICIAL, configuracion->HRRN_ALFA);
				 pcb->estimado_proxima_rafaga = estimadorNuevo;
				 /*
				 t_link_element* aux_list_raf_ant = list_rafa_anterior->head;
				 while( aux_list_raf_ant!=NULL )
				{
				   t_tiempos_rafaga_anterior* aux_list_raf_ant2 = aux_list_raf_ant->data;

				   if( aux_list_raf_ant2->pid == pcb->pid)
				   {
					 aux_list_raf_ant2->tiempo_out_exec = temporal_gettime(reloj_inicio);
					 pcb->estimado_proxima_rafaga = obtenerEstimadoRafaga(pcb, configuracion->ESTIMACION_INICIAL, configuracion->HRRN_ALFA);
					 log_warning(logger, "guardo el valor de salida por yield de PID: %d", pcb->pid);
					 break;
				   }
				   aux_list_raf_ant = aux_list_raf_ant->next;
				}*/

				 pcb->tiempo_llegada_a_ready = tiempo;

				 pthread_mutex_lock(&mx_cola_ready);
				 queue_push(cola_ready,pcb);
				 pthread_mutex_unlock(&mx_cola_ready);

				 // TODO para listar los pids despues de entrar a ready
								 char* pids = procesosEnReady(cola_ready);
								 log_info(logger, "Ingreso a Ready algoritmo %s - PIDS: [%s] ", configuracion->ALGORITMO_PLANIFICACION, pids);


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
					 double estimadorNuevo = obtenerEstimadoRafaga(pcb, configuracion->ESTIMACION_INICIAL, configuracion->HRRN_ALFA);
					 pcb->estimado_proxima_rafaga = estimadorNuevo;
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

			case F_OPEN:
				log_info(logger, "PID: %d - Recibo pedido de F_OPEN por: %s", pcb->pid, instruccion->parametro1);
				// LOG obligatorio
				log_info(logger, "PID: %d - Abrir Archivo: %s", pcb->pid, instruccion->parametro1);

				// Verifico si la tabla global esta vacia
				if(!list_is_empty(tabla_global_archivos)){
					// CONSULTAMOS SI EL ARCHIVO ESTA ABIERTO (=EN TABLA)
					log_warning(logger, "Hay elementos en la tabla global de archivos. Busco el %s", instruccion->parametro1);
					archivoABuscar = strtok(instruccion->parametro1, "\n");	// global
					bool (*aux)(void* x) = criterio_nombre_archivo;
					archivo = list_find(tabla_global_archivos, aux);	// Devuelvo el elemento de la tabla con el nombre buscado
				}else{
					archivo = NULL;
					log_warning(logger, "La tabla global de archivos esta vacia");
				}

				// Si el archivo esta abierto (=EN TABLA)
				if(archivo)
				{ log_warning(logger, "El archivo %s ya estaba abierto en la TGA",instruccion->parametro1);
					// SI esta abierto en la global: bloquemos proceso en cola de proceso bloqueados del archivo
					//TODO tiempos HRRN

					// cargamos en la list de archivos abiertos por el proceso
					archivo_proceso = malloc(sizeof(t_archivoAbierto));
					//archivo_proceso->nombre_archivo = strtok(instruccion->parametro1, "\n");
					strcpy(archivo_proceso->nombre_archivo, strtok(instruccion->parametro1,"\n"));
					archivo_proceso->puntero = 0;
					list_add(pcb->archivos_abiertos, archivo_proceso);
					log_info(logger, "PID: %d - Se agrego %s a la tabla de archivos abiertos (ya en TGA)", pcb->pid, archivo_proceso->nombre_archivo);

					// El proceso se bloquea hasta que se libere el archivo (fclose)
					log_warning(logger, "PID: %d - Bloqueado esperando archivo %s", pcb->pid, instruccion->parametro1);
					queue_push(archivo->c_proc_bloqueados,pcb);


				}else { // El archivo no esta abierto
					log_warning(logger, "Archivo %s no esta abierto",instruccion->parametro1);
					send_archivo(file_system_fd,instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, F_EXISTS);
					recv(file_system_fd, &mensaje , sizeof(op_code), 0);

					log_warning(logger, "Existencia=%d", mensaje);

					// Reviso si el archivo existe en el fs
					if(mensaje)
					{
						// El archivo existe -> lo abro
						send_archivo(file_system_fd,instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, F_OPEN);
						recv(file_system_fd, &mensaje , sizeof(op_code), 0);

						if(mensaje == F_OPEN_OK) {
							t_archivo_abierto* archivo_nuevo = malloc(sizeof(t_archivo_abierto));
							archivo_nuevo->nombre = strtok(instruccion->parametro1, "\n");
							archivo_nuevo->c_proc_bloqueados = queue_create();
							list_add(tabla_global_archivos, archivo_nuevo);
							log_warning(logger, "El archivo %s fue abierto y cargado a la tabla global de archivos abiertos", instruccion->parametro1);
						} else{
							log_error(logger, "PID: %d - F_OPEN ERROR: El archivo %s existe pero no se pudo abrir", pcb->pid, instruccion->parametro1);
							// no deberia entrar nunca
						}

					} else{	// El archivo no existe -> lo creo
						log_info(logger, "PID: %d - Recibo pedido de F_CREATE por archivo inexistente: %s", pcb->pid, instruccion->parametro1);
						send_archivo(file_system_fd,instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, F_CREATE);
						recv(file_system_fd, &mensaje , sizeof(op_code), 0);

						if(mensaje == F_CREATE_OK)
						{
							// PRIMER ENVIO PARA APERTURA DEL ARCHIVO NUEVO
							send_archivo(file_system_fd, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, F_OPEN);
							recv(file_system_fd, &mensaje , sizeof(op_code), 0);
							if(mensaje == F_OPEN_OK) {
								t_archivo_abierto* archivo_nuevo = malloc(sizeof(t_archivo_abierto));
								archivo_nuevo->nombre = strtok(instruccion->parametro1, "\n");
								archivo_nuevo->c_proc_bloqueados = queue_create();
								list_add(tabla_global_archivos, archivo_nuevo);
								log_warning(logger, "El archivo %s fue creado, abierto y cargado a la tabla global de archivos abiertos", instruccion->parametro1);
							} else{
								log_warning(logger, "PID: %d - F_OPEN ERROR: El archivo %s fue creado pero no se pudo abrir", pcb->pid, instruccion->parametro1);
								// no deberia entrar nunca
							}

						}else{
							log_error(logger, "PID: %d - F_OPEN ERROR: No se pudo hacer el F_CREATE del archivo nuevo: %s", pcb->pid, instruccion->parametro1);
							// falta de espacio o se intento crear un archivo ya existente (error)
						}

					}
					archivo_proceso = malloc(sizeof(t_archivoAbierto));
					//archivo_proceso->nombre_archivo = strtok(instruccion->parametro1, "\n");
					strcpy(archivo_proceso->nombre_archivo, strtok(instruccion->parametro1,"\n"));
					archivo_proceso->puntero = 0;
					list_add(pcb->archivos_abiertos, archivo_proceso);
					//t_archivoAbierto* borrame = list_get(pcb->archivos_abiertos, 0);
					//log_warning(logger, "###Primer elemento de la tabla del proceso=%s", borrame->nombre_archivo);
					log_info(logger, "PID: %d - Se agrego %s a la tabla de archivos abiertos", pcb->pid, archivo_proceso->nombre_archivo);
					//VER...
					pthread_mutex_lock(&mx_cola_ready);
					send_proceso(cpu_fd, pcb, DISPATCH);
					pthread_mutex_unlock(&mx_cola_ready);

				}

				sem_post(&s_cpu_desocupado);
				sem_post(&s_esperar_cpu);
				break;

			case F_CLOSE:
				// Saca el archivo de tabla global de archivos abiertos y de la tabla de archivos abiertos del proceso
				log_info(logger, "PID: %d - Recibo pedido de F_CLOSE por: %s", pcb->pid, instruccion->parametro1);

				// Tabla global de archivos abiertos
				if(!list_is_empty(tabla_global_archivos)) {	// Reviso que este el archivo en la TGA
					archivoABuscar = strtok(instruccion->parametro1, "\n");	// global
					bool (*aux1)(void* x) = criterio_nombre_archivo;	// se puede meter como parametro directamente de alguna forma y no tener que usar aux1
					archivo = list_find(tabla_global_archivos, aux1);
				} else {	// No esta en la TGA
					log_error(logger, "PID: %d - Pedido de F_CLOSE por archivo no abierto"); // No esta abierto. Tambien puede que no exista
					//TODO: EXIT-------------------------------##########################################
					break;
				}

				send_archivo(file_system_fd, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, F_CLOSE);
				// TODO: hace falta que responda un f_close_ok/fail y evaluar?

				// Tabla de archivos del proceso
				if(!list_is_empty(pcb->archivos_abiertos)) {	//
					archivoABuscar = strtok(instruccion->parametro1, "\n");	// global
					bool (*aux2)(void* x) = criterio_nombre_archivo_proceso;	// se puede meter como parametro directamente de alguna forma y no tener que usar aux2
					t_archivoAbierto* archivoAux = NULL;
					archivoAux = list_find(pcb->archivos_abiertos, aux2);
					if(archivoAux == NULL){
						log_error(logger, "PID: %d - Pedido de F_CLOSE por archivo no abierto por el proceso: %s", pcb->pid, instruccion->parametro1);
						//TODO: EXIT-------------------------------###############################################
						break;
					}
					// Si lo encuentro, saco el archivo de la tabla de archivos del proceso
					list_remove_element(pcb->archivos_abiertos, archivoAux);
					log_info(logger, "PID: %d - Se elimino %s de la tabla de archivos abiertos", pcb->pid, archivoAux->nombre_archivo);
				} else {
					log_error(logger, "PID: %d - Pedido de F_CLOSE sin archivos abiertos (tabla vacia)", pcb->pid);
					//TODO: EXIT-------------------------------###############################################con semaforos tienen que ser (cpu desocupaado y esperar cpu)
					break;
				}

				// Si no hay procesos bloqueados esperando -> lo saco de la TGA con un fclose
				if(queue_is_empty(archivo->c_proc_bloqueados)) {
					list_remove_element(tabla_global_archivos, archivo);
					log_warning(logger, "El archivo %s fue eliminado de la tabla global de archivos abiertos", archivo->nombre);
				} else {	// Si hay procesos bloqueados mando el primer bloqueado a ready
					PCB_t* procesoBloqueado = queue_pop(archivo->c_proc_bloqueados);
					procesoBloqueado->tiempo_llegada_a_ready = tiempo;	// hrrn
					pthread_mutex_lock(&mx_cola_ready);
					queue_push(cola_ready, procesoBloqueado);
					pthread_mutex_unlock(&mx_cola_ready);
					log_info(logger, "PID: %d - Estado anterior: Blocked por archivo - Estado actual: READY", procesoBloqueado->pid);//TODO: listar cola ready?##########
					sem_post(&s_cont_ready);
					sem_post(&s_ready_execute);
				}

				// Si no hubo ningun exit por error mando el proceso nuevamente al cpu
				pthread_mutex_lock(&mx_cola_ready);
				send_proceso(cpu_fd, pcb,DISPATCH);
				pthread_mutex_unlock(&mx_cola_ready);

  			    sem_post(&s_cpu_desocupado);
				sem_post(&s_esperar_cpu);

				break;
			case F_SEEK:
				log_info(logger, "PID: %d - Recibo pedido de F_SEEK por: %s", pcb->pid, instruccion->parametro1);

				send_archivo(file_system_fd, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, F_SEEK);

				pthread_mutex_lock(&mx_cola_ready);
				send_proceso(cpu_fd, pcb,DISPATCH);
				pthread_mutex_unlock(&mx_cola_ready);

				sem_post(&s_cpu_desocupado);
				sem_post(&s_esperar_cpu);
				break;
			case F_TRUNCATE:
				log_info(logger, "PID: %d - Recibo pedido de F_TRUNCATE por: %s", pcb->pid, instruccion->parametro1);

				send_archivo(file_system_fd, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, F_TRUNCATE);

				pthread_mutex_lock(&mx_cola_ready);
				send_proceso(cpu_fd, pcb,DISPATCH);
				pthread_mutex_unlock(&mx_cola_ready);

				sem_post(&s_cpu_desocupado);
				sem_post(&s_esperar_cpu);
				break;
			case F_READ:
				log_info(logger, "PID: %d - Recibo pedido de F_READ por: %s", pcb->pid, instruccion->parametro1);

				send_archivo(file_system_fd, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, F_READ);

				pthread_mutex_lock(&mx_cola_ready);
				send_proceso(cpu_fd, pcb,DISPATCH);
				pthread_mutex_unlock(&mx_cola_ready);

				sem_post(&s_cpu_desocupado);
				sem_post(&s_esperar_cpu);
				break;

			case F_WRITE:
				log_info(logger, "PID: %d - Recibo pedido de F_WRITE por: %s", pcb->pid, instruccion->parametro1);

				send_archivo(file_system_fd, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, F_WRITE);

				pthread_mutex_lock(&mx_cola_ready);
				send_proceso(cpu_fd, pcb,DISPATCH);
				pthread_mutex_unlock(&mx_cola_ready);

				sem_post(&s_cpu_desocupado);
				sem_post(&s_esperar_cpu);
				break;

			case F_CREATE:
				log_info(logger, "PID: %d - Recibo pedido de F_CREATE por: %s", pcb->pid, instruccion->parametro1);

				send_archivo(file_system_fd, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, F_CREATE);

				recv(file_system_fd, &mensaje , sizeof(op_code), 0);

				if(mensaje == F_CREATE_FAIL) {
					log_error(logger, "PID: %d - Recibo pedido de F_CREATE por archivo existente: %s", pcb->pid, instruccion->parametro1);
					cop = EXIT;
					send(pcb->cliente_fd,&cop,sizeof(op_code),0);
					motivoExit = "F_CREATE por archivo existente"; // ?
					execute_a_exit(pcb, motivoExit);
				}
				sem_post(&s_cpu_desocupado);
				sem_post(&s_esperar_cpu);

				break;
			default:
				log_error(logger, "AAAlgo anduvo mal en el server del kernel\n Cop: %d",cop);
		}
		//hace que no ande ¿?¿?
		//free(instruccion);
	}
}

void execute_a_exit(PCB_t* pcb, char* motivoExit){
	//pthread_mutex_lock(&mx_log);
    log_info(logger,"PID: %d - Estado Anterior: EXECUTE - Estado Actual: EXIT", pcb->pid);
    log_info(logger,"Finaliza el proceso %d - Motivo: %s", pcb->pid, motivoExit);
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
	log_info(logger, "Instruccion numer %d",(pcb->pc-1));
	//pthread_mutex_unlock(&mx_log);
	if(!strcmp(inst->comando,"I/O")){ // HAY QUE VER COMO METERSE EN ESE CHAR ** DISPOSITIVOS IO PARA QUE HAGA EL STRCMP
		//pthread_mutex_lock(&mx_log);
		//log_info(logger, " me meti al if");
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
		uint32_t tiempo = atoi(inst->parametro1);
		//pthread_mutex_lock(&mx_log);
		log_info(logger, " PID: %d - Bloqueado por: %s durante: %s", pcb->pid, inst->comando, inst->parametro1);
		//pthread_mutex_unlock(&mx_log);
		sleep(tiempo);
		//pthread_mutex_lock(&mx_log);
		log_info(logger, "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY", pcb->pid);
		//pthread_mutex_unlock(&mx_log);

		pcb->tiempo_llegada_a_ready = temporal_gettime(reloj_inicio);

		pthread_mutex_lock(&mx_cola_ready);
		queue_push(cola_ready, pcb);
		pthread_mutex_unlock(&mx_cola_ready);

		// TODO para listar los pids despues de entrar a ready
		char* pids = procesosEnReady(cola_ready);
		log_info(logger, "Ingreso a Ready algoritmo %s - PIDS: [%s] ", configuracion->ALGORITMO_PLANIFICACION, pids);
		sem_post(&s_ready_execute);
		sem_post(&s_cont_ready);
		sem_post(&s_io);
		//log_info(logger, "PID: %d - probando como queda", pcb->pid);
}

char* procesosEnReady(t_queue* cola_ready){
	//log_info(logger, "estoy ordenando amigo");
	t_list* listaReady = malloc(sizeof(t_list));
	listaReady = list_create();
	int queuesize = queue_size(cola_ready);
	int i = 0;
	PCB_t * proceso;
	int pids[queuesize];
	char* string_pids;

	while(i < queuesize){

		pthread_mutex_lock(&mx_cola_ready);
		proceso = queue_pop(cola_ready);
		pthread_mutex_unlock(&mx_cola_ready);
		//log_warning(logger, "lolo: %d", queue_size(cola_ready));
		pids[i] = proceso->pid;
		list_add(listaReady, proceso);
		//log_info(logger, "agrego");
		//log_warning(logger, "PIDSSS: %s", string_itoa(pids[i]) );
		if(i==0){
			strcpy(string_pids, string_itoa(pids[i]));
		}else{
			strcat(string_pids, " ");
			strcat(string_pids, string_itoa(pids[i]));

		}

		i++;
	}
	t_link_element* aux_proc1 = listaReady->head;
	i = 0;
	while( aux_proc1 != NULL)
	{
	   PCB_t* aux_proc = aux_proc1->data;
	   pthread_mutex_lock(&mx_cola_ready);
	   queue_push(cola_ready, aux_proc);
	   pthread_mutex_unlock(&mx_cola_ready);
	   aux_proc1 = aux_proc1->next;
	   i++;
	}
//log_error(logger, "PIDSSS: %s", string_pids );
	return string_pids;

}

void esperarRespuestaFS(){
	op_code mensaje;
	recv(cliente_socket, &mensaje, sizeof(op_code), 0);

	if(mensaje == F_OPEN_FAIL){
		//send_archivo(file_system_fd, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, F_CREATE);
		recv(cliente_socket, &mensaje , sizeof(op_code), 0);
		if(mensaje == OK){
			//Por ahora nada
		}
	}


}

bool criterio_nombre_archivo(t_archivo_abierto* archivo) {	// solo para TGA
	if(strcmp(archivo->nombre, archivoABuscar)){
		return false;
	}
	return true;
}
bool criterio_nombre_archivo_proceso(t_archivoAbierto* archivo) {	// solo para lista de archivos del proceso
	if(strcmp(archivo->nombre_archivo, archivoABuscar)){
		return false;
	}
	return true;
}


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
pthread_mutex_t mx_cola_blocked_fs 	= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cola_blocked_io 	= PTHREAD_MUTEX_INITIALIZER;

sem_t s_pasaje_a_ready, s_ready_execute,s_cpu_desocupado,s_cont_ready,s_multiprogramacion_actual,s_esperar_cpu,s_pcb_desalojado,s_blocked,s_io, s_blocked_fs, s_fs_compacta;
sem_t s_blocked_rec;
t_queue* cola_new;
t_queue* cola_ready;
t_queue* cola_ready_sec;
t_list* list_blocked;
t_queue* cola_blocked_respuesta_fs;	// no se usa
t_queue* cola_blocked_fs_libre;
t_queue* cola_blocked_io;
t_tiempos_rafaga_anterior* rafAnteriorAux;
t_list* list_rafa_anterior;
int64_t reloj = 1;

bool cpu_desocupado=true;
bool fs_desocupado = true;
bool haciendo_io = false;

t_list* tabla_global_archivos;
char* archivoABuscar;
t_archivo_abierto* archivo;
t_archivoAbierto* archivo_proceso;
char* recursoABuscar;
double estimadorNuevo;

op_code cop;

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
		sem_wait(&s_cpu_desocupado); // Para que no ejecute cada vez que un proceso llega a ready TODO: el hrrn si se ejecuta cada vez que llega a ready
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
	cola_blocked_respuesta_fs = queue_create();	// no se usa --> borrar
	cola_blocked_fs_libre     = queue_create();	// procesos que esperan fs libre
	cola_blocked_io = queue_create();

	tabla_global_archivos = list_create();

	sem_init(&s_ready_execute,0,1);
	sem_init(&s_cpu_desocupado, 0, 1);
	sem_init(&s_esperar_cpu, 0, 0);
	sem_init(&s_cont_ready,0,0);
	sem_init(&s_io, 0, 1);
	sem_init(&s_blocked_fs, 0, 1);
	sem_init(&s_fs_compacta, 0, 1);



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
		sem_wait(&s_esperar_cpu); //--> se bloquea     Hago el signal si mando algo al cpu

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
				 log_info(logger, "PID: %d - Recibo pedido de WAIT por RECURSO: %s", pcb->pid, strtok(instruccion->parametro1, "\n"));


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

						 t_tiempos_rafaga_anterior * tiemposRafaga = list_get(list_rafa_anterior, pcb->pid);
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

							 //sem_post(&s_ready_execute);
							 //sem_post(&s_cpu_desocupado);
							 sem_post(&s_esperar_cpu);

						 }else{
							 pthread_mutex_lock(&mx_instancias);
							 aux_rec2->instancias -= 1;  //TODO PREGUNTAR!!!
							 pthread_mutex_unlock(&mx_instancias);
							 //------


							 tiemposRafaga->tiempo_out_exec = temporal_gettime(reloj_inicio);
							 estimadorNuevo = obtenerEstimadoRafaga(pcb, configuracion->ESTIMACION_INICIAL, configuracion->HRRN_ALFA);
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
						char* recurso = malloc(sizeof(char) * 20);	// Todo: ojo aca
						strcpy(recurso, aux_rec2->recurso);

						list_add(tiemposRafaga->recursosAsignados, recurso);
						break;	// del while
					 }

					 aux_rec1 = aux_rec1->next;
					 pos_recurso++;
				 }
				 if(recurso_existe) {	// Si es null no se encontro el recurso
					 break;
				 } else {
					 log_error(logger, "PID: %d - Recibo pedido de WAIT por recurso desconocido: %s", pcb->pid, instruccion->parametro1);
					 motivoExit = "WAIT por Recurso Inexistente";
					 execute_a_exit(pcb,motivoExit);
					 sem_post(&s_ready_execute);
					 break;
				 }
			case CREATE_SEGMENT:
				log_info(logger, "PID: %d - Crear segmento - ID:%s - Tamanio: :%s  ", pcb->pid, strtok(instruccion->parametro1, "\n"),strtok(instruccion->parametro2, "\n"));
				uint32_t id_seg  = atoi(instruccion->parametro1);
				uint32_t tam_segmento = atoi(instruccion->parametro2);
				op_code op = CREATE_SEGMENT;

				log_warning(logger, "Id_seg: %d - Tam seg: %d ", id_seg, tam_segmento);
				//send(memoria_fd,&op,sizeof(op_code),0);
			    //send(memoria_fd,&op,sizeof(op_code),0);

				/*t_link_element* aux_seg = pcb->tabla_de_segmentos->head;
				t_segmento* aux_seg2 = aux_seg->data;
				log_error(logger, "PCB antes de enviar a memo: id_eg: %d - tam_seg:%d", aux_seg2->id_segmento, aux_seg2->tamanio_segmento );*/

				//send_pedido_memoria(memoria_fd, id_seg, tam_segmento, pcb->pid,0, op);
				pthread_mutex_lock(&mx_memoria);
				send(memoria_fd,&op,sizeof(op_code),0);
				send(memoria_fd,&(pcb->pid),sizeof(uint16_t),0);
				send(memoria_fd,&(id_seg),sizeof(uint32_t),0);
				send(memoria_fd,&(tam_segmento),sizeof(uint32_t),0);
				pthread_mutex_unlock(&mx_memoria);
				t_segmento* segmento = malloc(sizeof(t_segmento));

				uint32_t id_segmento      = 0;
				uint64_t direccion_base   = 0;
				uint32_t tamanio_segmento = 0;
				op_code cop_memo;

				recv(memoria_fd, &cop_memo, sizeof(op_code), 0);

				switch(cop_memo){
				case CREATE_SEGMENT_OK:
										// Recibir el segmento 0 desde memoria y ponerlo en PCB
										recv(memoria_fd, &id_segmento, sizeof(uint32_t), 0);
										recv(memoria_fd, &direccion_base, sizeof(uint64_t), 0);
										recv(memoria_fd, &tamanio_segmento, sizeof(uint32_t), 0);

										segmento->id_segmento      = id_segmento;
										segmento->direccion_base   = direccion_base;
										segmento->tamanio_segmento = tamanio_segmento;

										//agrego el PCB creado el segmento recibido desde memoria
										// TODO preguntar
										list_add(pcb->tabla_de_segmentos, segmento);

										log_warning(logger,"Recibiendo Segmento: Id: %d - Dir Base: %d - Tamanio: %d", id_segmento, direccion_base, tamanio_segmento);
										pthread_mutex_lock(&mx_cola_ready);  // TODO hacer mas pruebas
										send_proceso(cpu_fd, pcb,DISPATCH);
										pthread_mutex_unlock(&mx_cola_ready);

										break;
				case CREATE_SEGMENT_FAIL:
										log_error(logger, "NO HAY SUFICIENTE");
										execute_a_exit(pcb, "Out of Memory");
										sem_post(&s_ready_execute);
										//TODO recorrer tabla de segmentos y enviar delete de cada uno. meterolo dentro del execute_a_exit
										break;
				case CREATE_SEGMENT_COMPACTO:
										sem_wait(&s_blocked_fs);
										log_warning(logger, "SOLICITAR COMPACTACION");
										op_code cop_make = MAKE_COMPACTATION;
										//sem_(&s_esperar_cpu);
										pthread_mutex_lock(&mx_memoria);
										send(memoria_fd,&cop_make,sizeof(op_code),0);
										pthread_mutex_unlock(&mx_memoria);

										log_warning(logger, "ESPERO FIN DE COMPACTACION");

										recv(memoria_fd, &cop_memo, sizeof(op_code), 0);
										uint32_t pid;
										uint32_t seg_id;
										uint64_t nueva_base;
										while(cop_memo == ACTUALIZAR_SEGMENTO){
										recv(memoria_fd,&pid,sizeof(uint32_t),0);
										recv(memoria_fd,&nueva_base,sizeof(uint64_t),0);
										recv(memoria_fd,&seg_id,sizeof(uint32_t),0);
										recv(memoria_fd, &cop_memo, sizeof(op_code), 0);
										log_warning(logger, "Actualizar segmento %d del proceso %d con la base: %d",seg_id,pid,nueva_base);

										}
										pcb->pc-=1;
										log_warning(logger, "termino compactacion");

										// TODO Actualizar tablas de segmento de todos los procesos
										// y enviar nuevamente la solicitud de CREATE_SEGMENTE para el proceso
										// y hacer lo mismo que arriba de este case y enviar contexto a cpu
										pthread_mutex_lock(&mx_cola_ready);  // TODO hacer mas pruebas
										send_proceso(cpu_fd, pcb,DISPATCH);
										pthread_mutex_unlock(&mx_cola_ready);
										sem_post(&s_blocked_fs);
										break;

				}


				sem_post(&s_esperar_cpu);

				//sem_post(&s_cont_ready);
				 //sem_post(&s_ready_execute);
				 //sem_post(&s_cpu_desocupado);
				break;
			case DELETE_SEGMENT:
							log_info(logger, "PID: %d - Eliminar segmento - Id: %s ", pcb->pid, strtok(instruccion->parametro1, "\n"));
							uint32_t id_seg_del  = atoi(instruccion->parametro1);
							op_code op_del = DELETE_SEGMENT;

							log_warning(logger, "Id_seg a eliminar: %d  ", id_seg_del);

							pthread_mutex_lock(&mx_memoria);
							send(memoria_fd,&op_del,sizeof(op_code),0);
							send(memoria_fd,&(pcb->pid),sizeof(uint16_t),0);
							send(memoria_fd,&(id_seg_del),sizeof(uint32_t),0);
							pthread_mutex_unlock(&mx_memoria);

							op_code cop_memo_delete;

							recv(memoria_fd, &cop_memo_delete, sizeof(op_code), 0);

							if(cop_memo_delete == DELETE_SEGMENT_OK){
								log_info(logger, "PID: %d - Eliminar Segmento - Id Segmento: %d", pcb->pid, id_seg);
							}

							pthread_mutex_lock(&mx_cola_ready);  // TODO hacer mas pruebas
							send_proceso(cpu_fd, pcb,DISPATCH);
							pthread_mutex_unlock(&mx_cola_ready);
							sem_post(&s_esperar_cpu);
							break;

			case SIGNAL:
				log_info(logger, "PID: %d - Recibo pedido de SIGNAL por RECURSO: %s", pcb->pid, strtok(instruccion->parametro1, "\n"));

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
							 //log_info(logger, "entro al if instancias");

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
							 //sem_post(&s_ready_execute);
							 //sem_post(&s_cpu_desocupado);
							 sem_post(&s_esperar_cpu);

						 } else{
							 pthread_mutex_lock(&mx_instancias);
							 aux_rec2_s->instancias += 1;
							 pthread_mutex_unlock(&mx_instancias);
							 log_info(logger,"PID: %d - SIGNAL: %s - Instancias: %d ", pcb->pid, strtok(instruccion->parametro1, "\n"), aux_rec2_s->instancias  );

							 pthread_mutex_lock(&mx_cola_ready);
							 send_proceso(cpu_fd, pcb,DISPATCH);
							 pthread_mutex_unlock(&mx_cola_ready);

							 //sem_post(&s_ready_execute);
							 //sem_post(&s_cpu_desocupado);
							 sem_post(&s_esperar_cpu);

						   }
						 // esto solo para eliminarlos si quedan tras un exit
						 t_tiempos_rafaga_anterior * tiemposRafaga = list_get(list_rafa_anterior, pcb->pid);
						 bool (*aux)(void* x) = criterio_nombre_recurso;
						 char* auxrecurso = malloc(sizeof(char) * 20);	//xd
						 strcpy(auxrecurso, aux_rec2_s->recurso);
						 recursoABuscar = auxrecurso;
						 char* remover = list_find(tiemposRafaga->recursosAsignados, aux);
						 list_remove_element(tiemposRafaga->recursosAsignados, remover);
						 break;
					 }
					 aux_rec1_s = aux_rec1_s->next;
					 pos_recurso++;
				}

				if(recurso_existe) {	// Si no es null se encontro el recurso
					break;
				} else {
					log_error(logger, "PID: %d - Recibo pedido de SIGNAL por recurso desconocido: %s", pcb->pid, instruccion->parametro1);
					motivoExit = "SIGNAL por Recurso Inexistente";
					execute_a_exit(pcb,motivoExit);
					sem_post(&s_ready_execute);
					break;
				}

			case EXIT:	// Cada exit debe incluir estas tres lineas. El motivo seria mejor pasarlo por parametro.
				motivoExit = "POR FIN";
				execute_a_exit(pcb,motivoExit);
				sem_post(&s_ready_execute);
				break;
			case SEGMENTATION_FAULT:	// Cada exit debe incluir estas tres lineas. El motivo seria mejor pasarlo por parametro.
							motivoExit = "SEGMENTATION_FAULT";
							execute_a_exit(pcb,motivoExit);
							sem_post(&s_ready_execute);
							break;

			case YIELD:
				 log_info(logger, "PID: %d - Recibi YIELD de CPU lo mandamos al final de la cola READY", pcb->pid);
				// log_info("Valor del PC: %d", pcb->pc);
				 int tiempo = temporal_gettime(reloj_inicio);
				 t_tiempos_rafaga_anterior* lista = list_get(list_rafa_anterior, pcb->pid);
				 lista->tiempo_out_exec = tiempo;
				 estimadorNuevo = obtenerEstimadoRafaga(pcb, configuracion->ESTIMACION_INICIAL, configuracion->HRRN_ALFA);
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

				 // Lista los pids despues de entrar a ready
				char* pids = procesosEnReady(cola_ready);
				log_info(logger, "Ingreso a Ready algoritmo %s - PIDS: [%s] ", configuracion->ALGORITMO_PLANIFICACION, pids);


				 sem_post(&s_cont_ready);
			//	 sem_post(&s_pcb_desalojado);
				 sem_post(&s_ready_execute);
				 sem_post(&s_cpu_desocupado);
				 break;

			case INTERRUPT: // para hrrn

					break;	// es sin desalojo, no va este case

			case IO:
				/*pthread_t hilo_bloqueado;
				sem_post(&s_blocked);
				pthread_create(&hilo_bloqueado,NULL,(void*)bloqueando,pcb);
				pthread_detach(hilo_bloqueado);								// Hace y me aseguro el hilo no se va a joinear con el hilo principal
				//------*/
				// t_link_element* aux_list_raf_ant = list_rafa_anterior->head;
				rafAnteriorAux = list_get(list_rafa_anterior, pcb->pid);
				rafAnteriorAux->tiempo_out_exec = temporal_gettime(reloj_inicio);
				estimadorNuevo = obtenerEstimadoRafaga(pcb, configuracion->ESTIMACION_INICIAL, configuracion->HRRN_ALFA);
				pcb->estimado_proxima_rafaga = estimadorNuevo;

				/*while( aux_list_raf_ant!=NULL )
				{
				   t_tiempos_rafaga_anterior* aux_list_raf_ant2 = aux_list_raf_ant->data;

				   if( aux_list_raf_ant2->pid ==  pcb->pid)
				   {
					 aux_list_raf_ant2->tiempo_out_exec =  temporal_gettime(reloj_inicio);
					 estimadorNuevo = obtenerEstimadoRafaga(pcb, configuracion->ESTIMACION_INICIAL, configuracion->HRRN_ALFA);
					 pcb->estimado_proxima_rafaga = estimadorNuevo;
					 break;
				   }
				   aux_list_raf_ant = aux_list_raf_ant->next;
				}*/
				if(haciendo_io) {
					pthread_mutex_lock(&mx_cola_blocked_io);
					queue_push(cola_blocked_io, pcb);
					pthread_mutex_unlock(&mx_cola_blocked_io);
					log_info(logger, "PID: %d - Estado Anterior: EXECUTE - Estado Actual: BLOCKED por I/O ocupado", pcb->pid);
					sem_post(&s_ready_execute);
					sem_post(&s_cpu_desocupado);
					break;
				} haciendo_io = true;	// se podria aplicar un mutex

				//---
				//pthread_mutex_lock(&mx_log);
				log_info(logger, "PID: %d - Estado Anterior: EXECUTE - Estado Actual: BLOCKED por I/O", pcb->pid);
				pthread_t hilo_bloqueado;
				//sem_post(&s_blocked);	// TODO: revisar este sem
				pthread_create(&hilo_bloqueado,NULL, (void*) ejecutar_io, pcb);
				pthread_detach(hilo_bloqueado);
				sem_post(&s_ready_execute);
				sem_post(&s_cpu_desocupado);

				break;

			case F_OPEN:
				log_info(logger, "PID: %d - Recibo pedido de F_OPEN por: %s", pcb->pid, instruccion->parametro1);
				// LOG obligatorio
				log_info(logger, "PID: %d - Abrir Archivo: %s", pcb->pid, instruccion->parametro1);

				sem_wait(&s_blocked_fs);
				fs_desocupado = false;	//cambiar esto
				/*while(!fs_desocupado) {	// Espera activa
					// do nothing
				}fs_desocupado = false;*/
				/* parece que el f_open no se bloquea, sino que se queda esperando
				if(!fs_desocupado) {
					pthread_mutex_lock(&mx_cola_blocked_fs);
					queue_push(cola_blocked_fs_libre, pcb);
					pthread_mutex_lock(&mx_cola_blocked_fs);
					log_info(logger, "PID: %d - Estado anterior: Execute - Estado Actual: BLOCKED por FS ocupado", pcb->pid);
					sem_post(&s_cpu_desocupado);
					sem_post(&s_ready_execute);
					break;
				}
				*/

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
				{  log_warning(logger, "El archivo %s ya estaba abierto en la TGA",instruccion->parametro1);
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
					sem_post(&s_ready_execute);
					sem_post(&s_cpu_desocupado);

				}else { // El archivo no esta abierto
					log_warning(logger, "Archivo %s no esta abierto",instruccion->parametro1);
					send_archivo(file_system_fd,instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, "", F_EXISTS);
					recv(file_system_fd, &mensaje , sizeof(op_code), 0);

					log_warning(logger, "Existencia=%d", mensaje);

					// Reviso si el archivo existe en el fs
					if(mensaje)
					{
						// El archivo existe -> lo abro
						send_archivo(file_system_fd,instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, "", F_OPEN);
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
						send_archivo(file_system_fd,instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, "", F_CREATE);
						recv(file_system_fd, &mensaje , sizeof(op_code), 0);

						if(mensaje == F_CREATE_OK)
						{
							// PRIMER ENVIO PARA APERTURA DEL ARCHIVO NUEVO
							send_archivo(file_system_fd, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, "", F_OPEN);
							recv(file_system_fd, &mensaje , sizeof(op_code), 0);
							if(mensaje == F_OPEN_OK) {
								t_archivo_abierto* archivo_nuevo = malloc(sizeof(t_archivo_abierto));
								archivo_nuevo->nombre = strtok(instruccion->parametro1, "\n");
								archivo_nuevo->c_proc_bloqueados = queue_create();
								list_add(tabla_global_archivos, archivo_nuevo);
								log_warning(logger, "El archivo %s fue creado, abierto y cargado a la tabla global de archivos abiertos", instruccion->parametro1);
							} else{
								log_error(logger, "PID: %d - F_OPEN ERROR: El archivo %s fue creado pero no se pudo abrir", pcb->pid, instruccion->parametro1);
								// no deberia entrar nunca
							}

						} else{	// No deberia entrar nunca
							log_error(logger, "PID: %d - F_OPEN ERROR: No se pudo hacer el F_CREATE del archivo nuevo: %s", pcb->pid, instruccion->parametro1);
							//motivoExit = "Se intento crear un archivo existente";
							//execute_a_exit(pcb,motivoExit);
							//sem_post(&s_ready_execute);

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
					//sem_post(&s_cpu_desocupado);
					//sem_post(&s_ready_execute);
					sem_post(&s_esperar_cpu);
				}
				//sem_post(&s_cpu_desocupado);
				//sem_post(&s_esperar_cpu);
				fs_desocupado = true;
				sem_post(&s_blocked_fs);
				break;

			case F_CLOSE:	// no interactua con fs
				// Saca el archivo de tabla global de archivos abiertos y de la tabla de archivos abiertos del proceso
				log_info(logger, "PID: %d - Recibo pedido de F_CLOSE por: %s", pcb->pid, instruccion->parametro1);

				// Tabla global de archivos abiertos
				if(!list_is_empty(tabla_global_archivos)) {	// Reviso que este el archivo en la TGA
					archivoABuscar = strtok(instruccion->parametro1, "\n");	// global
					bool (*aux1)(void* x) = criterio_nombre_archivo;	// se puede meter como parametro directamente de alguna forma y no tener que usar aux1
					archivo = list_find(tabla_global_archivos, aux1);
				} else {	// No esta en la TGA
					log_error(logger, "PID: %d - Pedido de F_CLOSE por archivo no abierto", pcb->pid); // No esta abierto. Tambien puede que no exista
					motivoExit = "No esta abierto en tabla global";
					execute_a_exit(pcb,motivoExit);
					sem_post(&s_ready_execute);
					break;
				}

				//send_archivo(file_system_fd, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, F_CLOSE);
				// TODO: ni hay que mandarlo al fs

				// Tabla de archivos del proceso
				if(!list_is_empty(pcb->archivos_abiertos)) {	//
					archivoABuscar = strtok(instruccion->parametro1, "\n");	// global
					bool (*aux2)(void* x) = criterio_nombre_archivo_proceso;	// se puede meter como parametro directamente de alguna forma y no tener que usar aux2
					t_archivoAbierto* archivoAux = NULL;
					archivoAux = list_find(pcb->archivos_abiertos, aux2);
					if(archivoAux == NULL){
						log_error(logger, "PID: %d - Pedido de F_CLOSE por archivo no abierto por el proceso: %s", pcb->pid, instruccion->parametro1);
						motivoExit = "No esta abierto en tabla del proceso";
						execute_a_exit(pcb,motivoExit);
						sem_post(&s_ready_execute);
						break;
					}
					// Si lo encuentro, saco el archivo de la tabla de archivos del proceso
					list_remove_element(pcb->archivos_abiertos, archivoAux);
					log_info(logger, "PID: %d - Se elimino %s de la tabla de archivos abiertos", pcb->pid, archivoAux->nombre_archivo);
				} else {
					log_error(logger, "PID: %d - Pedido de F_CLOSE sin archivos abiertos (tabla vacia)", pcb->pid);
					motivoExit = "Tabla global vacia";
					execute_a_exit(pcb,motivoExit);
					sem_post(&s_ready_execute);
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
					// Lista los pids despues de entrar a ready
				    char* pids = procesosEnReady(cola_ready);
					log_info(logger, "Ingreso a Ready algoritmo %s - PIDS: [%s] ", configuracion->ALGORITMO_PLANIFICACION, pids);
					log_info(logger, "PID: %d - Estado anterior: Blocked por archivo - Estado actual: READY", procesoBloqueado->pid);//TODO: listar cola ready?##########
					sem_post(&s_cont_ready);
					sem_post(&s_ready_execute);
				}

				// Si no hubo ningun exit por error mando el proceso nuevamente al cpu
				pthread_mutex_lock(&mx_cola_ready);
				send_proceso(cpu_fd, pcb,DISPATCH);
				pthread_mutex_unlock(&mx_cola_ready);

  			    //sem_post(&s_cpu_desocupado);
				sem_post(&s_esperar_cpu);

				break;
			case F_SEEK:	// no interactua con fs
				//log_info(logger, "PID: %d - Recibo pedido de F_SEEK por: %s - puntero %s", pcb->pid, instruccion->parametro1, instruccion->parametro2);
				// LOG obligatorio
				log_info(logger, "PID: %d - Actualizar puntero Archivo:: %s - Puntero: %s", pcb->pid, instruccion->parametro1, instruccion->parametro2);

				// Tabla de archivos del proceso: verifico que el archivo este abierto y actualizo el valor del puntero
				if(!list_is_empty(pcb->archivos_abiertos)) {	//
					//log_error(logger, "Entro al if");
					archivoABuscar = strtok(instruccion->parametro1, "\n");	// global
					bool (*aux3)(void* x) = criterio_nombre_archivo_proceso;	// se puede meter como parametro directamente de alguna forma y no tener que usar aux2
					t_archivoAbierto* archivoAux2 = NULL;
					archivoAux2 = list_find(pcb->archivos_abiertos, aux3);
					if(archivoAux2 == NULL){
						log_error(logger, "PID: %d - Pedido de F_SEEK por archivo no abierto por el proceso: %s", pcb->pid, instruccion->parametro1);
						motivoExit = "No esta abierto en tabla del proceso";
						execute_a_exit(pcb,motivoExit);
						sem_post(&s_ready_execute);
						break;
					}
					// Si lo encuentro, actualizo el puntero puntero en la lista de archivos abiertos del proceso
					archivoAux2->puntero = atoi(strtok(instruccion->parametro2,"\n"));
					//log_info(logger, "PID: %d - Se elimino %s de la tabla de archivos abiertos", pcb->pid, archivoAux->nombre_archivo);
				} else {
					log_error(logger, "PID: %d - Pedido de F_CLOSE sin archivos abiertos (tabla vacia)", pcb->pid);
					motivoExit = "Tabla global vacia";
					execute_a_exit(pcb,motivoExit);
					sem_post(&s_ready_execute);
					break;
				}

				pthread_mutex_lock(&mx_cola_ready);
				send_proceso(cpu_fd, pcb,DISPATCH);
				pthread_mutex_unlock(&mx_cola_ready);

				//sem_post(&s_cpu_desocupado);
				sem_post(&s_esperar_cpu);
				break;

			case F_TRUNCATE:
				log_info(logger, "PID: %d - Recibo pedido de F_TRUNCATE por: %s", pcb->pid, instruccion->parametro1);

				// Tiempos hrrn
				rafAnteriorAux = list_get(list_rafa_anterior, pcb->pid);
				rafAnteriorAux->tiempo_out_exec = temporal_gettime(reloj_inicio);
				estimadorNuevo = obtenerEstimadoRafaga(pcb, configuracion->ESTIMACION_INICIAL, configuracion->HRRN_ALFA);
				pcb->estimado_proxima_rafaga = estimadorNuevo;
				//log_warning(logger, "PID: %d estimador nuevo %f ftruncante", pcb->pid, estimadorNuevo);

				/////// Puede traer problemas de sincronizacion utilizar el bool fs_desocupado en lugar de una implementacion con semaforos
				if(!fs_desocupado) {
					pthread_mutex_lock(&mx_cola_blocked_fs);
					queue_push(cola_blocked_fs_libre, pcb);
					pthread_mutex_unlock(&mx_cola_blocked_fs);
					log_info(logger, "PID: %d - Estado anterior: Execute - Estado Actual: BLOCKED por FS ocupado", pcb->pid);
					sem_post(&s_cpu_desocupado);
					sem_post(&s_ready_execute);
					break;
				}
				fs_desocupado = false;
				sem_wait(&s_blocked_fs);	// Solo para que el fopen espere
				//////
				send_archivo(file_system_fd, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, "", F_TRUNCATE);
				log_info(logger, "PID: %d - Estado anterior EXECUTE - Estado actual BLOCKED esperando respuesta de FS", pcb->pid);

				// Hilo de espera a respuesta
				pthread_t hilo_bloqueado_truncate;
				pthread_create(&hilo_bloqueado_truncate,NULL,(void*)bloqueando_por_filesystem,pcb);
				pthread_detach(hilo_bloqueado_truncate);

				sem_post(&s_ready_execute);
				sem_post(&s_cpu_desocupado);

				break;

			case F_READ:
				//sem_wait(&s_fs_compacta);
				/*
				  F_READ (Nombre Archivo, Dirección Lógica, Cantidad de Bytes):
				  Esta instrucción solicita al Kernel que se lea del archivo indicado,
				  la cantidad de bytes pasada por parámetro y se escriba en la dirección física de Memoria la información leída.
				 */
				log_info(logger, "PID: %d - Recibo pedido de F_READ por: %s", pcb->pid, instruccion->parametro1);
				// LOG OBligatorio
				log_info(logger, "PID: < %d >- Leer Archivo: <%s> - Puntero <PUNTERO> - Dirección Memoria <DIRECCIÓN MEMORIA> - Tamaño <TAMAÑO>", pcb->pid, instruccion->parametro1);

				// Tiempos hrrn
				rafAnteriorAux = list_get(list_rafa_anterior, pcb->pid);
				rafAnteriorAux->tiempo_out_exec = temporal_gettime(reloj_inicio);
				estimadorNuevo = obtenerEstimadoRafaga(pcb, configuracion->ESTIMACION_INICIAL, configuracion->HRRN_ALFA);
				pcb->estimado_proxima_rafaga = estimadorNuevo;

				///////
				if(!fs_desocupado) {
					pthread_mutex_lock(&mx_cola_blocked_fs);
					queue_push(cola_blocked_fs_libre, pcb);
					pthread_mutex_unlock(&mx_cola_blocked_fs);
					log_info(logger, "PID: %d - Estado anterior: Execute - Estado Actual: BLOCKED por FS ocupado", pcb->pid);
					sem_post(&s_cpu_desocupado);
					sem_post(&s_ready_execute);
					break;
				}
				fs_desocupado = false;
				sem_wait(&s_blocked_fs);	// Solo para que el fopen espere
				//////

				/*recv(file_system_fd, &mensaje , sizeof(op_code), 0);
				if(mensaje == F_READ_FAIL) {
					motivoExit = "Error al hacer F_READ";
					execute_a_exit(pcb);
					sem_post(&s_ready_execute);
					break;
				}*/

				{	// lo pongo en un bloque para que no joda con redeclarar variables
				archivoABuscar = strtok(instruccion->parametro1, "\n");	// global
				bool (*aux3)(void* x) = criterio_nombre_archivo_proceso;	// se puede meter como parametro directamente de alguna forma y no tener que usar aux2
				t_archivoAbierto* archivoAux2 = NULL;
				archivoAux2 = list_find(pcb->archivos_abiertos, aux3);
				int posicion = archivoAux2->puntero;
				send_archivo(file_system_fd, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, string_itoa(posicion), F_READ);
				log_info(logger, "PID: %d - Estado anterior EXECUTE - Estado actual BLOCKED esperando respuesta de FS", pcb->pid);
				}

				// Hilo de espera a respuesta
				pthread_t hilo_bloqueado_read;
				pthread_create(&hilo_bloqueado_read,NULL,(void*)bloqueando_por_filesystem,pcb);
				pthread_detach(hilo_bloqueado_read);

				sem_post(&s_ready_execute);
				sem_post(&s_cpu_desocupado);

				break;

			case F_WRITE:
				//sem_wait(&s_fs_compacta);
				log_info(logger, "PID: %d - Recibo pedido de F_WRITE por: %s", pcb->pid, instruccion->parametro1);

				// Tiempos hrrn
				rafAnteriorAux = list_get(list_rafa_anterior, pcb->pid);
				rafAnteriorAux->tiempo_out_exec = temporal_gettime(reloj_inicio);
				estimadorNuevo = obtenerEstimadoRafaga(pcb, configuracion->ESTIMACION_INICIAL, configuracion->HRRN_ALFA);
				pcb->estimado_proxima_rafaga = estimadorNuevo;

				/////// Si el fs esta ocupado bloqueo el proceso. Lo desbloquea el que lo esta usando
				if(!fs_desocupado) {
					pthread_mutex_lock(&mx_cola_blocked_fs);
					queue_push(cola_blocked_fs_libre, pcb);
					pthread_mutex_unlock(&mx_cola_blocked_fs);
					log_info(logger, "PID: %d - Estado anterior: Execute - Estado Actual: BLOCKED por FS ocupado", pcb->pid);
					sem_post(&s_cpu_desocupado);
					break;
				}
				fs_desocupado = false;
				sem_wait(&s_blocked_fs);	// Solo para que el fopen espere
				//////

				/*recv(file_system_fd, &mensaje , sizeof(op_code), 0);
				if(mensaje == F_WRITE_FAIL) {
					motivoExit = "Error al hacer F_WRITE";
					execute_a_exit(pcb);
					sem_post(&s_ready_execute);
					break;
				}*/
				{	// lo pongo en un bloque para que no joda con redeclarar variables
				archivoABuscar = strtok(instruccion->parametro1, "\n");	// global
				bool (*aux3)(void* x) = criterio_nombre_archivo_proceso;	// se puede meter como parametro directamente de alguna forma y no tener que usar aux2
				t_archivoAbierto* archivoAux2 = NULL;
				archivoAux2 = list_find(pcb->archivos_abiertos, aux3);
				int posicion = archivoAux2->puntero;
				send_archivo(file_system_fd, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, string_itoa(posicion), F_WRITE);
				log_info(logger, "PID: %d - Estado anterior EXECUTE - Estado actual BLOCKED esperando respuesta de FS", pcb->pid);
				}
				// Hilo de espera a respuesta
				pthread_t hilo_bloqueado_write;
				pthread_create(&hilo_bloqueado_write,NULL,(void*)bloqueando_por_filesystem,pcb);
				pthread_detach(hilo_bloqueado_write);

				sem_post(&s_ready_execute);
				sem_post(&s_cpu_desocupado);
				break;

			/*case F_CREATE:	// Este case no deberia existir -> no es una instruccion que llegue por archivo
				log_info(logger, "PID: %d - Recibo pedido de F_CREATE por: %s", pcb->pid, instruccion->parametro1);

				/////// por las dudas pero no
				if(!fs_desocupado) {
					pthread_mutex_lock(&mx_cola_blocked_fs);
					queue_push(cola_blocked_fs_libre, pcb);
					pthread_mutex_lock(&mx_cola_blocked_fs);
					log_info(logger, "PID: %d - Estado anterior: Execute - Estado Actual: BLOCKED por FS ocupado", pcb->pid);
					sem_post(&s_cpu_desocupado);
					break;
				}
				//////

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
			*/
			default:
				log_error(logger, "AAAlgo anduvo mal en el server del kernel\n Cop: %d",cop);
		}
		//hace que no ande ¿?¿?
		//free(instruccion);
	}
}

void execute_a_exit(PCB_t* pcb, char* motivoExit){
	cop = EXIT;

	liberar_recursos(pcb);
	liberar_archivos(pcb);

	send(pcb->cliente_fd,&cop,sizeof(op_code),0);
	//pthread_mutex_lock(&mx_log);
    log_info(logger,"PID: %d - Estado Anterior: EXECUTE - Estado Actual: EXIT", pcb->pid);
    log_info(logger,"Finaliza el proceso %d - Motivo: %s", pcb->pid, motivoExit);
    //pthread_mutex_unlock(&mx_log);
    sem_post(&s_multiprogramacion_actual);//cuando se finaliza
    //liberar_espacio_de_memoria(PCB); Liberamos las estructructuras de memoria
    pcb_destroy(pcb);
    //avisar_consola_finalizacion(); Funcion que le avisa a la consola que se finalizo correctamente
    sem_post(&s_cpu_desocupado);
    //sem_post(&s_ready_execute);
}

/*void bloqueando(PCB_t* pcb){
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
}*/

void ejecutar_io(PCB_t* pcb) {
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
		log_info(logger, " PID: %d - Bloqueado por I/O durante: %s", pcb->pid, inst->parametro1);
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
		//sem_post(&s_ready_execute);
		sem_post(&s_cont_ready);

		if(!queue_is_empty(cola_blocked_io)) {
			pthread_mutex_lock(&mx_cola_blocked_io);
			PCB_t* pcb_blocked = queue_pop(cola_blocked_io);
			pthread_mutex_unlock(&mx_cola_blocked_io);
			pthread_t hilo_bloqueado_io;
			pthread_create(&hilo_bloqueado_io,NULL, (void*) ejecutar_io, pcb_blocked);
			pthread_detach(hilo_bloqueado_io);
			log_info(logger, "PID: %d - Estado Anterior: BLOCKED por I/O ocupado - Estado Actual: BLOCKED por I/O", pcb_blocked->pid);
		} else {
			haciendo_io = false;
		}
		//sem_post(&s_io);
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

void esperarRespuestaFS(){	// no se de donde salio esto
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

bool criterio_nombre_recurso(char* recurso) {	// solo para lista de recursos en lista de rafagas
	if(strcmp(recurso, recursoABuscar)){
		return false;
	}
	return true;
}
bool criterio_nombre_recurso_lista_recursos(t_recurso* recurso) {	// solo para
	if(strcmp(recurso->recurso, recursoABuscar)){
		return false;
	}
	return true;
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

void bloqueando_por_filesystem(PCB_t* pcb){	// Esperando respuesta del fs
	int i = 0;
	op_code cop;
	////sem_wait(&s_blocked);
	//INSTRUCCION* inst = list_get(pcb->instrucciones, pcb->pc - 1);

	//log_info(logger, "Instruccion numer %d",(pcb->pc-1));
	//log_info(logger, "Bloqueando_por_filesystem %d", pcb->pid);
	//if(!strcmp(inst->comando,"F_TRUNCATE") || !strcmp(inst->comando,"F_READ") || !strcmp(inst->comando,"F_WRITE")){
	esperar_filesystem(pcb);
	//}

}

void execute_fread(PCB_t* pcb) {
	INSTRUCCION* instruccion = list_get(pcb->instrucciones, pcb->pc - 1);
	send_archivo(file_system_fd, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, "", F_READ);
	// Hilo de espera a respuesta
	pthread_t hilo_bloqueado_read;
	pthread_create(&hilo_bloqueado_read,NULL,(void*)bloqueando_por_filesystem,pcb);
	pthread_detach(hilo_bloqueado_read);
}
void execute_fwrite(PCB_t* pcb) {
	INSTRUCCION* instruccion = list_get(pcb->instrucciones, pcb->pc - 1);
	send_archivo(file_system_fd, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, "", F_WRITE);
	// Hilo de espera a respuesta
	pthread_t hilo_bloqueado_write;
	pthread_create(&hilo_bloqueado_write,NULL,(void*)bloqueando_por_filesystem,pcb);
	pthread_detach(hilo_bloqueado_write);
}
void execute_ftruncate(PCB_t* pcb) {
	INSTRUCCION* instruccion = list_get(pcb->instrucciones, pcb->pc - 1);
	send_archivo(file_system_fd, instruccion->parametro1, instruccion->parametro2, instruccion->parametro3, "", F_TRUNCATE);
	// Hilo de espera a respuesta
	pthread_t hilo_bloqueado_truncate;
	pthread_create(&hilo_bloqueado_truncate,NULL,(void*)bloqueando_por_filesystem,pcb);
	pthread_detach(hilo_bloqueado_truncate);
}

void esperar_filesystem(PCB_t* pcb){	// Solo instrucciones con demora

	op_code mensaje;

	bool entro_error = false;
	//log_info(logger, "PID: %d - En hilo esperando respuesta de FS", pcb->pid);

	recv(file_system_fd, &mensaje , sizeof(op_code), 0);

	switch(mensaje) {
		case F_TRUNCATE_OK:
		case F_READ_OK:
		case F_WRITE_OK:
			pcb->tiempo_llegada_a_ready = temporal_gettime(reloj_inicio);
			log_info(logger, "PID: %d - Estado anterior BLOCKED por respuesta FS - Estado actual READY %d", pcb->pid, fs_desocupado); // TODO: listar ready
			pthread_mutex_lock(&mx_cola_ready);
			queue_push(cola_ready, pcb);
			pthread_mutex_unlock(&mx_cola_ready);
			// Lista los pids despues de entrar a ready
			char* pids = procesosEnReady(cola_ready);
			log_info(logger, "Ingreso a Ready algoritmo %s - PIDS: [%s] ", configuracion->ALGORITMO_PLANIFICACION, pids);
			//sem_post(&s_cont_ready);
			//sem_post(&s_fs_compacta);

			break;
		case F_TRUNCATE_FAIL:
		case F_READ_FAIL:
		case F_WRITE_FAIL:
			char motivoExit[] = "Fallo una instruccion al FS";
			execute_a_exit(pcb,motivoExit);	// no hace el signal de ready_execute xq ya lo hizo antes
			entro_error = true;
			break;
		default:	// no deberia entrar nunca
			log_error(logger, "Error en case de hilo de espera a respuesta del FS");
			char motivoExit2[] = "ERROR inesperado con operacion con demora del FS";
			execute_a_exit(pcb,motivoExit2);	// no hace el signal de ready_execute xq ya lo hizo antes
			entro_error = true;
			break;
	}
	/*
	if(mensaje == F_TRUNCATE_OK || mensaje == F_READ_OK || mensaje == F_WRITE_OK){
		pcb->tiempo_llegada_a_ready = temporal_gettime(reloj_inicio);
		pthread_mutex_lock(&mx_cola_ready);
		queue_push(cola_ready, pcb);
		pthread_mutex_unlock(&mx_cola_ready);

		sem_post(&s_cont_ready);
	} else{	// nunca deberia entrar
		char* motivoExit = "ERROR con operacion con demora del FS";
		execute_a_exit(pcb,motivoExit);	// no hace el signal de ready_execute xq ya lo hizo antes
	}*/

	if(queue_is_empty(cola_blocked_fs_libre)) {
		fs_desocupado = true;
		sem_post(&s_blocked_fs);	// Ahora el fopen puede entrar. Ojo sincro: fs_desocupado y s_blocked_fs
	} else {
		pthread_mutex_lock(&mx_cola_blocked_fs);
		PCB_t* pcb_blocked = queue_pop(cola_blocked_fs_libre);
		pthread_mutex_unlock(&mx_cola_blocked_fs);
		log_info(logger, "PID: %d - Estado anterior: BLOCKED por FS ocupado - Estado actual: BLOCKED por respuesta FS", pcb_blocked->pid);
		INSTRUCCION* instruccion = list_get(pcb_blocked->instrucciones, pcb_blocked->pc - 1);

		//
		//pthread_t hilo_bloqueado_espera_fs;
		//if(strcmp(instruccion->comando, "F_OPEN") == 0)
		//{
			//execute_fopen(pcb);
		//}
		// Aca se crea recursivamente el siguiente hilo si hay alguien esperando
		if(strcmp(instruccion->comando, "F_READ") == 0)
		{
			execute_fread(pcb_blocked);
		}
		if(strcmp(instruccion->comando, "F_WRITE") == 0)
		{
			execute_fwrite(pcb_blocked);
		}
		if(strcmp(instruccion->comando, "F_TRUNCATE") == 0)
		{
			execute_ftruncate(pcb_blocked);
		}
	}

	if(!entro_error){
		sem_post(&s_cont_ready);
	}
}


void liberar_archivos(PCB_t* pcb){
	if(!list_is_empty(pcb->archivos_abiertos)){
		int i;
		int tamanio = list_size(pcb->archivos_abiertos);
		t_archivoAbierto* archivoACerrar;
		for(i = 0; i < tamanio; i++) {
			archivoACerrar = list_get(pcb->archivos_abiertos, i);
			archivoABuscar = archivoACerrar->nombre_archivo;
			if(!list_is_empty(tabla_global_archivos)) {	// Reviso que este el archivo en la TGA
				bool (*aux1)(void* x) = criterio_nombre_archivo;	// se puede meter como parametro directamente de alguna forma y no tener que usar aux1
				archivo = list_find(tabla_global_archivos, aux1);
				if(archivo != NULL) {
					log_warning(logger, "PID %d - Fue a exit con un archivo abierto", pcb->pid);
					if(queue_is_empty(archivo->c_proc_bloqueados)) {	// Igual a fclose
						list_remove_element(tabla_global_archivos, archivo);
						log_warning(logger, "El archivo %s fue eliminado de la tabla global de archivos abiertos", archivo->nombre);
					} else {	// Si hay procesos bloqueados mando el primer bloqueado a ready
						PCB_t* procesoBloqueado = queue_pop(archivo->c_proc_bloqueados);
						procesoBloqueado->tiempo_llegada_a_ready = temporal_gettime(reloj_inicio);	// hrrn
						pthread_mutex_lock(&mx_cola_ready);
						queue_push(cola_ready, procesoBloqueado);
						pthread_mutex_unlock(&mx_cola_ready);
						// Lista los pids despues de entrar a ready
						char* pids = procesosEnReady(cola_ready);
						log_info(logger, "Ingreso a Ready algoritmo %s - PIDS: [%s] ", configuracion->ALGORITMO_PLANIFICACION, pids);
						log_info(logger, "PID: %d - Estado anterior: Blocked por archivo - Estado actual: READY", procesoBloqueado->pid);//TODO: listar cola ready?##########
						sem_post(&s_cont_ready);
						//sem_post(&s_ready_execute);
					}
				} else { // no deberia entrar nunca
					log_error(logger, "PID %d - Fue a exit con procesos en su tabla que no estaban en la global", pcb->pid);
				}
			}
		}
	}
}

void liberar_recursos(PCB_t* pcb){
	t_tiempos_rafaga_anterior* elemento = list_get(list_rafa_anterior, pcb->pid); // estan ordenados por pid entonces se puede usar como index
	if(!list_is_empty(elemento->recursosAsignados)) {
		// parecido a signal
		int i;
		int length = list_size(elemento->recursosAsignados);
		PCB_t* pcb_blocked;
		for(i = 0; i < length; i++) {
			recursoABuscar = list_get(elemento->recursosAsignados, i);
			bool (*aux)(void* x) = criterio_nombre_recurso_lista_recursos;
			t_recurso* aux_rec2_s = list_find(lista_de_recursos, aux);

			if(aux_rec2_s->instancias < 0)
			 {
				 //log_info(logger, "entro al if instancias");

				 pthread_mutex_lock(&mx_instancias);
				 aux_rec2_s->instancias += 1;
				 pthread_mutex_unlock(&mx_instancias);

				 log_warning(logger,"PID: %d - SIGNAL por exit sin signal: %s - Instancias: %d ", pcb->pid, recursoABuscar, aux_rec2_s->instancias);

				 if(!queue_is_empty(aux_rec2_s->cola_bloqueados_recurso))
				 {
					 pthread_mutex_lock(&mx_cola_blocked);
					 pcb_blocked = queue_pop(aux_rec2_s->cola_bloqueados_recurso);
					 pthread_mutex_unlock(&mx_cola_blocked);
					 log_info(logger, "PID: %d que se  desbloqueo", pcb_blocked->pid );
				 }else {
					 log_error(logger, "Ocurrio un error con las instancias del recurso %d: ", pcb->pid);
				 }
				 // listado de pids despues de entrar a ready
				 char* pids = procesosEnReady(cola_ready);
				 log_info(logger, "Ingreso a Ready algoritmo %s - PIDS: [%s] ", configuracion->ALGORITMO_PLANIFICACION, pids);

				 log_info(logger, "PID: %d - Estado Anterior: BLOCKED - Estado Actual: READY - PROGRAM COINTER: %d", pcb_blocked->pid, pcb_blocked->pc );

				 pcb_blocked->tiempo_llegada_a_ready = temporal_gettime(reloj_inicio);

				 pthread_mutex_lock(&mx_cola_ready);
				 queue_push(cola_ready,pcb_blocked);
				 pthread_mutex_unlock(&mx_cola_ready);

				 sem_post(&s_cont_ready);
				 //sem_post(&s_ready_execute);
				 //sem_post(&s_cpu_desocupado);
				 //sem_post(&s_esperar_cpu);

			 } else{
				 pthread_mutex_lock(&mx_instancias);
				 aux_rec2_s->instancias += 1;
				 pthread_mutex_unlock(&mx_instancias);
				 log_warning(logger,"PID: %d - SIGNAL por exit sin signal: %s - Instancias: %d ", pcb->pid, recursoABuscar, aux_rec2_s->instancias);

				 /*
				 pthread_mutex_lock(&mx_cola_ready);
				 send_proceso(cpu_fd, pcb,DISPATCH);
				 pthread_mutex_unlock(&mx_cola_ready);

				 sem_post(&s_ready_execute);
				 sem_post(&s_cpu_desocupado);
				 sem_post(&s_esperar_cpu);*/

			   }

			}
		}
}






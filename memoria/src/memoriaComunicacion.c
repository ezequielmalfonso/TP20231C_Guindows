/*
 * comunicacion.c
 *
 *  Created on: 12 abr 2023
 *      Author: utnso
 */
#include "memoriaComunicacion.h"

typedef struct {
  int fd;
  char * server_name;
}
t_procesar_conexion_args;
int cpu_fd;
pthread_mutex_t mx_kernel= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cpu = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_pagefault = PTHREAD_MUTEX_INITIALIZER;
int* tamanio;
uint32_t* num_seg;
uint32_t* desplazamiento1;
uint32_t* pid;



//KERNEL
static void procesar_kernel(void * void_args) {
  t_procesar_conexion_args * args = (t_procesar_conexion_args * ) void_args;
  int cliente_socket = args -> fd;
  char * server_name = args -> server_name;
  free(args);
  uint16_t pid;
  uint16_t id_segmento;

  op_code cop;
  while (cliente_socket != -1) {

    if (recv(cliente_socket, & cop, sizeof(op_code), 0) != sizeof(op_code)) {
      log_info(logger, "Se ha finalizado la conexion");
      return;
    }

    switch (cop) {
    case DEBUG:
    			log_info(logger, "debug");
    			break;
    case KERNEL: log_info(logger, "RESPUESTA AL CONECTAR KERNEL");
    			 break;
    case CREAR_TABLA:
      pid = 0;
      id_segmento = 0;
      int cantidad_ids = 0;
      op_code op;

      pthread_mutex_lock(&mx_kernel);
      recv(cliente_socket, & pid, sizeof(uint16_t), 0);
      pthread_mutex_unlock(&mx_kernel);

      t_list* tabla_de_segmentos = cargarProceso(pid);

      log_info(logger, "[KERNEL] Recibiendo solicitud de segmento para programa %d", pid);

      // Despuesta cambiar a tabla de segmentos
      //envio_segmento_0(segmento);
      log_info(logger, "[KERNEL] Envio de segmento 0 para programa %d", pid);
      pthread_mutex_lock(&mx_kernel);
      send(cliente_socket, &(segmento->id_segmento), sizeof(uint32_t), MSG_WAITALL);
      send(cliente_socket, &(segmento->direccion_base), sizeof(uint64_t), MSG_WAITALL);
      send(cliente_socket, &(segmento->tamanio_segmento), sizeof(uint32_t), MSG_WAITALL);
      pthread_mutex_unlock(&mx_kernel);


      break;
    case CREATE_SEGMENT:
    		log_info(logger,"[KERNEL] recibido pedido crear segmento");
    		uint32_t id_seg;
    		uint32_t tam_segmento;
    		//int* nada;

    		pthread_mutex_lock(&mx_kernel);
			recv(cliente_socket, & pid, sizeof(uint16_t), 0);
			recv(cliente_socket, & id_seg, sizeof(uint32_t), 0);
			recv(cliente_socket, & tam_segmento, sizeof(uint32_t), 0);
			pthread_mutex_unlock(&mx_kernel);

			log_warning(logger,"Recibo: id:%d -tamanio:%d - pid:%d ",id_seg,tam_segmento,pid);
    		crearSegmento(pid, id_seg, tam_segmento);
    		t_list* tabla_proceso = buscarTabla(pid);
    		t_segmento* seg = buscarSegmento(tabla_proceso, id_seg);
    		log_info(logger, "[KERNEL] Envio de segmento id:%d para programa %d",id_seg, pid);
			pthread_mutex_lock(&mx_kernel);
			send(cliente_socket, &(seg->id_segmento), sizeof(uint32_t), MSG_WAITALL);
			send(cliente_socket, &(seg->direccion_base), sizeof(uint64_t), MSG_WAITALL);
			send(cliente_socket, &(seg->tamanio_segmento), sizeof(uint32_t), MSG_WAITALL);
			pthread_mutex_unlock(&mx_kernel);

			break;
    // Errores
    case -1:
      log_error(logger, "Cliente desconectado de %s...", server_name);
      return;
    default:
      log_error(logger, "Algo anduvo mal en el server KERNEL de %s", server_name);
      log_info(logger, "Cop: %d", cop);
      return;
    }
  }

  log_warning(logger, "El cliente KERNEL se desconecto de %s server", server_name);
  return;
}

//CPU
static void procesar_cpu(void * void_args) {
  t_procesar_conexion_args * args = (t_procesar_conexion_args * ) void_args;
  int cliente_socket = args -> fd;
  cpu_fd = cliente_socket;
  char * server_name = args -> server_name;
  free(args);
  uint32_t nro_marco;
  uint32_t desplazamiento;
  uint32_t dato;

  num_seg = malloc(sizeof(uint32_t));
  desplazamiento1 = malloc(sizeof(uint32_t));
  pid = malloc(sizeof(uint32_t));
  tamanio = malloc(sizeof(int));

  op_code cop;
  while (cliente_socket != -1) {

    if (recv(cliente_socket, & cop, sizeof(op_code), 0) != sizeof(op_code)) {
      log_info(logger, "Se ha finalizado la conexion");
      return;
    }

    switch (cop) {
    case DEBUG:
      log_info(logger, "debug");
      break;
    case CPU: log_info(logger, "RESPUESTA AL CONECTAR CPU");

    				//pthread_mutex_lock(&mx_cpu);
    				//send(cliente_socket, &(configuracion -> ENTRADAS_POR_TABLA), sizeof(uint16_t), 0);
    				//send(cliente_socket, &(configuracion -> TAM_PAGINA), sizeof(uint16_t), 0);
    				//pthread_mutex_unlock(&mx_cpu);
    				break;
    // Errores
    case MOV_IN:
    		log_info(logger, "Recibido pedido de MOV_IN");
    		recv_instruccion_memoria(cpu_fd, num_seg, desplazamiento1, pid, tamanio);
    		log_info(logger, "PID: %d - N° Segmento: %d, Desplazamiento: %d, Tamanio: %d", *pid, *num_seg, *desplazamiento1, *tamanio);
    		//log_info(logger, "xPID: %lu - N° Segmento: %lu, Desplazamiento: %lu, Tamanio: %d", *pid, *num_seg, *desplazamiento1, *tamanio);
    		void* leido = leerMemoria(*num_seg, *desplazamiento1, *pid, *tamanio);
    		send(cpu_fd, leido, *tamanio, 0);
    		break;
    	case MOV_OUT:
    		log_info(logger, "Recibido pedido de MOV_OUT");
    		cop = MOV_OUT_OK;
    		void* escribir=malloc(20);
    		recv_escribir_memoria(cpu_fd, num_seg, desplazamiento1, pid, tamanio, escribir);	//OJO que ahora num_seg, desplazamiento y pid son punteros
    		log_info(logger, "PID: %d - N° Segmento: %d, Desplazamiento: %d, Tamanio: %d, Escribir: %s", *pid, *num_seg, *desplazamiento1, *tamanio, escribir);
    		if(escribirEnMemoria( *num_seg,  *desplazamiento1,  *pid,  *tamanio-1,escribir)){
    		send(cpu_fd, &cop, sizeof(cop), 0); // hubieran usado op_codes :v
    				}	// else?
    			break;

    case -1:
    		log_error(logger, "Cliente desconectado de %s...", server_name);
    		return;
    default:
    		log_error(logger, "Algo anduvo mal en el server CPU de %s", server_name);
    		log_info(logger, "Cop: %d", cop);
    		return;
    }
  }

  log_warning(logger, "El cliente CPU se desconecto de %s server", server_name);
  return;
}

//FILESYSTEM
static void procesar_fileSystem(void * void_args) {
  t_procesar_conexion_args * args = (t_procesar_conexion_args * ) void_args;
  int cliente_socket = args -> fd;
  char * server_name = args -> server_name;
  free(args);

  op_code cop;
  while (cliente_socket != -1) {

    if (recv(cliente_socket, & cop, sizeof(op_code), 0) != sizeof(op_code)) {
      log_info(logger, "Se ha finalizado la conexion");
      return;
    }

    switch (cop) {
    case DEBUG:
      log_info(logger, "debug");
      break;
    case FS: log_info(logger, "RESPUESTA AL CONECTAR FILESYSTEM");
    				//pthread_mutex_lock(&mx_cpu);
    				//send(cliente_socket, &(configuracion -> ENTRADAS_POR_TABLA), sizeof(uint16_t), 0);
    				//send(cliente_socket, &(configuracion -> TAM_PAGINA), sizeof(uint16_t), 0);
    				//pthread_mutex_unlock(&mx_cpu);
    				break;
    // Errores
    case -1:
    		log_error(logger, "Cliente desconectado de %s...", server_name);
    		return;
    default:
    		log_error(logger, "Algo anduvo mal en el server de FS %s", server_name);
    		log_info(logger, "Cop: %d", cop);
    		return;
    }
  }

  log_warning(logger, "El cliente FS se desconecto de %s server", server_name);
  return;
}


int kernel_escuchar(char * server_name, int server_socket) {
  int cliente_socket = esperar_cliente(logger, server_name, server_socket);
  //sem_wait(&sem);
  if (cliente_socket != -1) {
    t_procesar_conexion_args * argsSev = malloc(sizeof(t_procesar_conexion_args));
    argsSev -> fd = cliente_socket;
    argsSev -> server_name = server_name;
    procesar_kernel(argsSev);
    return 1;
    // sem_post(&sem);
  }
  return 0;

}

int cpu_escuchar(char * server_name, int server_socket) {
  int cliente_socket = esperar_cliente(logger, server_name, server_socket);
  if (cliente_socket != -1) {
    t_procesar_conexion_args * argsSev = malloc(sizeof(t_procesar_conexion_args));
    argsSev -> fd = cliente_socket;
    argsSev -> server_name = server_name;
    procesar_cpu(argsSev);
    return 1;
  }
  return 0;
}

int fileSystem_escuchar(char * server_name, int server_socket) {
  int cliente_socket = esperar_cliente(logger, server_name, server_socket);
  if (cliente_socket != -1) {
    t_procesar_conexion_args * argsSev = malloc(sizeof(t_procesar_conexion_args));
    argsSev -> fd = cliente_socket;
    argsSev -> server_name = server_name;
    procesar_fileSystem(argsSev);
    return 1;
  }
  return 0;
}


void atenderCpu(){
	op_code cop;
	recv(cpu_fd, & cop, sizeof(op_code), 0);
	switch(cop){
	case MOV_IN:
		log_info(logger, "Recibido pedido de MOV_IN");
		recv_instruccion_memoria(cpu_fd, num_seg, desplazamiento1, pid, tamanio);
		log_info(logger, "PID: %d - N° Segmento: %d, Desplazamiento: %d, Tamanio: %d", *pid, *num_seg, *desplazamiento1, *tamanio);
		//log_info(logger, "xPID: %lu - N° Segmento: %lu, Desplazamiento: %lu, Tamanio: %d", *pid, *num_seg, *desplazamiento1, *tamanio);
		void* leido = leerMemoria(*num_seg, *desplazamiento1, *pid, *tamanio);
		send(cpu_fd, leido, *tamanio, 0);
		break;
	case MOV_OUT:
		log_info(logger, "Recibido pedido de MOV_OUT");
		cop = MOV_OUT_OK;
		void* escribir=malloc(20);
		recv_escribir_memoria(cpu_fd, num_seg, desplazamiento1, pid, tamanio, escribir);	//OJO que ahora num_seg, desplazamiento y pid son punteros
		log_info(logger, "PID: %d - N° Segmento: %d, Desplazamiento: %d, Tamanio: %d, Escribir: %s", *pid, *num_seg, *desplazamiento1, *tamanio, escribir);
		if(escribirEnMemoria( *num_seg,  *desplazamiento1,  *pid,  *tamanio-1,escribir)){
		send(cpu_fd, &cop, sizeof(cop), 0); // hubieran usado op_codes :v
				}	// else?

		break;
	 default:
		 break;
	}
}









/*
 * comunicacion.c
 *
 *  Created on: 12 abr 2023
 *      Author: utnso
 */
#include "comunicacion.h"

typedef struct {
  int fd;
  char * server_name;
}
t_procesar_conexion_args;

pthread_mutex_t mx_kernel= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_cpu = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mx_pagefault = PTHREAD_MUTEX_INITIALIZER;


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
      recv(cliente_socket, & cantidad_ids, sizeof(uint32_t), 0);
      pthread_mutex_unlock(&mx_kernel);

      t_list* tabla_de_segmentos = list_create();

      log_info(logger, "[KERNEL] Creando tabla para programa %d", pid);
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
  char * server_name = args -> server_name;
  free(args);
  uint32_t nro_marco;
  uint32_t desplazamiento;
  uint32_t dato;

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



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
    case INICIALIZAR: log_info(logger, "RESPUESTA AL CONECTAR KERNEL");
    				  break;
    /*case CREAR_TABLA:
      pid = 0;
      id_segmento = 0;
      int cantidad_ids = 0;
      op_code op;

      //pthread_mutex_lock(&mx_kernel);
      //recv(cliente_socket, & pid, sizeof(uint16_t), 0);
      //recv(cliente_socket, & cantidad_ids, sizeof(uint32_t), 0);
      //pthread_mutex_unlock(&mx_kernel);

      t_list* lista_tabla_de_paginas=list_create();

      log_info(logger, "[KERNEL] Creando tabla para programa %d", pid);

      int i = 0;/
    /*  while (i < cantidad_ids) {
        int sid = 0;
        int tamanio = 0;
        pthread_mutex_lock(&mx_kernel);
        recv(cliente_socket, & sid, sizeof(uint32_t), 0);
        recv(cliente_socket, & tamanio, sizeof(uint32_t), 0);
        pthread_mutex_unlock(&mx_kernel);
        //t_list* tabla_de_paginas = crear_tabla(pid);
       // log_info(logger, "[KERNEL] Tabla creada del Segmento %d con %d entradas ", sid, configuracion->ENTRADAS_POR_TABLA);
        list_add(lista_tabla_de_paginas, tabla_de_paginas);

        i++;
      }*/
      //list_add(lista_tablas_de_procesos,lista_tabla_de_paginas);
      //crear_estructura_clock(pid);


      //pthread_mutex_lock(&mx_kernel);
      //send(cliente_socket, & op, sizeof(op_code), 0);
      //pthread_mutex_unlock(&mx_kernel);
     // break;
   /* case ELIMINAR_ESTRUCTURAS:
      uint32_t tabla_paginas = 0;
      uint16_t pid = 0;

      pthread_mutex_lock(&mx_kernel);
      recv(cliente_socket, & tabla_paginas, sizeof(uint32_t), 0);
      recv(cliente_socket, & pid, sizeof(uint16_t), 0);
      pthread_mutex_unlock(&mx_kernel);

      log_info(logger, "[KERNEL] Eliminando tablas del proceso %d", pid);
      eliminar_estructuras(tabla_paginas, pid);

      break;
    case PAGEFAULT:
      //pthread_mutex_lock(&mx_pagefault);
      op_code op2 = PAGEFAULT;

      uint32_t num_segmento = 0;
      uint32_t num_pagina = 0;
       uint16_t pid_actual = 0;

      pthread_mutex_lock(&mx_kernel);
      recv(cliente_socket, &pid_actual, sizeof(uint16_t), 0);
      recv(cliente_socket, &num_segmento, sizeof(int32_t), 0);
      recv(cliente_socket, &num_pagina, sizeof(uint32_t), 0);
      pthread_mutex_unlock(&mx_kernel);

      uint32_t nro_marco = tratar_page_fault(num_segmento, num_pagina, pid_actual);
      log_info(logger, "[KERNEL] Numero de marco obtenido = %d", nro_marco);
      usleep(configuracion -> RETARDO_MEMORIA * 1000);
      pthread_mutex_lock(&mx_kernel);
      send(cliente_socket, & op2, sizeof(uint32_t), 0);
      pthread_mutex_unlock(&mx_kernel);
      //pthread_mutex_unlock(&mx_pagefault);

      break;*/
    // Errores
    case -1:
      log_error(logger, "Cliente desconectado de %s...", server_name);
      return;
    default:
      log_error(logger, "Algo anduvo mal en el server de %s", server_name);
      log_info(logger, "Cop: %d", cop);
      return;
    }
  }

  log_warning(logger, "El cliente se desconecto de %s server", server_name);
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
    case INICIALIZAR: log_info(logger, "RESPUESTA AL CONECTAR CPU");
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
    		log_error(logger, "Algo anduvo mal en el server de %s", server_name);
    		log_info(logger, "Cop: %d", cop);
    		return;
    }
  }

  log_warning(logger, "El cliente se desconecto de %s server", server_name);
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
    case INICIALIZAR: log_info(logger, "RESPUESTA AL CONECTAR FILESYSTEM");
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
    		log_error(logger, "Algo anduvo mal en el server de %s", server_name);
    		log_info(logger, "Cop: %d", cop);
    		return;
    }
  }

  log_warning(logger, "El cliente se desconecto de %s server", server_name);
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



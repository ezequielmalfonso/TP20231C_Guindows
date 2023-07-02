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
int kernel_socket;
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
  kernel_socket = cliente_socket;
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

    		pthread_mutex_lock(&mx_kernel);
			recv(cliente_socket, & pid, sizeof(uint16_t), 0);
			recv(cliente_socket, & id_seg, sizeof(uint32_t), 0);
			recv(cliente_socket, & tam_segmento, sizeof(uint32_t), 0);
			pthread_mutex_unlock(&mx_kernel);

			log_warning(logger,"Recibo: id:%d -tamanio:%d - pid:%d ",id_seg,tam_segmento,pid);

			if(noHayEspacio(tam_segmento)){
				log_error(logger, "NO HAY ESPACIO DISPONIBLE");
				op_code cop_fail = CREATE_SEGMENT_FAIL;
				pthread_mutex_lock(&mx_kernel);
				send(cliente_socket, &(cop_fail), sizeof(uint32_t), MSG_WAITALL);
				pthread_mutex_unlock(&mx_kernel);
				break;
			}else if(!hayEspacio(tam_segmento)){
				log_error(logger, "SOLICITAR COMPACTACION");
				op_code cop_comp = CREATE_SEGMENT_COMPACTO;
				pthread_mutex_lock(&mx_kernel);
				send(cliente_socket, &(cop_comp), sizeof(op_code), MSG_WAITALL);
				pthread_mutex_unlock(&mx_kernel);
				pthread_mutex_lock(&mx_kernel);
				recv(cliente_socket, & cop_comp, sizeof(op_code), 0);
				pthread_mutex_unlock(&mx_kernel);
				compactacion();

				break;
			}else{
	    		crearSegmento(pid, id_seg, tam_segmento);
	    		t_list* tabla_proceso = buscarTabla(pid);
	    		t_segmento* seg = buscarSegmento(tabla_proceso, id_seg);
	    		log_info(logger, "[KERNEL] Envio de segmento id:%d para programa %d",seg->id_segmento, pid);
	    		op_code cop_comp_ok = CREATE_SEGMENT_OK;
				pthread_mutex_lock(&mx_kernel);
				send(cliente_socket, &(cop_comp_ok), sizeof(uint32_t), MSG_WAITALL);
				send(cliente_socket, &(seg->id_segmento), sizeof(uint32_t), MSG_WAITALL);
				send(cliente_socket, &(seg->direccion_base), sizeof(uint64_t), MSG_WAITALL);
				send(cliente_socket, &(seg->tamanio_segmento), sizeof(uint32_t), MSG_WAITALL);
				pthread_mutex_unlock(&mx_kernel);
				break;
			}

    case DELETE_SEGMENT:
        		log_info(logger,"[KERNEL] Recibido pedido de Eliminar segmento");
        		uint32_t id_seg_delete;

        		pthread_mutex_lock(&mx_kernel);
    			recv(cliente_socket, & pid, sizeof(uint16_t), 0);
    			recv(cliente_socket, & id_seg_delete, sizeof(uint32_t), 0);
    			pthread_mutex_unlock(&mx_kernel);

    			log_warning(logger,"Recibo eliminar: id:%d - pid:%d ",id_seg_delete,pid);

    			eliminarSegmentoProceso(pid, id_seg_delete);
    			op_code cop_del = DELETE_SEGMENT_OK;
				pthread_mutex_lock(&mx_kernel);
				send(cliente_socket, &(cop_del), sizeof(uint32_t), MSG_WAITALL);
				pthread_mutex_unlock(&mx_kernel);

    			break;
    case MAKE_COMPACTATION:
    			log_info(logger,"[KERNEL] Recibido pedido de COMPACTAR");

    			// compacto

    			op_code cop_comp_make = FIN_COMPACTATION;

    			pthread_mutex_lock(&mx_kernel);
				send(cliente_socket, &(cop_comp_make), sizeof(uint32_t), MSG_WAITALL);
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

t_segmento* buscarPrimerHueco(){
	t_segmento* hueco1;
	t_segmento* hueco2=list_get(tabla_de_huecos,0);

		int i;
		for(i=0;i<list_size(tabla_de_huecos);i++)
		{
			hueco1 = list_get(tabla_de_huecos,i);
			log_info(logger,"Direccion del hueco leida:%d",hueco1->direccion_base);
			if(hueco1->direccion_base < hueco2->direccion_base){
				hueco2=hueco1;
			}
		}
		log_info(logger,"Direccion del hueco a llenar:%d",hueco2->direccion_base);
		return hueco2;

}
void eliminarHueco(t_segmento* h){
	int i;
	for(i = 0;i<list_size(tabla_de_huecos);i++){
	if(list_get(tabla_de_huecos,i)==h){
		log_info(logger,"encontre hueco para eliminar");
		break;
	}

	}
	t_segmento* aux =list_get(tabla_de_huecos,i);
	log_info(logger,"hueco a eliminar pos:%d - base:%d - tam:%d",i,aux->direccion_base,aux->tamanio_segmento);

	list_remove_and_destroy_element(tabla_de_huecos,i,free);
	log_info(logger,"elimine hueco");

}

t_segmento* buscarSiguienteSegmento(t_segmento* h){
	uint64_t pos_buscada = h->direccion_base+h->tamanio_segmento;
	log_info(logger,"pos buscada:%d",pos_buscada);
	for(int i = 0; i<list_size(tabla_de_paginas);i++){
		t_nodoDePagina* nodo = list_get(tabla_de_paginas,i);
		t_list* t_proces = nodo->tablaDelProceso;
		for(int j = 0; j<list_size(t_proces);j++){
			t_segmento* s = list_get(t_proces,j);
			if(pos_buscada == s->direccion_base){
				log_info(logger,"encontre el segmento %d",s->direccion_base);
				op_code cop_comp= ACTUALIZAR_SEGMENTO;
			      pthread_mutex_lock(&mx_kernel);
				send(kernel_socket, &(cop_comp), sizeof(op_code), MSG_WAITALL);
				send(kernel_socket,&(nodo->id_proceso),sizeof(uint32_t), MSG_WAITALL);
			      pthread_mutex_unlock(&mx_kernel);
				log_info(logger, "segmento encontado pid:%d - sid:%d - base:%d",nodo->id_proceso,s->id_segmento,s->direccion_base);
				return s;
			}
		}
	}
	t_segmento* h_sig;
	for(int i = 0;i<list_size(tabla_de_huecos);i++){
	h_sig=list_get(tabla_de_huecos,i);
	if(pos_buscada== h_sig->direccion_base){
		log_info(logger,"encontre hueco base:%d",h_sig->direccion_base);
		h_sig->direccion_base=h->direccion_base;
		h_sig->tamanio_segmento+=h->tamanio_segmento;
		eliminarHueco(h);
		log_info(logger,"pase el eliminar");
		t_segmento* r=malloc(sizeof(t_segmento));
		r->id_segmento=84;
		return r;

	}
	}
}
void* leerMemoriaDesdeDireccion(uint64_t base,uint32_t tam){
void* leer = "hola";
//memcpy(leer,memoria+base,tam);
log_info(logger,"leyo");
	return leer;
}
void escribirMemoriaDesdeDireccion(void* leido,t_segmento* hueco,uint32_t tam){

	//memcpy(memoria+hueco->direccion_base,leido,tam);
	log_info(logger,"escribio");
}


void moverSeg(t_segmento* seg,t_segmento* hueco){
	void* leido = leerMemoriaDesdeDireccion(seg->direccion_base,seg->tamanio_segmento);
	escribirMemoriaDesdeDireccion(leido,hueco,seg->tamanio_segmento);
	seg->direccion_base = hueco->direccion_base;
	hueco->direccion_base+=seg->tamanio_segmento;
	log_info(logger,"nueva direccion del seg %d :%d",seg->id_segmento,seg->direccion_base);
    pthread_mutex_lock(&mx_kernel);
	send(kernel_socket, &seg->direccion_base, sizeof(uint64_t), MSG_WAITALL);
	send(kernel_socket,&(seg->id_segmento),sizeof(uint32_t), MSG_WAITALL);
    pthread_mutex_unlock(&mx_kernel);

}
void llenarHueco(t_segmento* hueco){
	t_segmento* seg=buscarSiguienteSegmento(hueco);
	log_info(logger,"id:%d",seg->id_segmento);

	if(seg->id_segmento != 84){
	log_info(logger,"encontro segmento id:%d - tam:%d",seg->id_segmento,seg->tamanio_segmento);
	moverSeg(seg,hueco);
	}else{
		log_info(logger,"es un hueco");
		free(seg);
	}
	}
void compactacion(){

	t_segmento* hueco;
	while(list_size(tabla_de_huecos)>1){
		hueco = buscarPrimerHueco();
		llenarHueco(hueco);
		log_info(logger,"quedan %d huecos",list_size(tabla_de_huecos));
}
	op_code cop = FIN_COMPACTATION;
	pthread_mutex_lock(&mx_kernel);
		send(kernel_socket, &cop, sizeof(op_code), MSG_WAITALL);
	    pthread_mutex_unlock(&mx_kernel);
		log_info(logger,"Termine compactacion");


}






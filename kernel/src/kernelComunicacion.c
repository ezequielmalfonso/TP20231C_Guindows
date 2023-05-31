/*
 * comunicacion.c
 *
 *  Created on: 6 abr 2023
 *      Author: utnso
 */
#include "kernelComunicacion.h"

uint16_t pid_nuevo=0;

typedef struct {
    int fd;
    char* server_name;
} t_procesar_conexion_args;
//pthread_mutex_t pid_xd = PTHREAD_MUTEX_INITIALIZER;
int cliente_socket;
char* registros;

static void procesar_conexion(void* void_args) {
	t_procesar_conexion_args* args = (t_procesar_conexion_args*) void_args;
	int cliente_socket = args->fd;
	char* server_name = args->server_name;
	double   estimado_rafaga_inicial;
	uint32_t tiempo_llegada_a_ready;   // TODO unixepoch: Averiguar como funciona y como la usamos
	free(args);

	t_instrucciones* mensaje=malloc(sizeof(t_instrucciones));
	mensaje=recibir_instrucciones(cliente_socket);

	// Una vez recibidas las instruccion desde consola creo el PCB
    PCB_t* proceso = malloc(sizeof(PCB_t));
	proceso = pcb_create();
	// Aca inicializo los valores para el PCB
	//TODO pasarle los valores de inicializacion al PCB
	// casi un char* ??
	t_link_element* aux1 = mensaje->listaInstrucciones->head;
	INSTRUCCION* auxl2 = malloc(sizeof(INSTRUCCION));
	auxl2 = aux1->data;

	asignacion_tamanio_registros(auxl2);

	//TODO preguntar si asi vamos bien para darle el tamaño a los registros antes de enviarlo al CPU y
	//que el CPU Los tiene que cargar para devolverlos

	// uint32_t archivos_abiertos; TODO de que tipo seria??? Una lista o un array?

	estimado_rafaga_inicial = 0; //configuracion->ESTIMACION_INICIAL;


	// posible semaforo
	pcb_set(  proceso
			, pid_nuevo
			, mensaje->listaInstrucciones
			, 0   // PC
			, registros
			//, tabla_segmentos
			//, archivos_abiertos
			, estimado_rafaga_inicial
			, tiempo_llegada_a_ready
			, cliente_socket
			);
	t_tiempos_rafaga_anterior* rafaga_inicial = malloc(sizeof(t_tiempos_rafaga_anterior));
	rafaga_inicial->pid = pid_nuevo;
	rafaga_inicial->tiempo_in_exec = 0;
	rafaga_inicial->tiempo_out_exec = 0;
	list_add(list_rafa_anterior, rafaga_inicial);
	pid_nuevo++;
	//fin semaforo

	//Solicito a memoria tabla de segmentos
	solicitar_tabla_de_segmentos(proceso);

	t_segmento* segmento = malloc(sizeof(t_segmento));

	uint32_t id_segmento      = 0;
	uint64_t direccion_base   = 0;
	uint32_t tamanio_segmento = 0;

	// Recibir el segmento 0 desde memoria y ponerlo en PCB
	recv(memoria_fd, &id_segmento, sizeof(uint32_t), 0);
	recv(memoria_fd, &direccion_base, sizeof(uint64_t), 0);
	recv(memoria_fd, &tamanio_segmento, sizeof(uint32_t), 0);

	segmento->id_segmento      = id_segmento;
	segmento->direccion_base   = direccion_base;
	segmento->tamanio_segmento = tamanio_segmento;

	//agrego el PCB creado el segmento recibido desde memoria
	// TODO preguntar
	list_add(proceso->tabla_de_segmentos, segmento);

	log_info(logger,"Recibiendo Segmento 0 (Compartido) creado: Id: %d - Dir Base: %d - Tamanio: %d", id_segmento, direccion_base, tamanio_segmento);

	list_destroy(mensaje->listaInstrucciones);
	// destroy lists de segmentos
	free(mensaje);


	// creo un hilo por cada proceso y lo voy metiendo en la cola_new

	pthread_mutex_lock(&mx_cola_new);
	queue_push(cola_new,proceso);
	pthread_mutex_unlock(&mx_cola_new);

	//  saco el proceso de la cola NEW y lo paso A READY FIFO
	sem_wait(&s_multiprogramacion_actual);
	pthread_mutex_lock(&mx_cola_new);
	proceso=queue_pop(cola_new);
	pthread_mutex_unlock(&mx_cola_new);



	proceso->tiempo_llegada_a_ready = temporal_gettime(reloj_inicio);
	pthread_mutex_lock(&mx_cola_ready);
	queue_push(cola_ready,proceso);
	pthread_mutex_unlock(&mx_cola_ready);
	//pthread_mutex_lock(&mx_log);
	log_info(logger,"Se crea el proceso %d en NEW",proceso->pid);
	log_info(logger,"PID: %d - Estado Anterior: NEW - Estado Actual: READY", proceso->pid);

	sem_post(&s_cont_ready);
	sem_post(&s_ready_execute);

	//log_info(logger,"Se crea el proceso %d en NEW", proceso->pid);


	return ;
}
// SOLICITO TABLA DE SEGMENTOS A MEMORIA
// TODO le mando el proceso y memoria que me responde en este momento?
void solicitar_tabla_de_segmentos(PCB_t* pcb){ log_info(logger, "Envio solicitud a MEMORIA");
	op_code op=CREAR_TABLA;
	pthread_mutex_lock(&mx_memoria);
    send(memoria_fd,&op,sizeof(op_code),0);
	send(memoria_fd,&(pcb->pid),sizeof(uint16_t),0);
	pthread_mutex_unlock(&mx_memoria);
}


int server_escuchar(char* server_name, int server_socket) {
    cliente_socket = esperar_cliente(logger, server_name, server_socket);

    if (cliente_socket != -1) {
        pthread_t hilo;
        t_procesar_conexion_args* args = malloc(sizeof(t_procesar_conexion_args));
        args->fd = cliente_socket;
        args->server_name = server_name;
        pthread_create(&hilo, NULL, (void*) procesar_conexion, (void*) args);
        //pthread_detach(hilo);
        return 1;
    }
    return 0;
}

//CLIENTE
//CPU
int generar_conexiones(int* cpu_fd, t_config_kernel* configuracion) {
    char* port_cpu = string_itoa(configuracion->PUERTO_CPU);

    *cpu_fd = crear_conexion(
            logger,
            "CPU",
            configuracion->IP_CPU,
            port_cpu
    );

    free(port_cpu);

    return *port_cpu != 0;
}

//MEMORIA
int generar_conexion_memoria(int* memoria_fd, t_config_kernel* configuracion) {
    char* port_memoria = string_itoa(configuracion->PUERTO_MEMORIA);

    *memoria_fd = crear_conexion(
            logger,
            "MEMORIA",
            configuracion->IP_MEMORIA,
            port_memoria
    );

    free(port_memoria);

    return *memoria_fd != 0;
}

//FILESYSTEM
int generar_conexion_fileSystem(int* file_system_fd, t_config_kernel* configuracion) {
    char* port_file_system = string_itoa(configuracion->PUERTO_FILESYSTEM);

    *file_system_fd = crear_conexion(
            logger,
            "FILESYSTEM",
            configuracion->IP_FILESYSTEM,
			port_file_system
    );

    free(port_file_system);

    return *file_system_fd != 0;
}


void asignacion_tamanio_registros(INSTRUCCION* instruccion){

	if(strcmp(instruccion->parametro1, "AX" ) == 0 || strcmp(instruccion->parametro1, "BX") == 0 || strcmp(instruccion->parametro1, "CX") == 0 || strcmp(instruccion->parametro1, "DX") == 0){
			char* registros;
			registros = (char*)malloc(4);
		}else if(strcmp(instruccion->parametro1, "EAX" ) == 0 || strcmp(instruccion->parametro1, "EBX") == 0 || strcmp(instruccion->parametro1, "ECX") == 0 || strcmp(instruccion->parametro1, "EDX") == 0){
			char* registros;
			registros = (char*)malloc(8);
		}else if(strcmp(instruccion->parametro1, "RAX" ) == 0 || strcmp(instruccion->parametro1, "RBX") == 0 || strcmp(instruccion->parametro1, "RCX") == 0 || strcmp(instruccion->parametro1, "RDX") == 0){
			char* registros;
			registros = (char*)malloc(16);
		}else{
			log_info(logger, "Algo fallo");
		}
}

/*
 * protocolo.h
 *
 *  Created on: 6 abr 2023
 *      Author: utnso
 */
#ifndef SRC_PROTOCOLO_H_
#define SRC_PROTOCOLO_H_

#include <inttypes.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include "pcb.h"

typedef enum {
    DEBUG = 69,
	SET,
	MOV_IN,
	MOV_OUT,
	F_READ,
	F_WRITE,
	F_TRUNCATE,
	F_SEEK,
	F_CREATE,
	CREATE_SEGMENT,
	CREATE_SEGMENT_FAIL,
	CREATE_SEGMENT_OK,
	CREATE_SEGMENT_COMPACTO,
	MAKE_COMPACTATION,
	FIN_COMPACTATION,
	ACTUALIZAR_SEGMENTO,
	IO,
	WAIT,
	SIGNAL,
	F_OPEN,
	F_CLOSE,
	DELETE_SEGMENT,
	DELETE_SEGMENT_OK,
	EXIT,
	YIELD,
	DISPATCH,
	PAGEFAULT,
	CONTINUE,
	BLOCKED,
	INTERRUPT,
	PANTALLA,
	TECLADO,
	SOLICITUD_NRO_MARCO,
	SIGSEGV,
	ESCRITURA_OK,
	ELIMINAR_ESTRUCTURAS,
	CREAR_TABLA,
	INICIALIZAR,
	KERNEL,
	CPU,
	FS,
	F_OPEN_OK,
	F_OPEN_FAIL,
	OK,
	F_CREATE_FAIL,
	F_CREATE_OK,
	F_EXISTS,
	F_TRUNCATE_OK,
	F_TRUNCATE_FAIL,
	F_WRITE_OK,
	F_WRITE_FAIL,
	F_READ_OK,
	F_READ_FAIL,
	MOV_OUT_OK,
	MOV_IN_OK
	}op_code;

typedef struct {
    uint32_t size; // Tamaño del payload
    void* stream; // Payload
} t_buffer;

typedef struct{
	uint32_t elementosLista;
	t_list* listaInstrucciones;
   // uint32_t cantSegmentos;
   // t_list* listaTamSegmentos;
} t_instrucciones;


//INSTRUCCIONES CONSOLA-KERNEL
void* serializar_instrucciones_tam(uint32_t size, t_list* lista);
t_instrucciones* deserializar_instrucciones(t_buffer* buffer);
void enviar_instrucciones(int socket_fd, t_list* lista);
t_instrucciones* recibir_instrucciones(int socket_fd);
uint32_t calcular_instrucciones_buffer_size(t_list* lista);

//ENVIO PROCESO KERNEL CPU
bool send_proceso(int fd, PCB_t *proceso,op_code codigo);
static void* serializar_proceso(size_t* size, PCB_t *proceso, op_code codigo);
bool recv_proceso(int fd, PCB_t* proceso);
static void deserializar_proceso(void* stream, PCB_t* proceso);



//ENVIO INSTRUCCIONES KERNEL-FILESYSTEM
bool send_archivo(int fd, char*, char*, char*, char*, op_code codigo);
static void* serializar_instruccion(size_t* size, char* param1, char* param2, char* param3, char* posicion, op_code codigo);
bool recv_instruccion(int fd, char* param1, char* param2, char* param3, char* posicion);
static void deserializar_instruccion(void* stream, char* param1, char* param2, char* param3, char* posicion);

//Envio instruccion CPU - MEMORIA
static void deserializar_instruccion_memoria(void* stream, uint32_t* param1, uint32_t* param2, uint32_t* param3,int* param4);
bool recv_instruccion_memoria(int fd, uint32_t* num_seg, uint32_t* desplazamiento, uint32_t* pid,int* tamanio);
static void* serializar_instruccion_memoria(size_t* size, uint32_t param1, uint32_t param2, uint32_t param3,int param4, op_code codigo);
bool send_pedido_memoria(int fd, uint32_t num_seg, uint32_t desplazamiento, uint32_t pid,int tamanio, op_code codigo);
//escribir
bool recv_escribir_memoria(int fd, uint32_t* num_seg, uint32_t* desplazamiento, uint32_t* pid,int* tamanio, void* escribir) ;
static void deserializar_escribir_memoria(void* stream, uint32_t* param1, uint32_t* param2, uint32_t* param3,int* param4, void* escribir) ;
static void* serializar_escribir_memoria(size_t* size, uint32_t param1, uint32_t param2, uint32_t param3,void* param4, int tam, op_code codigo) ;
bool send_escribir_memoria(int fd, uint32_t num_seg, uint32_t desplazamiento, uint32_t pid,void* escribir,int tamanio, op_code codigo);
//ENVIO FS a MEMORIA
bool send_fs_memoria(int memoria_fd, char* direccion_fisica, int tamanio, op_code codigo);
static void* serializar_fs_memoria(size_t* size, char* direccion_fisica, int tamanio, op_code codigo);
//recibo En memoria de FS
bool recv_fs_memoria(int fd, char* direccion_fisica, int* tamanio);
void deserializar_fs_memoria(void* stream, char* direccion_fisica, int* tamanio);
bool send_fs_memoria_read(int memoria_fd, char* dir_fisica, int tamanio,void* leido, op_code codigo);
static void* serializar_fs_memoria_read(size_t* size, char* dir_fisica, int tamanio,void* leido, op_code codigo);
bool recv_fs_memoria_read(int fd, char* direccion_fisica, int* tamanio, void** leido);
void deserializar_fs_memoria_read(void* stream, char* direccion_fisica, int* tamanio, void** leido);

#endif /* SRC_PROTOCOLO_H_ */

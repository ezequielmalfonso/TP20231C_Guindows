#ifndef MMU_H_
#define MMU_H_

#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <inttypes.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include "protocolo.h"
#include "socket.h"
#include "cpuConfig.h"
#include "cpu.h"
#include <semaphore.h>

extern int cliente_socket;

uint32_t desplazamiento(int logica);
int mov_in(char direccion_logica[20], char registro[20], PCB_t* pcb);
uint32_t num_seg(int logica);
int mov_out(char* direccion_logica,char* registro,PCB_t* pcb);
int checkSegmentetitonFault(uint32_t desplazamiento, uint32_t n_segmento,PCB_t* pcb);
void set_registro(char* registro,char* registro_recibido, PCB_t* pcb);
void parseo_registro(char registro_recibido[20], PCB_t* pcb,int tamanio);
int calcularTam(char* registro);
void* leer_registro(char* registro, int tamanio);
void* traducirAFisica(void* direccion_logica, PCB_t* pcb, int tamanio, char* valor, bool leer);

#endif /* MMU_H_ */

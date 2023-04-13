/*
 * memoriaUtils.h
 *
 *  Created on: 12 abr 2023
 *      Author: utnso
 */

#ifndef MEMORIA_UTILS_H_
#define MEMORIA_UTILS_H_

#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<stdbool.h>
#include<pthread.h>
#include <stdint.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/string.h>
#include<readline/readline.h>
#include<semaphore.h>
#include<commons/collections/queue.h>
#include<commons/collections/list.h>
#include<commons/collections/dictionary.h>

void inicializar_memoria();
void apagar_memoria();


#endif /* MEMORIA_UTILS_H_ */

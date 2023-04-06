/*
 ============================================================================
 Name        : consola.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Modulo Consola
 ============================================================================
 */

#include "consola.h"

int main(int argc, char** argv) {

	// ESTO NO VA ES SOLO PARA PROBAR
		//argv[1] = "/home/utnso//Desktop/TP 2do cuatri 2022/tp-2022-2c-Grupo-54/consola/instrucciones.txt";
		argv[2] = "/home/utnso/tp-2023-1c-Guindows/consola/consolaConfig/consola.conf";

		int kernel_fd;
		cargarConfiguracion(argv[2]);
		generar_conexion(&kernel_fd, configuracion);
		uint32_t valor=0;

		/*int i = 0;
		while(segmentos[i]!=NULL){
				printf("SEGMENTO: %s",segmentos[i]);
				i++;
			}*/

		//printf("Path: %s \n", argv[2]);
		// Parseo del archivo y armado de lista
		//t_list* listaInstrucciones = list_create();

		// argv[1] es el path de la primera posicion que recibe el main
		//parseo_instrucciones(argv[1],listaInstrucciones);

		//enviar_instrucciones(kernel_fd, listaInstrucciones, configuracion->SEGMENTOS);

	//return 1;

}

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

	    // TODO ESTO NO VA ES SOLO PARA PROBAR
		//argv[1] = "/home/utnso/tp-2023-1c-Guindows/consola/instrucciones.txt";
		//argv[2] = "/home/utnso/tp-2023-1c-Guindows/consola/consolaConfig/consola.conf";

		int kernel_fd;
		cargarConfiguracion(argv[2]);
		generar_conexion(&kernel_fd, configuracion);

		//printf("Path: %s \n", argv[2]);
		// Parseo del archivo y armado de lista
		t_list* listaInstrucciones = list_create();

		// argv[1] es el path de la primera posicion que recibe el main
		parseo_instrucciones(argv[1],listaInstrucciones);

		//printf("La lista tiene %d elementos", k);
		//log_warning(logger, "ESTOY EN CONSOLA ANTES DE enviar las intrucciones");
		enviar_instrucciones(kernel_fd, listaInstrucciones);

		//TODO : Para cuando esten todos los modulos arriba aca va while esperandocodigo de operacion desde kernel
		//       pcb->cliente_fd
		//log_warning(logger, "ESTOY EN CONSOLA ANTES DEL WHILE");
		while(1){
				op_code cod,cop_aux;
				//log_error(logger, "ESTOY EN CONSOLA ANTES de recibir el codop");
				recv(kernel_fd,&cod,sizeof(op_code),MSG_WAITALL);

				switch(cod){
					case EXIT:
						log_warning(logger, "Se realizo el envio se desconecto del kernel - Finalizo consola por EXIT");
						limpiarConfiguracion();
						exit(-1);
					break;
					default:
						break;
				}
			}
	//return 1;

}



void parseo_instrucciones(char* path_instrucciones,t_list* listaInstrucciones){

	int i=0;

		FILE* pseudocode;
		//printf("Path: %s \n", path_instrucciones);
		//printf("Parametro a envia con LISTA: %d \n", atoi(argv[2]));

		pseudocode = fopen(path_instrucciones,"r");   		  // ABRO MODO Lectura
		char bufer[LONGITUD_MAXIMA_LINEA];		              // Genero buffer

		if(!pseudocode){
			printf("No se pudo acceder al archivo\n");
			return ;
		}

		char **linea;
		char *separador = " ";

		//t_list* listaInstrucciones = list_create();

		while(fgets(bufer, LONGITUD_MAXIMA_LINEA, pseudocode))
		{
			// ACA IMPRIMO CONTENIDO DE CADA LINEA
			linea =  string_split(bufer, separador);

			INSTRUCCION *instrucs;
			instrucs = (INSTRUCCION*)malloc(sizeof(INSTRUCCION));

			if(!instrucs){
					printf("No se ha podido reservar memoria.\n");
					exit(1);
			}
			//instrucs->comando = malloc(sizeof(instrucs->comando));
			//instrucs->parametro;
			//instrucs->parametro2;

			for(i = 0; linea[i] != '\0'; i++)
			{
				if( i == 0  )					//==>  1ra posicion
				{
					strcpy(instrucs->comando,linea[i]);
				}
				if( i == 1  ){					//==>  2da posicion   si no es null se mete el dato en la variable
					strcpy(instrucs->parametro1,linea[i]);
				}
				if( i == 2  ){					//==>  2da posicion  si no es null se mete el dato en la variable
					strcpy(instrucs->parametro2,linea[i]);
				}
				if( i == 3  ){					//==>  3ra posicion  si no es null se mete el dato en la variable
					strcpy(instrucs->parametro3,linea[i]);
				}
			}

		list_add(listaInstrucciones,instrucs);

		free(linea);
		}

		/*	ESTA LINEA ES SOLO PARA PROBAR QUE SE GENERE BIEN LA LISTA CON LOS DATOS PARSEADOS DEL ARCHIVO DE INTRUCCIONES
		 t_link_element* aux1 = listaInstrucciones->head;
		printf("Verificamos la lista:\n");
		while( aux1!=NULL )
		{
			INSTRUCCION* auxl2 = aux1->data;
			printf("Comando: %s | Par1: %s | Par2: %s | Par3: %s \n", auxl2->comando, auxl2->parametro1, auxl2->parametro2, auxl2->parametro3 );

			aux1 = aux1->next;
		}*/
}

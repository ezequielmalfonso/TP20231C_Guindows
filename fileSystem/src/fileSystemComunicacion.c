/*
 * comunicacion.c
 *
 *  Created on: 13 abr 2023
 *      Author: utnso
 */

#include "fileSystemComunicacion.h"

int cliente_socket;
typedef struct {
    int fd;
    char* server_name;
} t_procesar_conexion_args;

t_FCB* FCB_archivo;

static void procesar_conexion(void* void_args) {
 t_procesar_conexion_args* args = (t_procesar_conexion_args*) void_args;
 int cliente_socket = args->fd;
 char* server_name = args->server_name;
 free(args);

 op_code cop;
 while (cliente_socket != -1) {

	 if (recv(cliente_socket, &cop, sizeof(op_code), 0) != sizeof(op_code)) {
		 log_info(logger, "Se ha finalizado la conexion");
		 return;
	 }

	 char parametro1[20], parametro2[20], parametro3[20];
	 char* pathArchivo;
	 char* nombre;

	 recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
	 nombre = strtok(parametro1, "\n");
	 pathArchivo = string_from_format("%s/%s", configuracion->PATH_FCB, nombre);

	 switch (cop) {
	 	 FILE* fd_archivo; // antes era un int y se usaba open, ahora fopen
	 	 case DEBUG:
	 		 log_info(logger, "debug");
	 		 break;

	 	 case F_EXISTS:
	 		 //recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
	 		 //nombre = strtok(parametro1, "\n");
			 log_info(logger, "Se recibio F_EXISTS para archivo %s", parametro1);

			 //if(recorrerFCBs(configuracion->PATH_FCB,strtok(parametro1, "\n")) == -1){
			 if(!fileExiste(pathArchivo)){
			 	 cop=0;
			 	 send(cliente_socket, &cop, sizeof(op_code), 0);

			 }else {
				 cop=1;
				 send(cliente_socket, &cop, sizeof(op_code), 0);

			 }

	 		  break;
		 case F_OPEN:
			 //recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 //nombre = strtok(parametro1, "\n");
			 log_info(logger, "Se recibio F_OPEN para archivo %s", parametro1);


			// if(recorrerFCBs(configuracion->PATH_FCB, strtok(parametro1, "\n")) == -1){
			if(!fileExiste(pathArchivo)){
				 cop=F_OPEN_FAIL;
				 send(cliente_socket,&cop, sizeof(op_code), 0);
				 log_info(logger, "El archivo %s no existe", parametro1);
			 }	else {
				 cop=F_OPEN_OK;
				 send(cliente_socket,&cop, sizeof(op_code), 0);
				 log_info(logger, "Se ha abierto el archivo: %s", parametro1);
			 }

			 break;

		 case F_CREATE:
			 log_info(logger, "F_CREATE");
			 //recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 //nombre = strtok(parametro1, "\n");
			 log_info(logger, "Crear archivo: %s", parametro1);
			 //t_config *config_create(char *path);

			 if(fileExiste(pathArchivo)) {
				 log_error(logger, "Ya existe %s", pathArchivo);
				 cop = F_CREATE_FAIL;	//No debe entrar nunca por este
				 send(cliente_socket, &cop, sizeof(op_code), 0);
				 break;
			 }
			 FILE* fcb_new = fopen(pathArchivo, "w");
			 if(!fcb_new) {
				 log_error(logger, "Error inesperado al crear archivo");
				 break;
			 }

			 free(pathArchivo);
			 char* linea = string_from_format("NOMBRE_ARCHIVO=%s\n", nombre);
			 fwrite(linea, sizeof(char), string_length(linea), fcb_new);
			 free(linea);
			 linea = string_from_format("TAMANIO_ARCHIVO=%s\n", "0");
			 fwrite(linea, sizeof(char), string_length(linea), fcb_new);
			 free(linea);
			 linea = string_from_format("PUNTERO_DIRECTO=%s\n", "0");	//TODO: agregar esto al FCB
			 fwrite(linea, sizeof(char), string_length(linea), fcb_new);
			 free(linea);
			 linea = string_from_format("PUNTERO_INDIRECTO=%s", "0");
			 fwrite(linea, sizeof(char), string_length(linea), fcb_new);
			 free(linea);

			 fclose(fcb_new);

			 //log_info(logger, "FCB de %s creado correctamente",);
			 //free(nombre) no se si va o no
			 cop = F_CREATE_OK;
			 send(cliente_socket, &cop, sizeof(op_code), 0);

			 /*kejesto?
			 fwrite(fd_archivo,"NOMBRE_ARCHIVO: %s", parametro1);
			 fwrite(fd_archivo,	"TAMANIO_ARCHIVO: %d", 0);
			 fwrite(fd_archivo, "PUNTERO_DIRECTO: ");
			 fwrite(fd_archivo, "PUNTERO_INDIRECTO: ");
			 */
			 break;

		 case F_CLOSE:
			 //recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_CLOSE con parametros %s, %s y %s", parametro1, parametro2, parametro3);
			 break;

		 case F_SEEK:
			 //recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_SEEK con parametros %s, %s y %s", parametro1, parametro2, parametro3);
			 break;

		 case F_TRUNCATE:
			 //recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_TRUNCATE con parametros %s, %s y %s", parametro1, parametro2, parametro3);
			 sleep(3);
			 int p = datosFCB(pathArchivo);
			 if(p == -1) {	// No deberia entrar nunca
				log_error(logger, "Error inesperado antes de cargar FCB");
				cop=F_TRUNCATE_FAIL;
				send(cliente_socket, &cop, sizeof(op_code), 0);
				break;
			 }

			 int nuevoTamanioArchivo = atoi(parametro2);
			 // 1 - determinar cantidad de bloques que necesita el archivo con el nuevo tamaño
			 // 2 TAMAÑO_NUEVO - TAMAÑO ACTUAL,
			 //   SI la diferencia es < 0 ==> resto bloques
			 //   SI la diferencia es > 0 ==> SUMO bloques
			 //   SI es cero no hago nada
			 // 3 - Determinar cuantos bloques libres hay en el bitmap
			 int i;
			 int bloques_libres = 0;
			 long int tamano_bitmap = bitarray_get_max_bit(s_bitmap);	// tambien se puede saber con la cantidad de bloques / 8
			 //log_info(logger, "El tamanio del bitmap en bits es: %d", tamano_bitmap);


			 for(i = 0; i <  tamano_bitmap ; i++ )
			 {
				 log_warning(logger, "recorriendo bitarray " );
				 if( !bitarray_test_bit(s_bitmap,  i)){
					 log_warning(logger, "en el fi bitarray " );
					 bloques_libres++;
				 }

			 }
			 log_warning(logger, "bloques libres: %d" , bloques_libres );

			 if(nuevoTamanioArchivo == 0){
				 //TODO madarlo a exit o ver que se hace liberar todos los bloques si tenia
				 log_warning(logger, "Tamaño de archivo 0");
				 break;
			 }
			 int bloques_necesarios =  ceil(nuevoTamanioArchivo / configuracionSuperBloque->BLOCK_SIZE);

			 if(bloques_necesarios > 1 )
				 bloques_necesarios++;

			 // Verifico si hay bloques suficientes
			 if(bloques_necesarios > bloques_libres){
				 //TODO madarlo a exit o ver que se hace liberar todos los bloques si tenia
				 // Hacer sen dal kernel de error
				 log_warning(logger, "Tamaño de archivo 0");
				 break;
			 }


			 if(nuevoTamanioArchivo < FCB_archivo->tamanio_archivo){
				 // Liberamos bloques si fuera necesario
			 }else{
				 // Asignar nuevos bloques
				 if(FCB_archivo->tamanio_archivo == 0){
					bitarray_set_bit(s_bitmap, 0);
					bitarray_set_bit(s_bitmap, 1);
					int indice_bitmap    = buscarPrimerBloqueVacio (s_bitmap->bitarray, configuracionSuperBloque->BLOCK_SIZE);
					int puntero_directo  = indice_bitmap * configuracionSuperBloque->BLOCK_SIZE;
					//asumiendo q cambio el bit de 0 a 1 o 1 a 0

					if(indice_bitmap == -1){
						log_error(logger, "Error con indice de bitmap");
					}

					log_info(logger, "Primer bloque vacio: %d", indice_bitmap);
					bitarray_set_bit(s_bitmap, indice_bitmap);
					FCB_archivo->puntero_directo = puntero_directo;
					log_info(logger, "Puntero directo asignado: %d", puntero_directo);
				 }
			 }


			 log_warning(logger, "Cantidad bloq q necesita el archivo: %d" ,  bloques_necesarios );
			 log_warning(logger, "Tamaño nuevo: %d" ,  nuevoTamanioArchivo );


			 //Guarda los datos de la variable struct en FCB del archivo
			 config_set_value(FCB, "TAMANIO_ARCHIVO", string_itoa(nuevoTamanioArchivo));
			 config_set_value(FCB, "PUNTERO_DIRECTO", string_from_format("%.0lf", FCB_archivo->puntero_directo) );
			 config_save_in_file(FCB, pathArchivo);

			 cop = F_TRUNCATE_OK;
			 send(cliente_socket, &cop, sizeof(op_code), 0);
			 break;

		 case F_READ:
			 //recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_READ con parametros %s, %s y %s", parametro1, parametro2, parametro3);
			 break;

		 case F_WRITE:
			 //recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_WRITE con parametros %s, %s y %s", parametro1, parametro2, parametro3);
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


//CLIENTE
//MEMORIA
int generar_conexion(int* memoria_fd, t_config_file_system* configuracion) {
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

int recorrerFCBs(char* PATH, char* nombre_Archivo){
	DIR *directorioFCB = opendir(PATH);
log_warning(logger, "RUta: %s - Nombre: %s", PATH, nombre_Archivo);
	if(directorioFCB == NULL){
		log_info(logger, "Directorio Erroneo");
		return -2;
	}
	struct dirent *lectura;
	log_warning(logger, "EXISTEEEEEE!!!!!!???? %s - %s", lectura->d_name , nombre_Archivo);
	while ((lectura = readdir(directorioFCB)) != NULL){
		log_error(logger, "EXISTEEEEEE!!!!!!???? %s - %s", lectura->d_name , nombre_Archivo);
		if(lectura->d_name == nombre_Archivo){
			log_warning(logger, "EXISTEEEEEE!!!!!!???? %s - %s", lectura->d_name , nombre_Archivo);
			return 0;
		}
	}

	return -1;
}

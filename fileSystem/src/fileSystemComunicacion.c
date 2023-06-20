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

			 long int tamano_bitmap = bitarray_get_max_bit(s_bitmap);

			 // Contador de libres
			 int i;
			 int bloques_libres = 0;
			 for(i = 0; i <  tamano_bitmap ; i++ )
			 {
				 if(!bitarray_test_bit(s_bitmap,  i)) bloques_libres++;
			 }
			 log_warning(logger, "Bloques libres segun bitmap: %d de %d (ocupados=%d)" , bloques_libres, tamano_bitmap, tamano_bitmap-bloques_libres);

			 if(nuevoTamanioArchivo == 0){
				 //TODO liberar todos los bloques en el bitmap
				 log_warning(logger, "Trunc a 0");
				 break;
			 }
			 // Redondeamos para arriba
			 int bloques_totales =  ceil(nuevoTamanioArchivo / configuracionSuperBloque->BLOCK_SIZE);
			 int bloques_actuales = FCB_archivo->tamanio_archivo / configuracionSuperBloque->BLOCK_SIZE;	// Deberia ser siempre un numero redondo
			 int diferenciaBloques = bloques_totales - bloques_actuales;
			 log_warning(logger, "El archivo ocupa: %d bloques - Debe pasar a ocupar: %d bloques" , bloques_actuales, bloques_totales);

			 // Verifico si hay bloques suficientes
			 if(diferenciaBloques > bloques_libres){
				 //TODO send F_CREATE_FAIL
				 log_warning(logger, "Cantidad de bloques insuficiente");
				 break;
			 }
			 int indice_bitmap = 0;
			 // Trunc a menos size -> Liberamos bloques si fuera necesario
			 if(diferenciaBloques < 0){
				 // TODO liberar bloques. Si se queda con un solo bloque hay que liberar el bloque de puntero indirecto
			 }
			 // Trunc a mas size --> Asignamos nuevos bloques si fuera necesario
			 if(diferenciaBloques > 0){
				 // Si su size es 0 y lo agrando -> Siempre asigno un puntero directo, independientemente del size
				 if(FCB_archivo->tamanio_archivo == 0){
					indice_bitmap = buscarPrimerBloqueVacio (s_bitmap, configuracionSuperBloque->BLOCK_SIZE);
					int puntero_directo = indice_bitmap * configuracionSuperBloque->BLOCK_SIZE;

					if(indice_bitmap == -1){
						log_error(logger, "Error con indice de bitmap");
						break;	// No deberia entrar nunca
					}

					log_info(logger, "Se encontro un bloque en la posicion nro: %d", indice_bitmap);
					// Lo marco ocupado en el bitmap
					bitarray_set_bit(s_bitmap, indice_bitmap);
					// Guardo el puntero directo
					FCB_archivo->puntero_directo = puntero_directo;
					log_info(logger, "Puntero directo asignado: %d", puntero_directo);
					bloques_actuales++;
				 }
				 // Si se necesita mas de uno y no tiene bloque en puntero indirecto -> Asigno puntero indirecto
				 if(bloques_actuales == 1 && bloques_actuales < bloques_totales) {
					 indice_bitmap = buscarPrimerBloqueVacio (s_bitmap, configuracionSuperBloque->BLOCK_SIZE);
					 int puntero_indirecto = indice_bitmap * configuracionSuperBloque->BLOCK_SIZE;
					 if(indice_bitmap == -1){
						log_error(logger, "Error con indice de bitmap");
						break;	// No deberia entrar nunca
					 }
					 log_info(logger, "Se encontro un bloque en la posicion nro: %d", indice_bitmap);
					// Lo marco ocupado en el bitmap
					bitarray_set_bit(s_bitmap, indice_bitmap);
					// Guardo el puntero indirecto
					FCB_archivo->puntero_indirecto = puntero_indirecto;
					log_info(logger, "Puntero indirecto asignado: %d", puntero_indirecto);
				 }

				 // Solo entra si ya hay puntero directo y puntero indirecto
				 int i;
				 int puntero_nuevo;
				 for(i = bloques_actuales; i < bloques_totales; i++) {
					 indice_bitmap = buscarPrimerBloqueVacio (s_bitmap, configuracionSuperBloque->BLOCK_SIZE);
					 puntero_nuevo = indice_bitmap * configuracionSuperBloque->BLOCK_SIZE;
					 bitarray_set_bit(s_bitmap, indice_bitmap);
					 // TODO: escribir puntero_nuevo en el bloque de puntero_indirecto. Tener en cuenta demora en lectura y escritura
				 }


			 }
			 log_warning(logger, "Tamaño nuevo: %d" ,  nuevoTamanioArchivo );


			 //Guarda los datos de la variable struct en FCB del archivo
			 config_set_value(FCB, "TAMANIO_ARCHIVO", string_itoa(nuevoTamanioArchivo));
			 //config_set_value(FCB, "PUNTERO_DIRECTO", string_from_format("%.0lf", FCB_archivo->puntero_directo) );
			 config_set_value(FCB, "PUNTERO_DIRECTO", string_itoa(FCB_archivo->puntero_directo));
			 config_set_value(FCB, "PUNTERO_INDIRECTO", string_itoa(FCB_archivo->puntero_indirecto));
			 config_save_in_file(FCB, pathArchivo);

			 cop = F_TRUNCATE_OK;
			 send(cliente_socket, &cop, sizeof(op_code), 0);
			 break;

		 case F_READ:
			 //recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_READ con parametros %s, %s y %s", parametro1, parametro2, parametro3);
			 sleep(3); // borrar
			 cop = F_READ_OK;
			 send(cliente_socket, &cop, sizeof(op_code), 0);
			 break;

		 case F_WRITE:
			 //recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_WRITE con parametros %s, %s y %s", parametro1, parametro2, parametro3);
			 sleep(3);	// borrar
			 cop = F_WRITE_OK;
			 send(cliente_socket, &cop, sizeof(op_code), 0);
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

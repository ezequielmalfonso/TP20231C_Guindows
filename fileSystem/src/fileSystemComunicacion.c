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
int posicion;

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

	 char parametro1[20], parametro2[20], parametro3[20], pos[20];
	 char* pathArchivo;
	 char* nombre;

	 recv_instruccion(cliente_socket, parametro1, parametro2, parametro3, pos);	// la posicion es para fwrite y fread
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

		 case F_CLOSE:	// No llega, se ocupa kernel
			 //recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_CLOSE con parametros %s, %s y %s", parametro1, parametro2, parametro3);
			 break;

		 case F_SEEK:	// No llega, se ocupa kernel
			 //recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_SEEK con parametros %s, %s y %s", parametro1, parametro2, parametro3);
			 break;

		 case F_TRUNCATE:
			 //recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_TRUNCATE con parametros archivo: %s, nuevo tamaño: %s", parametro1, parametro2);
			 //sleep(5);
			 int p = datosFCB(pathArchivo);
			 if(p == -1) {	// No deberia entrar nunca
				log_error(logger, "Error inesperado antes de cargar FCB");
				cop=F_TRUNCATE_FAIL;
				send(cliente_socket, &cop, sizeof(op_code), 0);
				break;
			 }

			 int nuevoTamanioArchivo = atoi(parametro2);
			 long int tamano_bitmap = bitarray_get_max_bit(s_bitmap);
			 // Contador de libres
			 int i;
			 int bloques_libres = 0;
			 for(i = 0; i <  tamano_bitmap ; i++ )
			 {
				 if(!bitarray_test_bit(s_bitmap,  i)) bloques_libres++;
			 }
			 log_warning(logger, "Bloques libres segun bitmap: %d de %d (ocupados=%d)" , bloques_libres, tamano_bitmap, tamano_bitmap-bloques_libres);

			 // Redondeamos para arriba
			 int bloques_totales =  ceil(nuevoTamanioArchivo / configuracionSuperBloque->BLOCK_SIZE);
			 int bloques_actuales = FCB_archivo->tamanio_archivo / configuracionSuperBloque->BLOCK_SIZE;	// Deberia ser siempre un numero redondo
			 int diferenciaBloques = bloques_totales - bloques_actuales;
			 log_warning(logger, "El archivo ocupa: %d bloques - Debe pasar a ocupar: %d bloques" , bloques_actuales, bloques_totales);

			 // Trunc a 0
			 if(nuevoTamanioArchivo == 0){
				 log_warning(logger, "F_TRUNC a tamaño 0");

				 if(FCB_archivo->tamanio_archivo == 0) {
					 cop = F_TRUNCATE_OK;
					 send(cliente_socket, &cop, sizeof(op_code), 0);
					 break;
				 }

				 int bloque_liberar = FCB_archivo->puntero_directo / configuracionSuperBloque->BLOCK_SIZE;
				 bitarray_clean_bit(s_bitmap,bloque_liberar);
				 log_error(logger, "Elimando bloque de puntero directo (nro: %d)", bloque_liberar);
				 int i;
				 int nro_bloque;
				 int direccion_donde_leer;

				 if(bloques_actuales > 1){
					 cargarBloqueIndirecto(descriptor_archivo_bloque, FCB_archivo->puntero_indirecto);
					 for(i = 0; i < (bloques_actuales - 1) ; i++ ){
						 direccion_donde_leer = FCB_archivo->puntero_indirecto + (i * tamanio_puntero);
						 nro_bloque = leerBloqueIndirecto(descriptor_archivo_bloque, direccion_donde_leer) / configuracionSuperBloque->BLOCK_SIZE;
						 bitarray_clean_bit(s_bitmap, nro_bloque);
					 }
					 guardarBloqueIndirecto(descriptor_archivo_bloque, FCB_archivo->puntero_indirecto);
					 int bloque_liberar_indirecto = FCB_archivo->puntero_indirecto / configuracionSuperBloque->BLOCK_SIZE;
					 bitarray_clean_bit(s_bitmap,bloque_liberar_indirecto);

				 }

				 log_error(logger, "Terminando trunc a 0");
				 FCB_archivo->tamanio_archivo = 0;
				 FCB_archivo->puntero_directo = 0;
				 FCB_archivo->puntero_indirecto = 0;
			 }


			 // Verifico si hay bloques suficientes
			 if(diferenciaBloques > bloques_libres){
				 log_warning(logger, "Cantidad de bloques insuficiente");
				 cop = F_CREATE_FAIL;
				 send(cliente_socket, &cop, sizeof(op_code), 0);
				 break;
			 }
			 int indice_bitmap = 0;
			 // Trunc a menos size -> Liberamos bloques si fuera necesario
			 if(diferenciaBloques < 0 && nuevoTamanioArchivo != 0){
				 // TODO liberar bloques. Si se queda con un solo bloque hay que liberar el bloque de puntero indirecto
				 int i;
				 int nro_bloque;
				 int direccion_donde_leer;

				 if(bloques_actuales > 1){
					 cargarBloqueIndirecto(descriptor_archivo_bloque, FCB_archivo->puntero_indirecto);
					 for(i = bloques_actuales ; i > bloques_totales ; i--){
						 direccion_donde_leer = i*tamanio_puntero;
						 //direccion_donde_leer = FCB_archivo->puntero_indirecto + (i * tamanio_puntero);
						 nro_bloque = leerBloqueIndirecto(descriptor_archivo_bloque, direccion_donde_leer) / configuracionSuperBloque->BLOCK_SIZE;
						 bitarray_clean_bit(s_bitmap,nro_bloque);
						 log_error(logger, "Se ha borrado el bloque nro: %d", nro_bloque);
					 }
					 if(bloques_totales == 1 ){
					   int bloque_liberar_indirecto = FCB_archivo->puntero_indirecto / configuracionSuperBloque->BLOCK_SIZE;
					   bitarray_clean_bit(s_bitmap,bloque_liberar_indirecto);
					   FCB_archivo->puntero_indirecto = 0;
					   log_error(logger, "Puntero indirecto ELIMINADO (nro: %d)", bloque_liberar_indirecto);
					 }

				 }

				 log_error(logger, "Terminando trunc a menor tamaño: %d", nuevoTamanioArchivo);
				 FCB_archivo->tamanio_archivo = nuevoTamanioArchivo;

			 }
			 // Trunc a mas size --> Asignamos nuevos bloques si fuera necesario
			 if(diferenciaBloques > 0){
				 // TODO cantidad de punteros mayor a la cantidad de punteros que entran en el puntero indirecto
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
					usleep(configuracion->RETARDO_ACCESO_BLOQUE);
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
				 uint32_t puntero_nuevo;
				 int direccion_donde_escribir;
				 if(bloques_totales > bloques_actuales) {
					 cargarBloqueIndirecto(descriptor_archivo_bloque, FCB_archivo->puntero_indirecto);
					 for(i = bloques_actuales; i < bloques_totales; i++) {
						 indice_bitmap = buscarPrimerBloqueVacio (s_bitmap, configuracionSuperBloque->BLOCK_SIZE);
						 puntero_nuevo = indice_bitmap * configuracionSuperBloque->BLOCK_SIZE;
						 bitarray_set_bit(s_bitmap, indice_bitmap);
						 // TODO: escribir puntero_nuevo en el bloque de puntero_indirecto. Tener en cuenta demora en lectura y escritura
						 log_info(logger, "Puntero guardado en direccion: %d", FCB_archivo->puntero_indirecto + ((i-1) * tamanio_puntero));
						 //direccion_donde_escribir = FCB_archivo->puntero_indirecto + ((i-1) * tamanio_puntero);
						 direccion_donde_escribir = (i-1) * tamanio_puntero;
						 escribirBloqueIndirecto( descriptor_archivo_bloque, direccion_donde_escribir, puntero_nuevo);
					 }
					 guardarBloqueIndirecto(descriptor_archivo_bloque, FCB_archivo->puntero_indirecto);
				 }
			 }
			 log_warning(logger, "Tamaño nuevo: %d" ,  nuevoTamanioArchivo );

			 //Guarda los datos de la variable struct en FCB del archivo
			 config_set_value(FCB, "TAMANIO_ARCHIVO", string_itoa(nuevoTamanioArchivo));
			 config_set_value(FCB, "PUNTERO_DIRECTO", string_itoa(FCB_archivo->puntero_directo));
			 config_set_value(FCB, "PUNTERO_INDIRECTO", string_itoa(FCB_archivo->puntero_indirecto));
			 config_save_in_file(FCB, pathArchivo);
			 config_destroy(FCB);

			 cop = F_TRUNCATE_OK;
			 send(cliente_socket, &cop, sizeof(op_code), 0);
			 break;

		 case F_READ:
			 //recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_READ con parametros Archivo: %s, Tamaño: %s, Direccion Fisica: %s y Posicion: %s", parametro1, strtok(parametro3, "\n"), parametro2, pos);
			 int tamanioTotalALeer = atoi(parametro3);
			 sleep(2);
			 posicion = atoi(pos);
			 if (tamanioTotalALeer == 0) {
				 cop=F_READ_FAIL;
				 send(cliente_socket, &cop, sizeof(op_code), 0);
				 break;
			 }
			 {int p = datosFCB(pathArchivo);
			 if(p == -1) {	// No deberia entrar nunca
				log_error(logger, "Error inesperado antes de cargar FCB");
				cop=F_READ_FAIL;
				send(cliente_socket, &cop, sizeof(op_code), 0);
				break;
			 }}
			 if(posicion+tamanioTotalALeer > FCB_archivo->tamanio_archivo) {
				 log_error(logger, "Intento de lectura por fuera del tamaño del archivo: %s", FCB_archivo->nombre_archivo);
				 cop=F_READ_FAIL;
				 send(cliente_socket, &cop, sizeof(op_code), 0);
				 break;
			 }

			 //sleep(3); // borrar

			 int indice_nro_bloque = 0;
			 int nro_bloque_archivo = floor((double)posicion/configuracionSuperBloque->BLOCK_SIZE);	// Bloque donde empieza la lectura
			 uint32_t puntero_primer_bloque;
			 uint32_t direccion_indirecto = /*FCB_archivo->puntero_indirecto +*/ tamanio_puntero * (nro_bloque_archivo-1);

			 // Lo carga siempre, se podria ver de que solo lo cargue si lo necesita
			 cargarBloqueIndirecto(descriptor_archivo_bloque, FCB_archivo->puntero_indirecto);

			 if(nro_bloque_archivo == 0) {
				 puntero_primer_bloque = FCB_archivo->puntero_directo;
				 log_info(logger, "Se debe empezar por el puntero directo");
			 }
			 else {
				 log_info(logger, "Se debe empezar por el puntero indirecto");
				 puntero_primer_bloque = leerBloqueIndirecto(descriptor_archivo_bloque, direccion_indirecto);
				 indice_nro_bloque++;
				 direccion_indirecto += tamanio_puntero;

			 }
			 log_info(logger, "La lectura comienza en el bloque de puntero %d, perteneciente al bloque nro: %d", puntero_primer_bloque, nro_bloque_archivo);
			 int cantidad_bloques_a_leer = ceil((double)(tamanioTotalALeer) / configuracionSuperBloque->BLOCK_SIZE);
			 log_info(logger,"Tamaño a leer: %d, Tamaño Bloque: %u", tamanioTotalALeer,configuracionSuperBloque->BLOCK_SIZE);
			 log_info(logger, "Se deben leer %d bloques", cantidad_bloques_a_leer);

			 //sleep(1);//borrar

			 int posEnPrimerBloque = posicion % configuracionSuperBloque->BLOCK_SIZE;
			 int tamanioALeerPrimerBloque = configuracionSuperBloque->BLOCK_SIZE - posEnPrimerBloque;
			 tamanioALeerPrimerBloque = min(tamanioALeerPrimerBloque, tamanioTotalALeer);
			 char* leido = string_new();
			 char* buffer = malloc(configuracionSuperBloque->BLOCK_SIZE);
			 lseek(descriptor_archivo_bloque, posEnPrimerBloque + puntero_primer_bloque, SEEK_SET);
			 read(descriptor_archivo_bloque, buffer, tamanioALeerPrimerBloque);
			 usleep(configuracion->RETARDO_ACCESO_BLOQUE);
			 tamanioTotalALeer -= tamanioALeerPrimerBloque;
			 string_append(&leido, buffer);
			 free(buffer);
			 log_info(logger, "Se leyo a partir del offset %d dentro del bloque. Informacion del primer bloque leido: %s", posEnPrimerBloque, leido);
			 log_info(logger, "Tamaño leido en primer bloque: %d, tamaño restante por leer: %d", tamanioALeerPrimerBloque, tamanioTotalALeer);
			 // Ya tengo en buffer la informacion del primer bloque
			 int j;
			 int tamanioALeer;
			 int pointer_a_leer;
			 // Si entra ya leyo un bloque (solo debe recorrer los indirectos restantes)
			 for(j = 1; j <= tamanioTotalALeer; j += configuracionSuperBloque->BLOCK_SIZE) {
				 tamanioALeer = max(tamanioTotalALeer, configuracionSuperBloque->BLOCK_SIZE);
				 pointer_a_leer = leerBloqueIndirecto(descriptor_archivo_bloque, j*tamanio_puntero);
				 lseek(descriptor_archivo_bloque, pointer_a_leer, SEEK_SET);
				 char* buffer = malloc(tamanioALeer);
				 usleep(configuracion->RETARDO_ACCESO_BLOQUE);
				 read(descriptor_archivo_bloque, buffer, tamanioALeer);
				 string_append(&leido, buffer);
				 log_info(logger, "Se leyo en un bloque %s", buffer);
				 free(buffer);
			 }
			 log_info(logger, "Lectura completa: %s, listo para enviar a memoria - tamanio: %d", leido, atoi(parametro3));


			 send_fs_memoria_read(memoria_fd,parametro2,atoi(parametro3),leido, MOV_IN);

			 recv(memoria_fd, &cop, sizeof(op_code), 0);

			 if(cop == MOV_IN_OK){
				 cop = F_READ_OK;
			 }else{
				 cop = F_READ_FAIL;
			 }
			 config_destroy(FCB);
			 send(cliente_socket, &cop, sizeof(op_code), 0);
			 break;

		 case F_WRITE:
			 //recv_instruccion(cliente_socket, parametro1, parametro2, parametro3);
			 log_info(logger, "Se recibio F_WRITE con parametros Archivo: %s, Tamaño: %s, Direccion Fisica: %s y Posicion: %s", parametro1, strtok(parametro3, "\n"), parametro2, pos);
			 {int p = datosFCB(pathArchivo);
			 if(p == -1) {	// No deberia entrar nunca
				log_error(logger, "Error inesperado antes de cargar FCB");
				cop=F_WRITE_FAIL;
				send(cliente_socket, &cop, sizeof(op_code), 0);
				break;
			 }}
			 sleep(3);	// borrar
			 posicion = atoi(pos);
			 int tamanio_a_escribir  = atoi(parametro3);
			 if(tamanio_a_escribir + posicion > FCB_archivo->tamanio_archivo) {	// Debe entrar en el archivo partiendo de la posicion
				 log_error(logger, "El tamaño que se desea escribir (%d) no entra en el archivo: %s", tamanio_a_escribir, parametro1);
				 cop = F_WRITE_FAIL;
				 send(cliente_socket, &cop, sizeof(cop), 0);
			 }

			 void* escribirBuffer = malloc(tamanio_a_escribir);

			 //TODO
			 //send pedido a memoria
			 log_warning(logger, "Param: %s - Tam: %d", parametro2, tamanio_a_escribir );
			 send_fs_memoria(memoria_fd,parametro2,tamanio_a_escribir, MOV_OUT);

			 recv(memoria_fd, escribirBuffer, tamanio_a_escribir, MSG_WAITALL);
			 log_warning(logger, "Recibiendo buffer que escribir: %s", escribirBuffer);

			 //recv datos de memoria
			 // if memo fail -> send kernel F_WRITE_FAIL
			 //void* escribirBuffer = "holachauaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";	// borrar

			 int bloques_necesarios = ceil((double)tamanio_a_escribir / configuracionSuperBloque->BLOCK_SIZE);
			 int bloque_inicial = floor((double)posicion / configuracionSuperBloque->BLOCK_SIZE);
			 int offset_inicial = (uint32_t)posicion % configuracionSuperBloque->BLOCK_SIZE;
			 log_info(logger, "Bloque inicial: %d, Offset en este bloque: %d, bloques necesarios totales: %d", bloque_inicial, offset_inicial, bloques_necesarios);
			 int puntero;
			 int nro_indirecto = 0;
			 int size = min(tamanio_a_escribir, configuracionSuperBloque->BLOCK_SIZE - offset_inicial);

			 // Primer bloque: debo tener en cuenta el offset inicial
			 if(bloque_inicial == 0) {	//Primer bloque es el directo
				 puntero = FCB_archivo->puntero_directo;
			 }
			 else {	// Primer bloque es el indirecto
				 puntero = leerBloqueIndirecto(descriptor_archivo_bloque, (bloque_inicial-1) * tamanio_puntero);
				 nro_indirecto = bloque_inicial;
				 bloques_necesarios++;
			 }
			 puntero += offset_inicial;
			 usleep(configuracion->RETARDO_ACCESO_BLOQUE);
			 lseek(descriptor_archivo_bloque, puntero, SEEK_SET);
			 write(descriptor_archivo_bloque, escribirBuffer, size);
			 tamanio_a_escribir -= size;
			 escribirBuffer += size;
			 log_info(logger, "Se ha escrito en el bloque con puntero %d un tamaño de %d, partiendo de la posicion del archivo %d", puntero, size, posicion);
			 bloques_necesarios--;

			 // Demas bloques
			 if(nro_indirecto < bloques_necesarios) {	// solo para que no cargue el bloque indirecto si no lo necesita
				 cargarBloqueIndirecto(descriptor_archivo_bloque, FCB_archivo->puntero_indirecto);
				 int l;
				 for(l = nro_indirecto; l < bloques_necesarios; l++) {
					 puntero = leerBloqueIndirecto(descriptor_archivo_bloque,l*tamanio_puntero);
					 size = min(tamanio_a_escribir, configuracionSuperBloque->BLOCK_SIZE);
					 usleep(configuracion->RETARDO_ACCESO_BLOQUE);
					 lseek(descriptor_archivo_bloque, puntero, SEEK_SET);
					 write(descriptor_archivo_bloque, escribirBuffer, size);
					 tamanio_a_escribir -= size;
					 escribirBuffer += size;
					 log_info(logger, "Se ha escrito en el bloque con puntero %d", puntero);
				 }
			 }
			 //free(escribirBuffer);
			 cop = F_WRITE_OK;
			 config_destroy(FCB);
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

int max(int a, int b) {
	if(a > b) {
		return a;
	}
	else return b;
}
int min(int a, int b) {
	if(a < b) {
		return a;
	}
	else return b;
}


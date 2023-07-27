/*
 ============================================================================
 Name        : fileSystem.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "fileSystem.h"

typedef struct {
	char* NOMBRE_ARCHIVO;
	uint32_t TAMANIO_ARCHIVO;
	uint32_t PUNTERO_DIRECTO;
	uint32_t PUNTERO_INDIRECTO;
} t_fcb;

struct t_fcb *lista;


int fileSystemServer;
int memoria_fd;
char* bloqueIndirectoBuffer;
t_super_bloque* configuracionSuperBloque;
//BITMAP
int bitmap;
size_t bitmapSize;
t_bitarray* s_bitmap;
//void* fileData;
// FCB

int descriptor_archivo_bloque;
int tamanio_puntero;

int main(void) {
	// Configuracion gral
	cargarConfiguracion();
	//log_warning(logger, "PATH: %s",configuracion->PATH_SUPERBLOQUE );
	cargarSuperBloque(configuracion->PATH_SUPERBLOQUE);
	descriptor_archivo_bloque = cargarArchivoBloques(configuracion->PATH_BLOQUES, configuracionSuperBloque->BLOCK_SIZE, configuracionSuperBloque->BLOCK_COUNT);
	bloqueIndirectoBuffer = malloc(configuracionSuperBloque->BLOCK_SIZE);
	t_config* config_ips = config_create("../ips.conf");
	char* ip = config_get_string_value(config_ips,"IP_FILESYSTEM");

	//CLIENTE Conexion a MEMORIA
	generar_conexion(&memoria_fd, configuracion);
	// ENVIO y RECEPCION A MEMORIA
	op_code op=FS;
	send(memoria_fd,&op,sizeof(op_code),0);

	// Aca deberia ir cargarBitmap de bloque

	iniciarBitmap(configuracion->PATH_BITMAP, configuracionSuperBloque->BLOCK_COUNT);

	// RECORRER DIRECTORIO DE FCB's

	// Hay que usar mmap, msync, y munmap en ese orden

	// INICIO CPU SERVIDOR
	char* puertoFileSystem = string_itoa(configuracion->PUERTO_ESCUCHA);
	int FileSystemServer= iniciar_servidor(logger,"FileSystem server",ip,puertoFileSystem);//ACA IP PROPIA
	while (server_escuchar("FILESYSTEM_SV", FileSystemServer));

	limpiarConfiguracion();
	return 0;

}
// Cargar archivo superbloque
int configValidaSuperBloque(t_config* superBloque){
	return (config_has_property(superBloque,"BLOCK_SIZE") && config_has_property(superBloque,"BLOCK_COUNT"));
}
int cargarSuperBloque(char *path){
	log_warning(logger, "PATH: %s",configuracion->PATH_SUPERBLOQUE );
	t_config* superBloque;
	configuracionSuperBloque = malloc(sizeof(t_super_bloque));
	//logger = log_create("superBloque.log", "SuperBLoque", 1, LOG_LEVEL_INFO);

		superBloque = config_create(path);
		if (superBloque == NULL) {
			superBloque = config_create(path);
		}

		if (superBloque == NULL || !configValidaSuperBloque(superBloque)) {
			log_error(logger,"Archivo de SuperBloque inválido.");
			return -1;
		}
		log_error(logger, "PATH: %s",configuracion->PATH_SUPERBLOQUE );
		configuracionSuperBloque->BLOCK_SIZE = config_get_int_value(superBloque, "BLOCK_SIZE");
		configuracionSuperBloque->BLOCK_COUNT = config_get_int_value(superBloque, "BLOCK_COUNT");

		//tamanio_puntero = ceil(log2(configuracionSuperBloque->BLOCK_COUNT * configuracionSuperBloque->BLOCK_SIZE)); binario
		//tamanio_puntero = ceil(log10(configuracionSuperBloque->BLOCK_COUNT * configuracionSuperBloque->BLOCK_SIZE)); decimal
		// Nota: los punteros se deben almacenas como uint32
		//log_warning(logger, "Tamanio puntero: %d", tamanio_puntero);
		tamanio_puntero = 10;

		log_info(logger,
			"\nBLOCK_SIZE: %d\n"
			"BLOCK_COUNT: %d",
			configuracionSuperBloque->BLOCK_SIZE,
			configuracionSuperBloque->BLOCK_COUNT);

	return 0;
}
// El archivo de bloques no es una config no lo borre pero lo dejo comentado

//int cargarArchivoBloques(char *path){
	//log_warning(logger, "Bloque");
	//t_config* bloque;
	//logger = log_create("superBloque.log", "SuperBLoque", 1, LOG_LEVEL_INFO);

		//bloque = config_create(path);
		//if (bloque == NULL) {
			//bloque = config_create(path);
		//}

		//configuracionSuperBloque->BLOCK_SIZE = config_get_int_value(bloque, "BLOCK_SIZE");
		//configuracionSuperBloque->BLOCK_COUNT = config_get_int_value(bloque, "BLOCK_COUNT");
		/*
		log_info(logger,
			"\nBLOCK_SIZE: %d\n"
			"BLOCK_COUNT: %d",
			configuracionSuperBloque->BLOCK_SIZE,
			configuracionSuperBloque->BLOCK_COUNT);
		*/
	//return 0;
//}





int iniciarBitmap (char* path ,uint32_t block_count ){

	bitmapSize = block_count / 8; //Calculo el tamanio
	//TODO creo que solo va el de leer, pq nos dicen qu e lo podemos tener creado de antemano
	//bitmap = open(path, O_RDWR | O_CREAT, 0777); // Paso la ruta absoluta
	bitmap = open(path, O_RDWR , 0777); // Paso la ruta absoluta

	if(bitmap == -1){
		log_info(logger, "No existe el BITMAP"); // Si falla FOPEN muestro error
		bitmap = open(path, O_RDWR | O_CREAT , 0777);
		// Aca tenemos 2 opciones, truncar el tamanio del archivo a fileSize o darle un tamanio mucho mayor
		ftruncate(bitmap,bitmapSize); // Podriamos verificar si se trunco con un if
	}
	//ftruncate(bitmap,bitmapSize); // Podriamos verificar si se trunco con un if
	// void * mmap (void *address, size_t length, int protect, int flags, int filedes, off_t offset)

	char* fileData = mmap(NULL,bitmapSize,PROT_READ | PROT_WRITE,MAP_SHARED, bitmap,0);

	if (fileData == MAP_FAILED) {
	     log_info(logger, "Error al mapear el archivo");
	     close(bitmap);
	     return 1;
	}
	s_bitmap = bitarray_create_with_mode(fileData, bitmapSize, LSB_FIRST);
	//memset(fileData, 0, bitmapSize); // TODO: ESTO SE DEBERIA IR DSP DE CORRER


	//Sincronizo los datos en memoria con el archivo bitmap.dat
	//msync(fileData, bitmapSize, MS_SYNC); //Podriamos verificar con un if si se sincronizo
	//munmap(fileData, bitmapSize);
	//close(bitmap);

	return 0;
}

// FUNCIONES PARA TRABAJAR CON BLOQUES PARA NO USAR MMAP CON ALGEBRA DE PUNTEROS XD


// Aca falta todavia crear estructura de tipo bloque, y dsp falta relacionar todo, BITMAP, ARCHIVO DE BLOQUES Y FCBS

int cargarArchivoBloques(char *path, int BLOCK_SIZE, int BLOCK_COUNT){
	int fd_ArchivoBloques;

    fd_ArchivoBloques = open(path, O_RDWR, 0777);

	if (fd_ArchivoBloques == -1){
		fd_ArchivoBloques = open(path, O_CREAT | O_RDWR, 0777);
		ftruncate(fd_ArchivoBloques, BLOCK_SIZE * BLOCK_COUNT);
	}
	else {
		log_info(logger,"Archivo de Bloques Cargado");
	}
	return fd_ArchivoBloques;
}

// ESta funcion podria ser para todos los archivos, bitmap, etc, pero hay que sincronizar con disco algunos.
int cerrarArchivoBloques(int fd){
	return close(fd);
}

void escribirBloqueIndirecto(int descriptor, int offset, uint32_t direccion){

	//lseek(descriptor, offset, SEEK_SET);
	//Esto es si el tamanio de direccion debe variar
	char* dir = string_from_format("%d", direccion);
	char* dir_reverse = string_reverse(dir);
	int i;
	int longitud_inicial = string_length(dir);

	for(i = longitud_inicial ; i < tamanio_puntero ; i++ ){
		string_append(&dir_reverse, "0");
	}
	char* dir_final = string_reverse(dir_reverse);
	log_error(logger, "Dir a escribir : %s ", dir);
	//write(descriptor, dir_final, tamanio_puntero);
	memcpy(bloqueIndirectoBuffer+offset, dir_final, tamanio_puntero);
	//write(descriptor, &direccion, sizeof(uint32_t));

}
uint32_t leerBloqueIndirecto(int descriptor, int offset){

	log_warning(logger, "Buscando puntero en offset: %d", offset);

	//lseek(descriptor, offset, SEEK_SET);
	//char* buffer = malloc(1+sizeof(char)*tamanio_puntero);
	char* buffer = malloc(tamanio_puntero+1);	// +1 xq si
	memcpy(buffer, bloqueIndirectoBuffer+offset, tamanio_puntero);
	buffer[tamanio_puntero] = '\0';
	//read(descriptor, buffer, tamanio_puntero);
//	buffer[sizeof(char)*tamanio_puntero] = '\0';
	//read(descriptor, buffer, tamanio_puntero);

	//log_warning(logger, "Buffer : %s", buffer);

	//int dir = atoi(buffer);
	char* ptr ;//= malloc(tamanio_puntero+1);	// No se usa
	uint32_t direccion = strtoul(buffer, &ptr, 10);	// unsigned long ocupa lo mismo que uint_32
	//printf(logger, "Direccion de lectura &d", direccion);
	//int direccion = floor(dir / configuracionSuperBloque->BLOCK_SIZE);
	//free(ptr);
	free(buffer);
	log_warning(logger, "Se obtuvo la direccion: %d", direccion);
	return direccion;
}


void cargarBloqueIndirecto(int descriptor, int offset) {
	log_info(logger, "Cargando bloque indirecto con direccion %d", offset);
	lseek(descriptor, offset, SEEK_SET);
	read(descriptor, bloqueIndirectoBuffer, configuracionSuperBloque->BLOCK_SIZE);
	sleep(configuracion->RETARDO_ACCESO_BLOQUE/1000);
}

void guardarBloqueIndirecto(int descriptor, int offset) {
	log_info(logger, "Sincronizando los cambios en el bloque indirecto");
	lseek(descriptor, offset, SEEK_SET);
	write(descriptor, bloqueIndirectoBuffer, configuracionSuperBloque->BLOCK_SIZE);
	sleep(configuracion->RETARDO_ACCESO_BLOQUE/1000);
}

void leerBloque (int fd_ArchivoBloque, int numeroBloque, const void *datos){
	off_t offset = numeroBloque * configuracionSuperBloque->BLOCK_SIZE;
	lseek(fd_ArchivoBloque, offset, SEEK_SET);
	write(fd_ArchivoBloque, datos, configuracionSuperBloque->BLOCK_SIZE);
}

int buscarPrimerBloqueVacio (t_bitarray* s_bitmap, uint32_t BLOCK_SIZE){
	long int posicion_libre;
	long int tamano_bitmap = bitarray_get_max_bit(s_bitmap);
	for(posicion_libre = 0; posicion_libre <  tamano_bitmap ; posicion_libre++ )
	 {
		log_info(logger, "Acceso a Bitmap - Bloque: %d - Estado: %d", posicion_libre, bitarray_test_bit(s_bitmap,  posicion_libre));
		 if(!bitarray_test_bit(s_bitmap,  posicion_libre)){
			 return posicion_libre;
		 }
	 }
	return -1;
}

int fileExiste(char* nombreArchivo) {
	FILE* archivo;
	if((archivo = fopen(nombreArchivo, "r"))) {
		fclose(archivo);
		return 1;
	}
	return 0;
}



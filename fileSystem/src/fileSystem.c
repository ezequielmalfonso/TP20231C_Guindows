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
t_super_bloque* configuracionSuperBloque;
//BITMAP
int bitmap;
size_t bitmapSize;
t_bitarray* s_bitmap;
//void* fileData;
// FCB



int main(void) {
	// Configuracion gral
	cargarConfiguracion();
	//log_warning(logger, "PATH: %s",configuracion->PATH_SUPERBLOQUE );
	cargarSuperBloque(configuracion->PATH_SUPERBLOQUE);
	cargarArchivoBloques(configuracion->PATH_BLOQUES, configuracionSuperBloque->BLOCK_SIZE, configuracionSuperBloque->BLOCK_COUNT);

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
	bitmap = open(path, O_RDWR | O_CREAT, 0777); // Paso la ruta absoluta
	//bitmap = open(path, O_RDWR , 0777); // Paso la ruta absoluta

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
	munmap(fileData, bitmapSize);
	close(bitmap);

	return 0;
}




// FUNCIONES PARA TRABAJAR CON BLOQUES PARA NO USAR MMAP CON ALGEBRA DE PUNTEROS XD

int fd_ArchivoBloques;
// Aca falta todavia crear estructura de tipo bloque, y dsp falta relacionar todo, BITMAP, ARCHIVO DE BLOQUES Y FCBS

int cargarArchivoBloques(char *path, int BLOCK_SIZE, int BLOCK_COUNT){
	if (open(path, O_RDWR) == -1){
		fd_ArchivoBloques = open(path, O_CREAT | O_RDWR);
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

void escribirBloque(int fd_ArchivoBloque, int numeroBloque, const void *datos){
	off_t offset = numeroBloque * configuracionSuperBloque->BLOCK_SIZE;
	lseek(fd_ArchivoBloque, offset, SEEK_SET);
	write(fd_ArchivoBloque, datos, configuracionSuperBloque->BLOCK_SIZE);
}


void leerBloque (int fd_ArchivoBloque, int numeroBloque, const void *datos){
	off_t offset = numeroBloque * configuracionSuperBloque->BLOCK_SIZE;
	lseek(fd_ArchivoBloque, offset, SEEK_SET);
	write(fd_ArchivoBloque, datos, configuracionSuperBloque->BLOCK_SIZE);
}


/*int buscarPrimerBloqueVacio (char* fileData, uint32_t BLOCK_SIZE){
	for(int i = 0 ; i < BLOCK_SIZE; i++){
		if( (fileData[i/8]) & (1 << (i % 8)) == 0){
			return i; // RETORNO EL INDICE DEL PRIMER BLOQUE VACIO
		}
	}
	return -1; // TODOS LOS BLOQUES OCUPADOS
}*/

int buscarPrimerBloqueVacio (t_bitarray* s_bitmap, uint32_t BLOCK_SIZE){
	int posicion_libre;
	long int tamano_bitmap = bitarray_get_max_bit(s_bitmap);
	printf("\n");

	for(posicion_libre = 1; posicion_libre <  tamano_bitmap ; posicion_libre++ )
	 {
		 if( !bitarray_test_bit(s_bitmap,  posicion_libre)){
			 //printf("Valor bit: %d ",bitarray_test_bit(s_bitmap,  posicion_libre));
			 return posicion_libre;
		 }
	 }
	printf("\n");
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



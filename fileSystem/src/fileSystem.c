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

// FCB

t_FCB* FCB_archivo;


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
	return 0;
}



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

	// void * mmap (void *address, size_t length, int protect, int flags, int filedes, off_t offset)
	void * fileData = mmap(NULL,bitmapSize,PROT_READ | PROT_WRITE,MAP_PRIVATE, bitmap,0);

	if (fileData == MAP_FAILED) {
	     log_info(logger, "Error al mapear el archivo");
	     close(bitmap);
	     return 1;
	}
	// Creo el bitarray
	bitarray_create_with_mode(fileData,bitmapSize, LSB_FIRST);
	// Pongo todos los bits en 0
	memset(fileData, 0, bitmapSize);
	//Sincronizo los datos en memoria con el archivo bitmap.dat
	msync(fileData, bitmapSize, MS_SYNC); //Podriamos verificar con un if si se sincronizo

	munmap(fileData, bitmapSize);
	close(bitmap);

	return 0;
}

t_fcb* recorrerDirectorioFCB(char* PATH){


	return lista;
}

/* CREO QUE ESTA MAL, PORQUE ACA ESTOY LEYENDO UN ARCHIVO, Y REALMENTE DEBERIA INICIAR VACIO

// LEER FCB's

int configuracionValidaFCB(t_config* FCB){
	return (config_has_property(FCB,"NOMBRE_ARCHIVO")
			&& config_has_property(FCB,"TAMANIO_ARCHIVO")
			&& config_has_property(FCB,"PUNTERO_DIRECTO")
			&& config_has_property(FCB,"PUNTERO_INDIRECTO")
			);
}

int datosFCB(char* path){
	t_config* FCB;
	FCB_archivo = malloc(sizeof(t_FCB));

	FCB = config_create(path);
	if(FCB == NULL){
		FCB = config_create(path);
	}
	if(FCB == NULL || !configuracionValidaFCB(FCB)){
		log_error(logger,"FCB invalido");
	}

	FCB_archivo->nombre_archivo = config_get_string_value (FCB,"NOMBRE_ARCHIVO");
	FCB_archivo->tamanio_archivo = config_get_int_value (FCB,"TAMANIO_ARCHIVO");
	FCB_archivo->puntero_directo = config_get_int_value (FCB, "PUNTERO_DIRECTO");
	FCB_archivo->puntero_indirecto = config_get_int_value (FCB,"PUNTERO_INDIRECTO");

	log_info(logger,
			"\nNOMBRE_ARCHIVO: %s \n"
			"TAMANIO_ARCHIVO: %d"
			"PUNTERO_DIRECTO: %d"
			"PUNTERO_INDIRECTO: %d",
			FCB_archivo->nombre_archivo,
			FCB_archivo->tamanio_archivo,
			FCB_archivo->puntero_directo,
			FCB_archivo->puntero_indirecto
	);

	return 0;
}
*/

int fileExiste(char* nombreArchivo) {
	FILE* archivo;
	if((archivo = fopen(nombreArchivo, "r"))) {
		fclose(archivo);
		return 1;
	}
	return 0;
}



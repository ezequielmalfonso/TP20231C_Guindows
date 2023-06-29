/*
 * memoriaUtils.c
 *
 *  Created on: 12 abr 2023
 *      Author: utnso
 */


#include "memoria_utils.h"

uint32_t puerto_escucha;
uint32_t tam_memoria;
uint32_t tam_segmento_0;
uint32_t cant_segmentos;
uint32_t retardo_memoria;
uint32_t retardo_compactacion;
char* algortimo_asignacion;

void* memoria;
t_list* tabla_de_segmentos;
t_segmento* segmento;
t_list* tabla_de_paginas;
t_list* tabla_de_huecos;
/*
 * Inicializacion de Estructuras administrativas de Memoria
 */

void inicializar_memoria(){

	tam_memoria 		 = configuracion->TAM_MEMORIA;
	tam_segmento_0 		 = configuracion->TAM_SEGMENTO_0;
	cant_segmentos 		 = configuracion->CANT_SEGMENTOS;
	retardo_memoria 	 = configuracion->RETARDO_MEMORIA;
	retardo_compactacion = configuracion->RETARDO_COMPACTACION;
	algortimo_asignacion = configuracion->ALGORITMO_ASIGNACION;
	memoria = malloc(configuracion->TAM_MEMORIA);

	// Creacion segmento 0
    segmento = malloc(sizeof(t_segmento));
    segmento->id_segmento      = 0;
    segmento->direccion_base   = 0;
    segmento->tamanio_segmento = configuracion->TAM_SEGMENTO_0;

	log_info(logger,"Algoritmo %s configurado", configuracion->ALGORITMO_ASIGNACION);
	log_info(logger,"Segmento 0 (Compartido) creado: Id: %d - Dir Base: %d - Tamanio: %d", segmento->id_segmento, segmento->direccion_base, segmento->tamanio_segmento);

	tabla_de_segmentos = list_create();
	list_add(tabla_de_segmentos, segmento);

	tabla_de_paginas = list_create();

	tabla_de_huecos = list_create();
	t_segmento* hueco = malloc(sizeof(t_segmento));;
	hueco->id_segmento	 = 0;
	hueco->direccion_base = segmento->direccion_base + segmento->tamanio_segmento;
	hueco->tamanio_segmento = sizeof(memoria)-segmento->tamanio_segmento;
	list_add(tabla_de_huecos, hueco);
}


t_list* cargarProceso(uint32_t pid){
	t_nodoDePagina* nodo = malloc(sizeof(t_nodoDePagina));
	t_list* tabla;
	nodo->tablaDelProceso = crearTabla();
	nodo->id_proceso=pid;
	list_add(tabla_de_paginas,nodo);
	tabla = nodo->tablaDelProceso;
	//free(nodo);TODO ver esto
	return tabla;
}


t_list* crearTabla(){
	t_list* tablaDelProceso=list_create();
	list_add(tablaDelProceso,segmento);
	return tablaDelProceso;
}

void crearSegmento(uint32_t pid, uint32_t id_seg, int tam){
	t_list* tablaProceso = buscarTabla(pid);
	t_segmento* seg;
	seg = malloc(sizeof(t_segmento));
	seg->id_segmento      = id_seg;

	if(!strcmp(configuracion->ALGORITMO_ASIGNACION, "BEST")){
		seg->direccion_base   = bestFit(tam);
	}else if(!strcmp(configuracion->ALGORITMO_ASIGNACION, "FIRST")){
		seg->direccion_base   = firstFit(tam);
	}else if(!strcmp(configuracion->ALGORITMO_ASIGNACION, "WORST")){
		//TODO falta el WORST
		//seg->direccion_base   = worstFit(tam); //
	}
	else{
		log_error(logger, "Fallo al cargar algoritmo de asignación!!!");
	}

	seg->tamanio_segmento = tam;
	list_add(tablaProceso,segmento);
	free(seg);
}
void* leerMemoria(uint32_t id_seg, uint32_t desplazamiento, uint32_t pid, int tam){
	t_segmento* seg = buscarSegmento(buscarTabla(pid),id_seg);
	void* direccion = memoria+seg->direccion_base+desplazamiento;
	void* leido = malloc(tam);
	memcpy(leido,direccion,tam);
	return leido;
}

int escribirEnMemoria(uint32_t id_seg,uint32_t  desplazamiento,uint32_t  pid, int tamanio, void* escribir){
	t_segmento* seg = buscarSegmento(buscarTabla(pid),id_seg);
	void* direccion = memoria+seg->direccion_base+desplazamiento;
	memcpy(direccion,escribir,tamanio);
	return 1;


}
t_list* buscarTabla(uint32_t pid){
	if(list_is_empty(tabla_de_paginas)) {	// no deberia entrar nunca?
		log_error(logger, "La tabla de paginas esta vacia");
	}
	int i=0;
	t_nodoDePagina* aux = list_get(tabla_de_paginas,i);
	while(pid!=aux->id_proceso){
		if(i >= list_size(tabla_de_paginas)) {
			log_error(logger, "Error en la busqueda de tabla");
			break;
		}
	aux	= list_get(tabla_de_paginas,i);
	i++;
	}
	return aux->tablaDelProceso;
	}
t_segmento* buscarSegmento(t_list* tabla, uint32_t id){
	t_segmento* segmentoN;
	int i;
			for(i=0;i<list_size(tabla);i++){
			segmentoN = list_get(tabla,i);
			if(segmentoN->id_segmento == id)break;
		}
	return segmentoN;
}
uint64_t firstFit(int tam){
	int i=0;
	t_segmento* segmentoAux = list_get(tabla_de_huecos,i);
	while(tam<segmentoAux->tamanio_segmento || segmentoAux==NULL )
	{
		// NO termino de entender q hace realmente, esta el whilw no tenia corte
		// y en replace esta queriendo reemplzar un posicion q no esta
		// Aca creo que habria q recuperar antes el size de la lista
		i++;
		if( i < list_size(tabla_de_huecos)){
			segmentoAux = list_get(tabla_de_huecos,i);
		}else{
			break;
		}

	}
	uint64_t nuevaBase = segmentoAux->direccion_base;
	if(segmentoAux==NULL){}// pedir acoplamiento}
	segmentoAux->direccion_base=segmentoAux->direccion_base+tam;
	segmentoAux->tamanio_segmento=segmentoAux->tamanio_segmento-tam;
	if(segmentoAux->tamanio_segmento<0){
		list_remove_and_destroy_element(tabla_de_huecos,i,free);
	}else{
	list_replace(tabla_de_huecos,i,segmentoAux);
	}
	return nuevaBase;
}

uint64_t bestFit(int tam){
	if(noHayEspacio(tam)){
		//logError
	}
	int i =0;
	t_segmento* segmentoAux2 = NULL;
	t_segmento* segmentoAux = list_get(tabla_de_huecos,0);
	for(i = 0;i<list_size(tabla_de_huecos);i++){
		segmentoAux= list_get(tabla_de_huecos,i);
		if(segmentoAux->tamanio_segmento>tam){
			if(segmentoAux2==NULL || segmentoAux->tamanio_segmento<segmentoAux2->tamanio_segmento){
				segmentoAux2 = segmentoAux;
			}
		}
	}
	uint64_t nuevaBase = segmentoAux2->direccion_base;
	if(segmentoAux2==NULL){}// pedir acoplamiento}
	segmentoAux2->direccion_base=segmentoAux->direccion_base+tam;
	segmentoAux2->tamanio_segmento=segmentoAux->tamanio_segmento-tam;

	if(segmentoAux->tamanio_segmento<0){
		list_remove_and_destroy_element(tabla_de_huecos,i,free);
	}else{
	list_replace(tabla_de_huecos,i,segmentoAux);
	}
	return nuevaBase;
}

bool noHayEspacio(int tam){
	t_segmento* segmentoAux = list_get(tabla_de_huecos,0);
	int espacioLibre=0;
	for(int i =0; i<list_size(tabla_de_huecos);i++){
		segmentoAux =  list_get(tabla_de_huecos,0);
		espacioLibre += segmentoAux->tamanio_segmento;
	}
	return tam>espacioLibre;
}
void eliminarSegmentoProceso(uint32_t pid, uint32_t sid){
	t_list* tablaProceso = buscarTabla(pid);
	int i=0;
	t_segmento* segmentoAux = list_get(tablaProceso,i);
	while(sid!=segmentoAux->id_segmento || i< list_size(tablaProceso)){//TODO resolver la variable CANT_SEGMENTOS
		i++;
		segmentoAux = list_get(tablaProceso,i);
	}
	if(segmentoAux==NULL){
		//log de error segmento no existe
	}
	list_remove_and_destroy_element(tablaProceso,i,free);

	list_add(tabla_de_huecos,segmentoAux);
	//log segmento borrado
}

void agregarHueco(t_segmento* seg){
	bool hayQueAgregarlo;
//	int breakcondition;
	t_segmento* sAux = list_get(tabla_de_huecos,0);
	for(int i = 0;i<list_size(tabla_de_huecos);i++){
		sAux = list_get(tabla_de_huecos,i);
		if(seg->direccion_base==sAux->direccion_base+sAux->tamanio_segmento){
		sAux->tamanio_segmento+=seg->tamanio_segmento; //un hueco termina donde arranca el nuevo
		hayQueAgregarlo = false;
		}
	}
	for(int i=0;i<list_size(tabla_de_huecos);i++){
		sAux = list_get(tabla_de_huecos,i);
		if(seg->direccion_base+seg->tamanio_segmento==sAux->direccion_base){
		sAux->direccion_base=seg->direccion_base;
		sAux->tamanio_segmento+=seg->tamanio_segmento;
		hayQueAgregarlo = false;
		}
	}
	if(hayQueAgregarlo)list_add(tabla_de_huecos,seg);
}
t_segmento* unirHuecosAlfinal(t_segmento* hueco, t_segmento* huecoNuevo){
	hueco->tamanio_segmento+=huecoNuevo->tamanio_segmento;

return hueco;
}

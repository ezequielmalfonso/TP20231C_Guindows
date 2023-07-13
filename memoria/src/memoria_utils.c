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

int mensaje_error;

pthread_mutex_t mx_memoria = PTHREAD_MUTEX_INITIALIZER;


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
	hueco->tamanio_segmento = configuracion->TAM_MEMORIA-segmento->tamanio_segmento;
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
		seg->direccion_base   = worstFit(tam); //
	}
	else{
		log_error(logger, "Fallo al cargar algoritmo de asignación!!!");
	}

	if(seg->direccion_base != -1){
		seg->tamanio_segmento = tam;
		list_add(tablaProceso,seg);
	}else{
		mensaje_error = -1;
	}


	//free(seg);
}
void* leerMemoria(uint32_t id_seg, uint32_t desplazamiento, uint32_t pid, int tam){
	pthread_mutex_lock(&mx_memoria);
	sleep(configuracion->RETARDO_MEMORIA/1000);
	t_segmento* seg = buscarSegmento(buscarTabla(pid),id_seg);
	void* direccion = memoria+seg->direccion_base+desplazamiento;
	void* leido = malloc(tam);
	memcpy(leido,direccion,tam);
	pthread_mutex_unlock(&mx_memoria);
	return leido;
}

int escribirEnMemoria(uint32_t id_seg,uint32_t  desplazamiento,uint32_t  pid, int tamanio, void* escribir){
	pthread_mutex_lock(&mx_memoria);
	sleep(configuracion->RETARDO_MEMORIA/1000);
	t_segmento* seg = buscarSegmento(buscarTabla(pid),id_seg);
	void* direccion = memoria+seg->direccion_base+desplazamiento;
	memcpy(direccion,escribir,tamanio);
	pthread_mutex_unlock(&mx_memoria);
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
	for(i=0;i<list_size(tabla);i++)
	{
		segmentoN = list_get(tabla,i);
		if(segmentoN->id_segmento == id)
			break;
	}
	return segmentoN;
}
uint64_t firstFit(int tam){
	int i=0;
	t_segmento* segmentoAux = list_get(tabla_de_huecos,i);
	while(tam > segmentoAux->tamanio_segmento && i <= list_size(tabla_de_huecos) )
	{
		i++;
		segmentoAux = list_get(tabla_de_huecos,i);
	}
	uint64_t nuevaBase = segmentoAux->direccion_base;
	// TODO ver : if(segmentoAux==NULL){}// pedir acoplamiento}
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

	int i =0;
	int pos_hueco = 0;
	t_segmento* segmentoAux2 = NULL;
	t_segmento* segmentoAux = list_get(tabla_de_huecos,0);
	for(i = 0;i<list_size(tabla_de_huecos);i++)
	{
		segmentoAux= list_get(tabla_de_huecos,i);

		if(segmentoAux->tamanio_segmento>=tam)
		{
			if(segmentoAux2 == NULL){
				segmentoAux2 = list_get(tabla_de_huecos,i);
				pos_hueco = i;
			}
			if( segmentoAux->tamanio_segmento < segmentoAux2->tamanio_segmento){
				segmentoAux2 = list_get(tabla_de_huecos,i);
				pos_hueco = i;
			}
		}
	}

	uint64_t nuevaBase = segmentoAux2->direccion_base;
	if(segmentoAux2==NULL){}// pedir acoplamiento}
	segmentoAux2->direccion_base=segmentoAux2->direccion_base+tam;
	segmentoAux2->tamanio_segmento=segmentoAux2->tamanio_segmento-tam;

	if(segmentoAux2->tamanio_segmento == 0){
		list_remove_and_destroy_element(tabla_de_huecos,pos_hueco,free);
	}else{
	list_replace(tabla_de_huecos,pos_hueco,segmentoAux2);
	}
	return nuevaBase;

}
uint64_t worstFit(int tam){

	int i =0;
	int pos_hueco = 0;

	t_segmento* segmentoAux2 = NULL;
	t_segmento* segmentoAux3 = NULL;
	t_segmento* segmentoAux = list_get(tabla_de_huecos,0);

	for(i = 0;i<list_size(tabla_de_huecos);i++)
	{
		segmentoAux= list_get(tabla_de_huecos,i);


		if(segmentoAux->tamanio_segmento>=tam)
		{
			if(segmentoAux2 == NULL){
				segmentoAux2 = list_get(tabla_de_huecos,i);
				pos_hueco = i;
			}else
			if( segmentoAux->tamanio_segmento > segmentoAux2->tamanio_segmento)
			{
				segmentoAux2 = list_get(tabla_de_huecos,i);
				pos_hueco = i;
			}
		}
	}
	uint64_t nuevaBase = segmentoAux2->direccion_base;
	if(segmentoAux2==NULL){}// pedir acoplamiento}
	segmentoAux2->direccion_base=segmentoAux2->direccion_base+tam;
	segmentoAux2->tamanio_segmento=segmentoAux2->tamanio_segmento-tam;

	if(segmentoAux2->tamanio_segmento == 0){
		list_remove_and_destroy_element(tabla_de_huecos,pos_hueco,free);
	}else{
		list_replace(tabla_de_huecos,pos_hueco,segmentoAux2);
	}
	return nuevaBase;
}

bool noHayEspacio(int tam){
	t_segmento* segmentoAux = list_get(tabla_de_huecos,0);
	int espacioLibre=0;

	//
	/*
	t_link_element* aux_h = tabla_de_huecos->head;
	while(aux_h!=NULL)
	{
		t_segmento* aux_h2 = aux_h->data;
		log_warning(logger, "Lista huecos: ID_seg: %d - tam_seg: %d ",aux_h2->id_segmento, aux_h2->tamanio_segmento );
		aux_h = aux_h->next;
	}
	*/
	//
	for(int i =0; i<list_size(tabla_de_huecos);i++){
		segmentoAux =  list_get(tabla_de_huecos,i);
		//log_warning(logger, "Lista huecos: ID_seg: %d - tam_seg: %d - BASE: %d",segmentoAux->id_segmento, segmentoAux->tamanio_segmento, segmentoAux->direccion_base );
		espacioLibre += segmentoAux->tamanio_segmento;
	}
	//log_warning(logger, "Espacio libre de memoria: %d - Tamaño necesario: %d" , espacioLibre, tam);
	return tam>espacioLibre;
}

bool hayEspacio(int tam){
	t_segmento* segmentoAux = list_get(tabla_de_huecos,0);
	int espacioLibre=0;
	for(int i =0; i<list_size(tabla_de_huecos);i++){
		segmentoAux =  list_get(tabla_de_huecos,i);

		if(tam <= segmentoAux->tamanio_segmento)
		{
			espacioLibre++;
		}
	}

	return espacioLibre;
}
void eliminarSegmentoProceso(uint32_t pid, uint32_t sid){
	t_list* tablaProceso = buscarTabla(pid);
	int i=0;
	t_segmento* segmentoAux = list_get(tablaProceso,i);
	while(sid!=segmentoAux->id_segmento && i< list_size(tablaProceso)){//TODO resolver la variable CANT_SEGMENTOS
		i++;
		segmentoAux = list_get(tablaProceso,i);
	}
	if(segmentoAux==NULL){
		//log de error segmento no existe
	}

	log_info(logger,"[KERNEL] PID: %d - Eliminar Segmento id: %d - BASE: %d - TAMAÑO: %d", pid, sid, segmentoAux->direccion_base , segmentoAux->tamanio_segmento);
	agregarHueco( segmentoAux);
	list_remove_and_destroy_element(tablaProceso,i,free);

}

void agregarHueco(t_segmento* seg){
	bool hayQueAgregarlo = true;
//	int breakcondition;
	log_info(logger,"Direccion del seg a llenar:%d",seg->direccion_base);
	t_segmento* sAux = list_get(tabla_de_huecos,0);
	// TODO ESTA BIEN Q en los dos if ponga la variable hayQueAgregarlo en false???
	for(int i = 0;i<list_size(tabla_de_huecos);i++){
		sAux = list_get(tabla_de_huecos,i);
		if(seg->direccion_base==sAux->direccion_base+sAux->tamanio_segmento){
		sAux->tamanio_segmento+=seg->tamanio_segmento; //un hueco termina donde arranca el nuevo
		hayQueAgregarlo = false;
		log_info(logger,"manoseo el hueco");
		}
	}
	for(int i=0;i<list_size(tabla_de_huecos);i++){
		sAux = list_get(tabla_de_huecos,i);
		if(seg->direccion_base+seg->tamanio_segmento==sAux->direccion_base){
		sAux->direccion_base=seg->direccion_base;
		sAux->tamanio_segmento+=seg->tamanio_segmento;
		hayQueAgregarlo = false;
		log_info(logger,"manoseo el hueco");
		}
	}
	if(hayQueAgregarlo)
	{
		t_segmento* nuevo_hueco=malloc(sizeof(t_segmento));
		nuevo_hueco->direccion_base=seg->direccion_base;
		nuevo_hueco->tamanio_segmento=seg->tamanio_segmento;
		log_info(logger,"creo un hueco: base:%d - tam: %d",nuevo_hueco->direccion_base,nuevo_hueco->tamanio_segmento);
		list_add(tabla_de_huecos,nuevo_hueco);
	}


	//
	/*t_link_element* aux_h = tabla_de_huecos->head;
	while(aux_h!=NULL)
	{
		t_segmento* aux_h2 = aux_h->data;
		log_error(logger, "Lista huecos: ID_seg: %d - tam_seg: %d ",aux_h2->id_segmento, aux_h2->tamanio_segmento );
		aux_h = aux_h->next;
	}*/
	//

}

t_segmento* unirHuecosAlfinal(t_segmento* hueco, t_segmento* huecoNuevo){
	hueco->tamanio_segmento+=huecoNuevo->tamanio_segmento;

return hueco;
}





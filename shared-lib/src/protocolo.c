
#include "protocolo.h"
#include "estructuras.h"

//INSTRUCCIONES CONSOLA A KERNEL

void enviar_instrucciones(int socket_fd, t_list* lista){

	uint32_t size = calcular_instrucciones_buffer_size(lista);
	uint32_t sizeBuffer=size+ sizeof(int);
	void* stream = serializar_instrucciones_tam(size, lista);
	t_buffer* buffer=malloc(sizeBuffer);
	buffer->size=size;
	buffer->stream=stream;

	void* a_enviar=malloc(sizeBuffer);
	int offset=0;

	memcpy(a_enviar + offset, &(buffer->size), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(a_enviar + offset, buffer->stream, buffer->size);

	send(socket_fd, a_enviar,buffer->size+sizeof(uint32_t) ,0);

	free(a_enviar);
    free(buffer->stream);
    free(buffer);

}
t_instrucciones* recibir_instrucciones(int socket_fd)
{
	t_buffer* buffer=malloc(sizeof(t_buffer));

	recv(socket_fd, &(buffer->size), sizeof(int), MSG_WAITALL);
	buffer->stream=malloc(buffer->size);
	recv(socket_fd, buffer->stream, buffer->size, MSG_WAITALL);
	t_instrucciones* mensaje = deserializar_instrucciones(buffer);

	free(buffer->stream);
    free(buffer);

	return mensaje;
}

uint32_t calcular_instrucciones_buffer_size(t_list* lista){

	uint32_t size=0;
	//int i=0;

	size += 25*(lista->elements_count)*sizeof(uint32_t);

	//free(listaIns);
	//free(aux);
	return size;
}
void* serializar_instrucciones_tam(uint32_t size, t_list* lista) {

    INSTRUCCION* aux;

    aux = list_get(lista,0);
    uint32_t elementosLista= list_size(lista);

    uint32_t offset = 0;
    void* stream = malloc(size);

    memcpy(stream + offset, &elementosLista, sizeof(uint32_t));
    offset+= sizeof(uint32_t);

    t_link_element* aux1 = lista->head;
    //printf("Verificamos la lista:\n");
    while( aux1!=NULL )
	{
		INSTRUCCION* auxl2 = aux1->data;
		//printf("Comando: %s | Par1: %s | Par2: %s | Par3: %s\n", auxl2->comando, auxl2->parametro1, auxl2->parametro2, auxl2->parametro3 );

		memcpy(stream + offset, &auxl2->comando, sizeof(aux->comando));
		offset += sizeof(aux->comando);
		memcpy(stream + offset, &auxl2->parametro1, sizeof(aux->parametro1));
		offset += sizeof(aux->parametro1);
		memcpy(stream + offset, &auxl2->parametro2, sizeof(aux->parametro2));
		offset += sizeof(aux->parametro2);
		memcpy(stream + offset, &auxl2->parametro3, sizeof(aux->parametro3));
		offset += sizeof(aux->parametro3);
		aux1 = aux1->next;
	}

    free(aux);
    return stream;
}
t_instrucciones* deserializar_instrucciones(t_buffer* buffer){
    int i=0, c=0;
	t_instrucciones* mensaje=malloc(sizeof(t_instrucciones));
	void* stream = buffer->stream;

	memcpy(&(mensaje->elementosLista), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);

	mensaje->listaInstrucciones=list_create();
	//printf("Verificamos la lista:\n");
	while(i!=mensaje->elementosLista)
	{
		INSTRUCCION* aux=malloc(sizeof(INSTRUCCION));

		memcpy(&(aux->comando), stream, sizeof(aux->comando));
	    stream += sizeof(aux->comando);
	    memcpy(&(aux->parametro1),stream , sizeof(aux->parametro1));
	    stream += sizeof(aux->parametro1);
	    memcpy(&(aux->parametro2), stream, sizeof(aux->parametro2));
	    stream += sizeof(aux->parametro2);
	    memcpy(&(aux->parametro3), stream, sizeof(aux->parametro3));
	    stream += sizeof(aux->parametro3);
	    //printf("Comando: %s | Par1: %s | Par2: %s | Par3: %s\n", aux->comando, aux->parametro1, aux->parametro2, aux->parametro3 );
	    list_add(mensaje->listaInstrucciones,aux);
	    i++;
	}

	return mensaje;
}

//ENVIAR PCB KERNEL->CPU_DISPATCH

bool send_proceso(int fd, PCB_t *proceso,op_code codigo) {
	//////////////
	//printf("\n\n######protocolo.c  send_proceso### esta empty?: %d\n", list_is_empty(proceso->archivos_abiertos));
	//////////
    size_t size;
    void* stream = serializar_proceso(&size, proceso,codigo);
    if (send(fd, stream, size, 0) != size) {
        free(stream);
        return false;
    }
    free(stream);
    return true;
}

static void* serializar_proceso(size_t* size, PCB_t *proceso, op_code codigo) {


	uint32_t elementosLista         = list_size(proceso->instrucciones);
	uint32_t tablaSegmentos         = list_size(proceso->tabla_de_segmentos);
	uint32_t list_archivos_abiertos = list_size(proceso->archivos_abiertos);

	*size= sizeof(op_code)+   // 4
		   sizeof(size_t)+ 			//SIZE
		   sizeof(uint16_t)+ 		//SIZE PID
		   sizeof(uint32_t)+ 		//SIZE elementosLista
		   sizeof(INSTRUCCION)* elementosLista+ 	//SIZE LISTA INSTRUCCIONES
		   sizeof(uint32_t)+ 		//SIZE PC
		   sizeof(registros_t)+ 	//SIZE REGISTROS
		   sizeof(uint32_t)+ 		//SIZE tablaSegmento
		   sizeof(t_segmento)*tablaSegmentos+//((25)*tablaSegmentos)+   //sizeof(uint32_t)+//SIZE cantListaSegmentos
		   sizeof(double)+         	// Estimado rafaga
		   sizeof(uint32_t)+       	// tiempo de llegada
		   sizeof(t_archivoAbierto)*list_archivos_abiertos + //SIZE SEGMENTOS SIN LOS ID
		   sizeof(int); 			//SIZE CONSOLA FD

	size_t size_load = *size- sizeof(op_code) -sizeof(size_t);
	uint32_t offset = 0;
	void* stream = malloc(*size);

	op_code cop = codigo;
	memcpy(stream + offset, &cop, sizeof(op_code));
	offset+= sizeof(op_code);

	memcpy(stream + offset, &size_load, sizeof(size_t));
	offset+= sizeof(size_t);

	memcpy(stream + offset, &proceso->pid, sizeof(uint16_t));
	offset+= sizeof(uint16_t);

	memcpy(stream + offset, &elementosLista, sizeof(uint32_t));
	offset+= sizeof(uint32_t);

	t_link_element* aux1 = proceso->instrucciones->head;
	while( aux1!=NULL )
	{
		INSTRUCCION* auxl2 = aux1->data;
		//printf("Verificamos la lista:\n");
		//printf("Comando: %s | Par1: %s | Par2: %s | Par3: %s\n\n", auxl2->comando, auxl2->parametro1, auxl2->parametro2, auxl2->parametro3 );

		memcpy(stream + offset, &auxl2->comando, sizeof(auxl2->comando));
		offset += sizeof(auxl2->comando);
		memcpy(stream + offset, &auxl2->parametro1, sizeof(auxl2->parametro1));
		offset += sizeof(auxl2->parametro1);
		memcpy(stream + offset, &auxl2->parametro2, sizeof(auxl2->parametro2));
		offset += sizeof(auxl2->parametro2);
		memcpy(stream + offset, &auxl2->parametro3, sizeof(auxl2->parametro3));
		offset += sizeof(auxl2->parametro3);
		aux1 = aux1->next;
	}

	memcpy(stream + offset, &proceso->pc, sizeof(uint32_t));
	offset+= sizeof(uint32_t);

	memcpy(stream + offset, &proceso->registro_cpu, sizeof(registros_t));
	offset+= sizeof(registros_t);
/*	memcpy(stream + offset, &proceso->registro_cpu, sizeof(char*));
	offset+= sizeof(char*);
	memcpy(stream + offset, &proceso->registro_cpu, sizeof(char*));
	offset+= sizeof(char*);
	memcpy(stream + offset, &proceso->registro_cpu, sizeof(char*));
	offset+= sizeof(char*);*/

	memcpy(stream + offset, &tablaSegmentos, sizeof(uint32_t));
	offset+= sizeof(uint32_t);

	t_link_element* aux_tab_seg = proceso->tabla_de_segmentos->head;

	while( aux_tab_seg!=NULL )
	{
		t_segmento* aux_tab_seg_2 = aux_tab_seg->data;
		memcpy(stream + offset, &aux_tab_seg_2->id_segmento, sizeof(aux_tab_seg_2->id_segmento));
		offset += sizeof(aux_tab_seg_2->id_segmento);
		memcpy(stream + offset, &aux_tab_seg_2->direccion_base, sizeof(aux_tab_seg_2->direccion_base));
		offset += sizeof(aux_tab_seg_2->direccion_base);
		memcpy(stream + offset, &aux_tab_seg_2->tamanio_segmento, sizeof(aux_tab_seg_2->tamanio_segmento));
		offset += sizeof(aux_tab_seg_2->tamanio_segmento);
		aux_tab_seg = aux_tab_seg->next;
	}

	memcpy(stream + offset, &proceso->estimado_proxima_rafaga, sizeof(double));
	offset+= sizeof(double);
	memcpy(stream + offset, &proceso->tiempo_llegada_a_ready, sizeof(uint32_t));
	offset+= sizeof(uint32_t);

	memcpy(stream + offset, &list_archivos_abiertos, sizeof(uint32_t)); //cantidad archviso abiertos
	offset+= sizeof(uint32_t);

	t_link_element* aux_list_arch = proceso->archivos_abiertos->head;
	while(aux_list_arch != NULL)
	{
		t_archivoAbierto* aux_list_arch2 = aux_list_arch->data;
		memcpy(stream + offset, &aux_list_arch2->nombre_archivo, sizeof(aux_list_arch2->nombre_archivo));
		offset += sizeof(aux_list_arch2->nombre_archivo);
		memcpy(stream + offset, &aux_list_arch2->puntero, sizeof(aux_list_arch2->puntero));
		offset += sizeof(aux_list_arch2->puntero);
		aux_list_arch = aux_list_arch->next;
	}

	memcpy(stream + offset, &proceso->cliente_fd, sizeof(int));

	//free(aux);
	return stream;
}
// RECIBO PCB DESDE KERNEL al CPU
bool recv_proceso(int fd, PCB_t* proceso){
    size_t size_payload;
    if (recv(fd, &size_payload, sizeof(size_t), MSG_WAITALL) != sizeof(size_t))
        return false;
    void* stream = malloc(size_payload);
    if (recv(fd, stream, size_payload, MSG_WAITALL) != size_payload) {
        free(stream);
        return false;
    }
    deserializar_proceso(stream, proceso);
    //////////////
    	//printf("\n\n########protocolo.c  recv_proceso### esta empty?: %d\n", list_is_empty(proceso->archivos_abiertos));
    	//////////
    free(stream);
    return true;

}

static void deserializar_proceso(void* stream, PCB_t* proceso) {
	int i=0,c=0, j=0;

	uint32_t elementosLista=0, cantSegmentos=0, cantidadArchivos = 0;

	memcpy(&(proceso->pid), stream, sizeof(uint16_t));
	stream+=sizeof(uint16_t);

	proceso->instrucciones=list_create();

	memcpy(&elementosLista, stream, sizeof(uint32_t));
	stream+=sizeof(uint32_t);

	while(i!=elementosLista)
	{
		INSTRUCCION* aux=malloc(sizeof(INSTRUCCION));

		memcpy(&(aux->comando), stream, sizeof(aux->comando));
		stream += sizeof(aux->comando);
		memcpy(&(aux->parametro1),stream , sizeof(aux->parametro1));
		stream += sizeof(aux->parametro1);
		memcpy(&(aux->parametro2), stream, sizeof(aux->parametro2));
		stream += sizeof(aux->parametro2);
		memcpy(&(aux->parametro3), stream, sizeof(aux->parametro3));
		stream += sizeof(aux->parametro3);
		list_add(proceso->instrucciones,aux);

		//printf("Verificamos la lista recibida en cpu:\n");
		//printf("Comando: %s | Par1: %s | Par2: %s | Par: %s\n\n", aux->comando, aux->parametro1, aux->parametro2 ,aux->parametro3);
		i++;
	}

	memcpy(&(proceso->pc), stream, sizeof(uint32_t));
	stream+=sizeof(uint32_t);
//printf("PC: %s", proceso->pc);

	memcpy(&(proceso->registro_cpu),stream, sizeof(registros_t));
	stream+=sizeof(registros_t);
/*	memcpy(&(proceso->registro_cpu),stream, sizeof(char*));
	stream+=sizeof(char*);
	memcpy(&(proceso->registro_cpu),stream, sizeof(char*));
	stream+=sizeof(char*);
	memcpy(&(proceso->registro_cpu),stream, sizeof(char*));
	stream+=sizeof(char*);
*/
	memcpy(&cantSegmentos, stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	proceso->tabla_de_segmentos=list_create();


	while(c < cantSegmentos)
	{
	  t_segmento* aux_seg = (t_segmento*)malloc(sizeof(t_segmento));
	  aux_seg->id_segmento = 0;
	  aux_seg->direccion_base = 0;
	  aux_seg->tamanio_segmento = 0;
	  memcpy(&(aux_seg->id_segmento), stream, sizeof(aux_seg->id_segmento));
	  stream += sizeof(aux_seg->id_segmento);
	  memcpy(&(aux_seg->direccion_base), stream, sizeof(aux_seg->direccion_base));
	  stream += sizeof(aux_seg->direccion_base);
	  memcpy(&(aux_seg->tamanio_segmento), stream, sizeof(aux_seg->tamanio_segmento));
	  stream += sizeof(aux_seg->tamanio_segmento);
	  list_add(proceso->tabla_de_segmentos, aux_seg);
	  //printf("Comando: %d | Par1: %d | Par2: %d \n\n", aux_seg->id_segmento, aux_seg->direccion_base, aux_seg->tamanio_segmento );
	  c++;
	}

	memcpy(&proceso->estimado_proxima_rafaga, stream, sizeof(double));
	stream+= sizeof(double);
	memcpy(&proceso->tiempo_llegada_a_ready, stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	memcpy(&cantidadArchivos, stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	while(j<cantidadArchivos)
	{
		t_archivoAbierto* aux_arch = (t_archivoAbierto*)malloc(sizeof(t_archivoAbierto));
		memcpy(&(aux_arch->nombre_archivo), stream, sizeof(aux_arch->nombre_archivo));
		stream += sizeof(aux_arch->nombre_archivo);
		memcpy(&(aux_arch->puntero), stream, sizeof(aux_arch->puntero));
		stream += sizeof(aux_arch->puntero);
		list_add(proceso->archivos_abiertos, aux_arch);
		j++;
	}

	memcpy(&proceso->cliente_fd, stream, sizeof(int));


}

// ENVIO DE INSTRUCCION KERNEL-FILESYSTEM
bool send_archivo(int fd, char* param1, char* param2, char* param3, op_code codigo) {
	size_t size;
	void* stream = serializar_instruccion(&size, param1, param2, param3, codigo);
	if (send(fd, stream, size, 0) != size) {
		free(stream);
		return false;
	}
	free(stream);
	return true;
}

static void* serializar_instruccion(size_t* size, char* param1, char* param2, char* param3, op_code codigo) {
	size_t char20size = sizeof(char) * 20;
	size_t opcodesize = sizeof(op_code);
	*size = 3 * char20size + opcodesize + sizeof(size_t);
	void * stream = malloc(*size);
	size_t size_load = *size- opcodesize - sizeof(size_t);	// El size no incluye el size ni el opcode (se saca antes)

	memcpy(stream, &codigo, opcodesize);
	int offset = opcodesize;
	memcpy(stream + offset, &size_load, sizeof(size_t));
	offset += sizeof(size_t);
	memcpy(stream + offset, param1, char20size);
	offset += char20size;
	memcpy(stream + offset, param2, char20size);
	offset += char20size;
	memcpy(stream + offset, param3, char20size);
	return stream;
}

bool recv_instruccion(int fd, char* param1, char* param2, char* param3) {
	size_t size_payload;
	if (recv(fd, &size_payload, sizeof(size_t), MSG_WAITALL) != sizeof(size_t))
		return false;
	void* stream = malloc(size_payload);
	if (recv(fd, stream, size_payload, MSG_WAITALL) != size_payload) {
		free(stream);
	    return false;
	}
	    deserializar_instruccion(stream, param1, param2, param3);
	    free(stream);
	    return true;
}

static void deserializar_instruccion(void* stream, char* param1, char* param2, char* param3) {
	size_t char20size = sizeof(char) * 20;
	memcpy(param1, stream, char20size);
	stream += char20size;
	memcpy(param2, stream, char20size);
	stream += char20size;
	memcpy(param3, stream, char20size);
}

// ENVIO DE INSTRUCCION CPU-Memoria
bool send_pedido_memoria(int fd, uint32_t num_seg, uint32_t desplazamiento, uint32_t pid,int tamanio, op_code codigo) {
	size_t size;
	void* stream = serializar_instruccion_memoria(&size, num_seg, desplazamiento, pid,tamanio, codigo);
	if (send(fd, stream, size, 0) != size) {
		free(stream);
		return false;
	}
	free(stream);
	return true;
}

static void* serializar_instruccion_memoria(size_t* size, uint32_t param1, uint32_t param2, uint32_t param3,int param4, op_code codigo) {
	size_t opcodesize = sizeof(op_code);
	*size = 4 *sizeof(uint32_t) + opcodesize + sizeof(size_t);
	void * stream = malloc(*size);
	size_t size_load = *size- opcodesize - sizeof(size_t);	// El size no incluye el size ni el opcode (se saca antes)

	memcpy(stream, &codigo, opcodesize);
	int offset = opcodesize;
	memcpy(stream + offset, &size_load, sizeof(size_t));
	offset += sizeof(size_t);
	memcpy(stream + offset, &param1,sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &param2,sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &param3,sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &param4,sizeof(int));
	return stream;
}

bool recv_instruccion_memoria(int fd, uint32_t num_seg, uint32_t desplazamiento, uint32_t pid,int tamanio) {
	size_t size_payload;
	if (recv(fd, &size_payload, sizeof(size_t), MSG_WAITALL) != sizeof(size_t))
		return false;
	void* stream = malloc(size_payload);
	if (recv(fd, stream, size_payload, MSG_WAITALL) != size_payload) {
		free(stream);
	    return false;
	}
	    deserializar_instruccion_memoria(stream, num_seg,desplazamiento,pid,tamanio);
	    free(stream);
	    return true;
}

static void deserializar_instruccion_memoria(void* stream, uint32_t param1, uint32_t param2, uint32_t param3,int param4) {
	memcpy(&param1, stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memcpy(&param2, stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memcpy(&param3, stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memcpy(&param4, stream, sizeof(int));

}
bool send_escribir_memoria(int fd, uint32_t num_seg, uint32_t desplazamiento, uint32_t pid,void* escribir,int tamanio, op_code codigo) {
	size_t size;
	void* stream = serializar_escribir_memoria(&size, num_seg, desplazamiento, pid,escribir,tamanio, codigo);
	if (send(fd, stream, size, 0) != size) {
		free(stream);
		return false;
	}
	free(stream);
	return true;
}

static void* serializar_escribir_memoria(size_t* size, uint32_t param1, uint32_t param2, uint32_t param3,void* param4, int tam, op_code codigo) {
	size_t opcodesize = sizeof(op_code);
	*size = 4 *sizeof(uint32_t) + opcodesize + sizeof(size_t);
	void * stream = malloc(*size);
	size_t size_load = *size- opcodesize - sizeof(size_t);	// El size no incluye el size ni el opcode (se saca antes)

	memcpy(stream, &codigo, opcodesize);
	int offset = opcodesize;
	memcpy(stream + offset, &size_load, sizeof(size_t));
	offset += sizeof(size_t);
	memcpy(stream + offset, &param1,sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &param2,sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &param3,sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(stream + offset, &param4,tam);
	memcpy(stream + offset, &tam,sizeof(int));
	offset += sizeof(int);
	memcpy(stream + offset, &param4,tam);
	return stream;
}

static void deserializar_escribir_memoria(void* stream, uint32_t param1, uint32_t param2, uint32_t param3,int param4, void* escribir) {
	memcpy(&param1, stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memcpy(&param2, stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memcpy(&param3, stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	memcpy(&param4, stream, sizeof(int));
	stream += sizeof(int);
	memcpy(escribir, stream, param4);

}
bool recv_escribir_memoria(int fd, uint32_t num_seg, uint32_t desplazamiento, uint32_t pid,int tamanio, void* escribir) {
	size_t size_payload;
	if (recv(fd, &size_payload, sizeof(size_t), MSG_WAITALL) != sizeof(size_t))
		return false;
	void* stream = malloc(size_payload);
	if (recv(fd, stream, size_payload, MSG_WAITALL) != size_payload) {
		free(stream);
	    return false;
	}
	    deserializar_escribir_memoria(stream, num_seg,desplazamiento,pid,tamanio,escribir);
	    free(stream);
	    return true;
}

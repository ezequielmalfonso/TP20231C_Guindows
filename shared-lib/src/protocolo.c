
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


	uint32_t elementosLista= list_size(proceso->instrucciones);
	uint32_t tablaSegmentos= list_size(proceso->tabla_de_segmentos);

	*size= sizeof(op_code)+   // 4
		   sizeof(size_t)+ 			//SIZE
		   sizeof(uint16_t)+ 		//SIZE PID
		   sizeof(uint32_t)+ 		//SIZE elementosLista
		   sizeof(INSTRUCCION)* elementosLista+ 	//SIZE LISTA INSTRUCCIONES
		   sizeof(uint32_t)+ 		//SIZE PC
		   (sizeof(uint32_t)*4)+ 	//SIZE REGISTROS
		   sizeof(uint32_t)+ 		//SIZE tablaSegmento
		   sizeof(t_segmento)*tablaSegmentos+//((25)*tablaSegmentos)+   //sizeof(uint32_t)+//SIZE cantListaSegmentos
		   sizeof(double)+         	// Estimado rafaga
		   sizeof(uint32_t)+       	// tiempo de llegada
		   //sizeof(uint32_t)*listSegmentos + //SIZE SEGMENTOS SIN LOS ID
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

	memcpy(stream + offset, &proceso->registro_cpu[0], sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream + offset, &proceso->registro_cpu[1], sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream + offset, &proceso->registro_cpu[2], sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream + offset, &proceso->registro_cpu[3], sizeof(uint32_t));
	offset+= sizeof(uint32_t);

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

	memcpy(stream + offset, &proceso->estimado_rafaga_inicial, sizeof(uint32_t));
	offset+= sizeof(uint32_t);
	memcpy(stream + offset, &proceso->tiempo_llegada_a_ready, sizeof(uint32_t));
	offset+= sizeof(uint32_t);

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
    free(stream);
    return true;

}

static void deserializar_proceso(void* stream, PCB_t* proceso) {
	int i=0,c=0;

	uint32_t elementosLista=0, cantSegmentos=0;

	memcpy(&(proceso->pid), stream, sizeof(uint16_t));
	stream+=sizeof(uint16_t);

	proceso->instrucciones=list_create();
	proceso->tabla_de_segmentos=list_create();

	memcpy(&elementosLista, stream, sizeof(uint32_t));
	stream+=sizeof(uint32_t);

	while(i!=elementosLista)
	{
		INSTRUCCION* aux=malloc(sizeof(INSTRUCCION));
		printf("Verificamos la lista recibida en cpu:\n");
		printf("Comando: %s | Par1: %s | Par2: %s | Par: %s\n\n", aux->comando, aux->parametro1, aux->parametro2 ,aux->parametro3);

		memcpy(&(aux->comando), stream, sizeof(aux->comando));
		stream += sizeof(aux->comando);
		memcpy(&(aux->parametro1),stream , sizeof(aux->parametro1));
		stream += sizeof(aux->parametro1);
		memcpy(&(aux->parametro2), stream, sizeof(aux->parametro2));
		stream += sizeof(aux->parametro2);
		memcpy(&(aux->parametro3), stream, sizeof(aux->parametro3));
		stream += sizeof(aux->parametro3);
		list_add(proceso->instrucciones,aux);
		i++;
	}

	memcpy(&(proceso->pc), stream, sizeof(uint32_t));
	stream+=sizeof(uint32_t);

	memcpy(&(proceso->registro_cpu[0]),stream, sizeof(uint32_t));
	stream+=sizeof(uint32_t);
	memcpy(&(proceso->registro_cpu[1]),stream, sizeof(uint32_t));
	stream+=sizeof(uint32_t);
	memcpy(&(proceso->registro_cpu[2]),stream, sizeof(uint32_t));
	stream+=sizeof(uint32_t);
	memcpy(&(proceso->registro_cpu[3]),stream, sizeof(uint32_t));
	stream+=sizeof(uint32_t);

	memcpy(&cantSegmentos, stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	while(c < cantSegmentos)
	{
	  t_segmento* aux_seg = malloc(sizeof(t_segmento));

	  memcpy(&(aux_seg->id_segmento), stream, sizeof(aux_seg->id_segmento));
	  stream += sizeof(aux_seg->id_segmento);
	  memcpy(&(aux_seg->direccion_base), stream, sizeof(aux_seg->direccion_base));
	  stream += sizeof(aux_seg->direccion_base);
	  memcpy(&(aux_seg->tamanio_segmento), stream, sizeof(aux_seg->tamanio_segmento));
	  stream += sizeof(aux_seg->tamanio_segmento);

	  list_add(proceso->tabla_de_segmentos, aux_seg);
	  c++;
	}

	memcpy(&proceso->estimado_rafaga_inicial, stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);
	memcpy(&proceso->tiempo_llegada_a_ready, stream, sizeof(uint32_t));
	stream+= sizeof(uint32_t);

	memcpy(&proceso->cliente_fd, stream, sizeof(int));


}


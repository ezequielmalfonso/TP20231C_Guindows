
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
	int i=0;

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
	   // printf("Comando: %s | Par1: %s | Par2: %s | Par3: %s\n", aux->comando, aux->parametro1, aux->parametro2, aux->parametro3 );
	    list_add(mensaje->listaInstrucciones,aux);
	    i++;
	}

	return mensaje;
}

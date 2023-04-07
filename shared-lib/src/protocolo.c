
#include "protocolo.h"


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

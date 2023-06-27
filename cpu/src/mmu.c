#include "mmu.h"


uint32_t num_seg(int logica){

	return logica/configuracion->TAM_MAX_SEGMENTO;
}
uint32_t desplazamiento(int logica){
	return logica%configuracion->TAM_MAX_SEGMENTO;
}

void mov_in(char direccion_logica[20], char registro[20], PCB_t* pcb){
	uint32_t desplazamiento1 = desplazamiento(atoi(direccion_logica));
	uint32_t n_segmento = num_seg(atoi(direccion_logica));

	int tamanio = calcularTam(registro);
	/*if(checkSegmentetitonFault(desplazamiento1, n_segmento,pcb)){
		//log error segmentation fault
	}*/
	char* registro_recibido= malloc(20);
	send_pedido_memoria(memoria_fd,n_segmento,desplazamiento1,(pcb->pid),tamanio, MOV_IN);
	recv(memoria_fd, registro_recibido ,tamanio, MSG_WAITALL);
	log_info(logger,"estamos recibiendo de memoria: %s", registro_recibido);
	set_registro(registro, registro_recibido, pcb);
//	log_info(logger,"PID: %d - Ejecutando SET parametro 1: %s parametro 2: %s", pid,instruccion_ejecutar->parametro1,instruccion_ejecutar->parametro2);
//	bool recv_instruccion(memoria_fd, tam, respuestaMemoria, char* param3);
//enviar pedido a memoria TODO
}
void mov_out(char* direccion_logica, char* registro,PCB_t* pcb){
	uint32_t desplazamiento1 = desplazamiento(atoi(direccion_logica));
		uint32_t n_segmento = num_seg(atoi(direccion_logica));
		int tamanio = calcularTam(registro);
		char* escribir = malloc(tamanio);
		memcpy(escribir,leer_registro(registro),tamanio);
		log_info(logger, "estamos mandando a memoria: %s", escribir);
		if(checkSegmentetitonFault(desplazamiento1, n_segmento,pcb)){
			//log error segmentation fault
		}
		send_escribir_memoria(memoria_fd,n_segmento,desplazamiento1,(pcb->pid),escribir,tamanio,MOV_OUT);
		op_code cop;
		recv(memoria_fd, &cop, sizeof(cop), 0);
		log_info(logger,"se escribio en memoria:  %d",cop);
		//enviar pedido a memoria TODO
		free(escribir);
}

int checkSegmentetitonFault(uint32_t desplazamiento, uint32_t n_segmento,PCB_t* pcb){
	t_segmento* segmentoN;
		for(int i;i<list_size(pcb->tabla_de_segmentos);i++){
		segmentoN = list_get(pcb->tabla_de_segmentos,i);
		if(segmentoN->id_segmento == n_segmento)break;
	}
	return desplazamiento > segmentoN->tamanio_segmento;
}

int calcularTam(char* registro){
	switch(registro[0]){
	case 'E': return 8;
	case 'R': return 16;
	default: return 4;
}
}
void set_registro(char* registro1,char* registro_recibido, PCB_t* pcb){
	//log_warning(logger,"Registro: %s", registro_recibido);
	//pcb->registro_cpu = malloc(sizeof(registros_t));
	log_info(logger,"registro recibido: %s", registro_recibido);
	char* registro = strtok(registro1,"\n");
	if(!strcmp(registro,"AX")){
		memcpy(regAX, registro_recibido, sizeof(char) * 4);
		log_info(logger, "reg AX: %s", regAX);
		//parseo_registro(registro_recibido, pcb,4);
	}else if(!strcmp(registro,"BX")){
		memcpy(regBX, registro_recibido, sizeof(char) * 4);
		//parseo_registro(registro_recibido, pcb,4);
	}else if(!strcmp(registro,"CX")){
		memcpy(regCX, registro_recibido, sizeof(char) * 4);
		//parseo_registro(registro_recibido, pcb,4);
	}else if(!strcmp(registro,"DX")){
		memcpy(regDX, registro_recibido, sizeof(char) * 4);
		//parseo_registro(registro_recibido, pcb,4);
	}else if(!strcmp(registro,"EAX")){
		memcpy(regEAX, registro_recibido, sizeof(char) * 8);
		//parseo_registro(registro_recibido, pcb,8);
	}else if(!strcmp(registro,"EBX")){
		memcpy(regEBX, registro_recibido, sizeof(char) * 8);
		//parseo_registro(registro_recibido, pcb,8);
	}else if(!strcmp(registro,"ECX")){
		memcpy(regECX, registro_recibido, sizeof(char) * 8);
		//parseo_registro(registro_recibido, pcb,8);
	}else if(!strcmp(registro,"EDX")){
		memcpy(regEDX, registro_recibido, sizeof(char) * 8);
		//parseo_registro(registro_recibido, pcb,16);
	}else if(!strcmp(registro,"RAX")){
		memcpy(regRAX, registro_recibido, sizeof(char) * 16);
		//parseo_registro(registro_recibido, pcb,16);
	}else if(!strcmp(registro,"RBX")){
		memcpy(regRBX, registro_recibido, sizeof(char) * 16);
		//parseo_registro(registro_recibido, pcb,16);
	}else if(!strcmp(registro,"RCX")){
		memcpy(regRCX, registro_recibido, sizeof(char) * 16);
		//parseo_registro(registro_recibido, pcb,16);
	}else if(!strcmp(registro,"RDX")){
		memcpy(regRDX, registro_recibido, sizeof(char) * 16);
		//parseo_registro(registro_recibido, pcb,16);
	}else{
		log_error(logger,"me fui al else del set");
	}
	log_info(logger, "Realizado SET del registro %s con valor %s", registro, regAX);
	log_info(logger, "Realizado SET del registro %s con valor %s", registro, regBX);

}

void* leer_registro(char* registro1){
	char* registro_recibido=malloc(20);
	char* registro=strtok(registro1,"\n");
	log_error(logger,"registro : %s",registro);
	log_error(logger,"registro AX : %s",regAX);

	if(!strcmp(registro,"AX")){
		memcpy(registro_recibido, regAX, sizeof(char) * 4);
	    //parseo_registro(registro_recibido, pcb,4);
	}else if(!strcmp(registro,"BX")){

	    memcpy(registro_recibido, regBX, sizeof(char) * 4);
	    //parseo_registro(registro_recibido, pcb,4);
	}else if(!strcmp(registro,"CX")){

	    memcpy(registro_recibido, regCX, sizeof(char) * 4);
	    //parseo_registro(registro_recibido, pcb,4);
	}else if(!strcmp(registro,"DX")){

	    memcpy(registro_recibido, regDX, sizeof(char) * 4);
	    //parseo_registro(registro_recibido, pcb,4);
	}else if(!strcmp(registro,"EAX")){
	    memcpy(registro_recibido, regEAX, sizeof(char) * 8);
	    //parseo_registro(registro_recibido, pcb,8);
	}else if(!strcmp(registro,"EBX")){
	    memcpy(registro_recibido, regEBX, sizeof(char) * 8);
	    //parseo_registro(registro_recibido, pcb,8);
	}else if(!strcmp(registro,"ECX")){
	    memcpy(registro_recibido, regECX, sizeof(char) * 8);
	    //parseo_registro(registro_recibido, pcb,8);
	}else if(!strcmp(registro,"EDX")){
	    memcpy(registro_recibido, regEDX, sizeof(char) * 8);
	    //parseo_registro(registro_recibido, pcb,16);
	}else if(!strcmp(registro,"RAX")){
	    memcpy(registro_recibido, regRAX, sizeof(char) * 16);
	    //parseo_registro(registro_recibido, pcb,16);
	}else if(!strcmp(registro,"RBX")){
	    memcpy(registro_recibido, regRBX, sizeof(char) * 16);
	    //parseo_registro(registro_recibido, pcb,16);
	}else if(!strcmp(registro,"RCX")){
	    memcpy(registro_recibido, regRCX, sizeof(char) * 16);
	    //parseo_registro(registro_recibido, pcb,16);
	}else if(!strcmp(registro,"RDX")){
	    memcpy(registro_recibido, regRDX, sizeof(char) * 16);
	    //parseo_registro(registro_recibido, pcb,16);
	}else{
		log_error(logger, "me fui al else");
	}


	log_info(logger, "Realizado lectura del registro %s con valor %s", registro, registro_recibido);
	return registro_recibido;
}
/*void parseo_registro(char registro_recibido[20], PCB_t* pcb,int tamanio){
	//log_warning(logger,"Registro recibido: %s - tamaño: %d: ",strtok(registro_recibido,"\0"), tamanio);
		int i = 0;
		char reg[tamanio];

		while( i < tamanio)
		{
			reg[i] = registro_recibido[i];
			i++;
		}
		log_error(logger, "reg: %s", reg);
		strcpy(pcb->registro_cpu->ax,reg);
		log_warning(logger,"Valor en el registro ax: %s", pcb->registro_cpu->ax);
}*/

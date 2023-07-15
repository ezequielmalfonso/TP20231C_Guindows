#include "mmu.h"


uint32_t num_seg(int logica){

	return logica/configuracion->TAM_MAX_SEGMENTO;
}
uint32_t desplazamiento(int logica){
	return logica%configuracion->TAM_MAX_SEGMENTO;
}

int mov_in(char direccion_logica[20], char registro[20], PCB_t* pcb){
	uint32_t desplazamiento1 = desplazamiento(atoi(direccion_logica));
	uint32_t n_segmento = num_seg(atoi(direccion_logica));

	int tamanio = calcularTam(registro);
	//log_error(logger, "(MOV_IN) PID: %d -TAM REG: %d",pcb->pid, tamanio);

	if(checkSegmentetitonFault(desplazamiento1+tamanio, n_segmento,pcb)){
		log_error(logger, "PID: %d - SEGMENTATION FAULT", pcb->pid);
		//op_code codigo = SEGMENTATION_FAULT;
		//send_proceso(cliente_socket,pcb,codigo);
		return 0;

	}else{
	char* registro_recibido= malloc(20);
	send_pedido_memoria(memoria_fd,n_segmento,desplazamiento1,(pcb->pid),tamanio, MOV_IN);
	recv(memoria_fd, registro_recibido ,tamanio, MSG_WAITALL);


	//memcpy(registro_recibido+4,"\n",2);
		//log_info(logger,"estamos recibiendo de memoria: %s", registro_recibido);
	//log_info(logger,"estamos seteando de memoria: %d", tamanio);
	//log_warning(logger,"Antes de setear registro: Regsitro: %s - Reg_recibido:%s", registro, registro_recibido);
	set_registro(registro, registro_recibido, pcb);
	//log_error(logger,"Dps de setear registro");

	uint64_t base=0;
	uint64_t direccion_fisica=0;
	send_escribir_memoria(memoria_fd, n_segmento, 0, pcb->pid, "", 0, BASE);
	recv(memoria_fd, &base, sizeof(uint64_t), MSG_WAITALL);
	//log_error(logger,"(MOV_IN)Base rec desde memo: %d", base);
	direccion_fisica = desplazamiento1+base;
	log_info(logger, "PID: %d - Acción: LEER - Segmento: %d - Dirección Física: %d - Valor: %s", pcb->pid, n_segmento, direccion_fisica, registro_recibido);

//	log_info(logger,"PID: %d - Ejecutando SET parametro 1: %s parametro 2: %s", pid,instruccion_ejecutar->parametro1,instruccion_ejecutar->parametro2);
//	bool recv_instruccion(memoria_fd, tam, respuestaMemoria, char* param3);
//  enviar pedido a memoria TODO
	return 1;
	}
}
int mov_out(char* direccion_logica, char* registro,PCB_t* pcb){
	uint32_t desplazamiento1 = desplazamiento(atoi(direccion_logica));
		uint32_t n_segmento = num_seg(atoi(direccion_logica));
		int tamanio = calcularTam(registro);
		//log_warning(logger, "Tamanio registro: %d", tamanio );
		char* escribir = malloc(tamanio+1);
		memcpy(escribir,leer_registro(registro,tamanio),tamanio);
		//escribir[tamanio-1]='\0';
		//log_info(logger, "estamos mandando a memoria: %s", escribir);
		//log_error(logger, "(MOV_OUT)PID: %d - TAM REG: %d", pcb->pid, tamanio);
		if(checkSegmentetitonFault(desplazamiento1+tamanio, n_segmento,pcb)){
			log_error(logger, "PID: %d - SEGMENTATION FAULT", pcb->pid);
			//op_code codigo = SEGMENTATION_FAULT;
			//send_proceso(cliente_socket,pcb,codigo);
			return 0;

		}else{

		uint64_t base=0;
		uint64_t direccion_fisica=0;
		//log_warning(logger, "Antes del send a memo");
		send_escribir_memoria(memoria_fd, n_segmento, 0, pcb->pid, "", 0, BASE);
		recv(memoria_fd, base, sizeof(uint64_t), MSG_WAITALL);
		//log_error(logger,"(MOV_OUT)Base rec desde memo: %d", base);

		direccion_fisica = desplazamiento1+base;
		log_info(logger, "PID: %d - Acción: ESCRIBIR - Segmento: %d - Dirección Física: %d - Valor: %s", pcb->pid, n_segmento, direccion_fisica, escribir);

		send_escribir_memoria(memoria_fd,n_segmento,desplazamiento1,(pcb->pid),escribir,tamanio+1,MOV_OUT);
		op_code cop;
		recv(memoria_fd, &cop, sizeof(cop), 0);
		//log_info(logger,"se escribio en memoria:  %d",cop);
		//enviar pedido a memoria TODO
		free(escribir);
		return 1;
		}
}

int checkSegmentetitonFault(uint32_t desplazamiento, uint32_t n_segmento,PCB_t* pcb){
	t_segmento* segmentoN;
	int i;
	for(i = 0;i<list_size(pcb->tabla_de_segmentos);i++){
		segmentoN = list_get(pcb->tabla_de_segmentos,i);
		if(segmentoN->id_segmento == n_segmento)
			break;
	}
	//log_warning(logger,"DESP: %d - TAM SEG: %d",desplazamiento, segmentoN->tamanio_segmento );
	return desplazamiento > segmentoN->tamanio_segmento;
}

int calcularTam(char* registro){
	//log_warning(logger, "Registo para calcualr tamaño: %s" , registro );
	switch(registro[0]){
	case 'A': //log_warning(logger, "Tamanio registro A: ");
			  return 4;
	case 'B': //log_warning(logger, "Tamanio registro B: ");
			  return 4;
	case 'C': //log_warning(logger, "Tamanio registro C: ");
			  return 4;
	case 'D': //log_warning(logger, "Tamanio registro D: ");
			  return 4;
	case 'E':
		      //log_warning(logger, "Tamanio registro E: ");
			  return 8;
	case 'R':
		      //log_warning(logger, "Tamanio registro R: ");
		      return 16;
	default: log_error(logger, "FALLO AL CALCULAR TAMAÑO!!!!");
	         return 0;
   }

}

void set_registro(char* registro1,char* registro_recibido, PCB_t* pcb){
	//log_warning(logger,"Registro: %s", registro_recibido);
	//pcb->registro_cpu = malloc(sizeof(registros_t));
	//log_info(logger,"registro recibido: %s", registro_recibido);
	char* registro = strtok(registro1,"\n");
	registro_recibido = strtok(registro_recibido,"\n");
	if(!strcmp(registro,"AX")){
		memcpy(regAX, registro_recibido, sizeof(char) * 5);
		//log_warning(logger, "Contenido registro AX: %.4s", regAX);
	}else if(!strcmp(registro,"BX")){
		memcpy(regBX, registro_recibido, sizeof(char) * 5);
		//log_warning(logger, "Contenido registro BX: %.4s", regBX);
	}else if(!strcmp(registro,"CX")){
		memcpy(regCX, registro_recibido, sizeof(char) * 5);
		//log_warning(logger, "Contenido registro CX: %.4s", regCX);
	}else if(!strcmp(registro,"DX")){
		memcpy(regDX, registro_recibido, sizeof(char) * 5);
		//log_warning(logger, "Contenido registro DX: %.4s", regDX);
	}else if(!strcmp(registro,"EAX")){
		memcpy(regEAX, registro_recibido, sizeof(char) * 9);
		//log_warning(logger, "Contenido registro EAX: %.8s", regEAX);
	}else if(!strcmp(registro,"EBX")){
		memcpy(regEBX, registro_recibido, sizeof(char) * 9);
		//log_warning(logger, "Contenido registro EBX: %.8s", regEBX);
	}else if(!strcmp(registro,"ECX")){
		memcpy(regECX, registro_recibido, sizeof(char) * 9);
		//log_warning(logger, "Contenido registro ECX: %.8s", regECX);
	}else if(!strcmp(registro,"EDX")){
		memcpy(regEDX, registro_recibido, sizeof(char) * 9);
		//log_warning(logger, "Contenido registro EDX: %.8s", regEDX);
	}else if(!strcmp(registro,"RAX")){
		memcpy(regRAX, registro_recibido, sizeof(char) * 17);
		//log_warning(logger, "Contenido registro RAX: %.16s", regRAX);
	}else if(!strcmp(registro,"RBX")){
		memcpy(regRBX, registro_recibido, sizeof(char) * 17);
		//log_warning(logger, "Contenido registro RBX: %.16s", regRBX);
	}else if(!strcmp(registro,"RCX")){
		memcpy(regRCX, registro_recibido, sizeof(char) * 17);
		//log_warning(logger, "Contenido registro RCX: %.16s", regRCX);
	}else if(!strcmp(registro,"RDX")){
		memcpy(regRDX, registro_recibido, sizeof(char) * 17);
		//log_warning(logger, "Contenido registro RDX: %.16s", regRDX);
	}else{
		log_error(logger,"me fui al else del set");
	}
	//log_info(logger, "Realizado SET del registro %s con valor %s", registro, regAX);
	//log_info(logger, "Realizado SET del registro %s con valor %s", registro, regBX);

}

void* leer_registro(char* registro1, int tamanio){
	char* registro_recibido=malloc(20);
	char* registro=strtok(registro1,"\n");
	//log_error(logger,"registro : %s - Tamanio: %d",registro, tamanio);
	//log_error(logger,"registro AX : %s",regAX);

	if(!strcmp(registro,"AX")){   //log_error(logger,"registro  : %s",regAX);
		memcpy(registro_recibido, regAX, tamanio);
		//registro_recibido[tamanio] = '\0';
	    //log_error(logger,"Valor reg recibiod: %s",registro_recibido);
	}else if(!strcmp(registro,"BX")){ //log_error(logger,"registro  : %s",regBX);
	    memcpy(registro_recibido, regBX, tamanio);
	    //registro_recibido[tamanio] = '\0';
	}else if(!strcmp(registro,"CX")){ //log_error(logger,"registro  : %s",regCX);
	    memcpy(registro_recibido, regCX, tamanio);
	    //registro_recibido[tamanio] = '\0';
	}else if(!strcmp(registro,"DX")){ //log_error(logger,"registro  : %s",regDX);
	    memcpy(registro_recibido, regDX, tamanio);
	    //parseo_registro(registro_recibido, pcb,4);
	}else if(!strcmp(registro,"EAX")){ //log_error(logger,"registro  : %s",regEAX);
	    memcpy(registro_recibido, regEAX, tamanio);
	   // log_error(logger,"Valor reg recibiod: %s",registro_recibido);
	    //parseo_registro(registro_recibido, pcb,8);
	}else if(!strcmp(registro,"EBX")){ //log_error(logger,"registro  : %s",regEBX);
	    memcpy(registro_recibido, regEBX, tamanio);
	   // log_error(logger,"Valor reg recibiod: %s",registro_recibido);

	}else if(!strcmp(registro,"ECX")){ //log_error(logger,"registro  : %s",regECX);
	    memcpy(registro_recibido, regECX, tamanio);
	    //parseo_registro(registro_recibido, pcb,8);
	}else if(!strcmp(registro,"EDX")){ //log_error(logger,"registro  : %s",regEDX);
	    memcpy(registro_recibido, regEDX, tamanio);
	    //parseo_registro(registro_recibido, pcb,16);
	}else if(!strcmp(registro,"RAX")){ //log_error(logger,"registro  : %s",regRAX);
	    memcpy(registro_recibido, regRAX, tamanio );
	    //parseo_registro(registro_recibido, pcb,16);
	}else if(!strcmp(registro,"RBX")){ //log_error(logger,"registro  : %s",regRBX);
	    memcpy(registro_recibido, regRBX, tamanio);
	    //parseo_registro(registro_recibido, pcb,16);
	}else if(!strcmp(registro,"RCX")){ //log_error(logger,"registro  : %s",regRCX);
	    memcpy(registro_recibido, regRCX, tamanio);
	    //parseo_registro(registro_recibido, pcb,16);
	}else if(!strcmp(registro,"RDX")){ //log_error(logger,"registro  : %s",regRDX);
	    memcpy(registro_recibido, regRDX, tamanio);
	    //parseo_registro(registro_recibido, pcb,16);
	}else{
		log_error(logger, "me fui al else");
	}

	log_info(logger, "Realizado lectura del registro %s con valor %s", registro, registro_recibido);
	return registro_recibido;
}

// PARA FILESYSTEM

void* traducirAFisica(void* direccion_logica, PCB_t* pcb, int tamanio,char* valor, bool leer){
	uint32_t desplazamiento1 = desplazamiento(direccion_logica);
	uint32_t n_segmento = num_seg(direccion_logica);
	uint64_t base;

	send_escribir_memoria(memoria_fd, n_segmento, 0, pcb->pid, "", 0, BASE);
	recv(memoria_fd, &base, sizeof(uint64_t), MSG_WAITALL);

	//log_error(logger,"(FS)Base rec desde memo: %d - Desplazamiento: %d", base, desplazamiento1);

	uint64_t direccion_fisica = (uint64_t)desplazamiento1+base;
/*
	if(leer){
		log_info(logger, "PID: %d - Acción: LEER - Segmento: %d - Dirección Física: %d - Valor: %s", pcb->pid, n_segmento, direccion_fisica, valor);
	}else{
		log_info(logger, "PID: %d - Acción: ESCRIBIR - Segmento: %d - Dirección Física: %d - Valor: %s", pcb->pid, n_segmento, direccion_fisica, valor);
	}*/

	if(checkSegmentetitonFault(desplazamiento1+tamanio, n_segmento,pcb)){
		log_error(logger, "PID: %d - SEGMENTATION FAULT", pcb->pid);
					//op_code codigo = SEGMENTATION_FAULT;
					//send_proceso(cliente_socket,pcb,codigo);
					return "0";

	}else{
	 	//log_error(logger, "Desp: %d - Seg: %d", desplazamiento1, n_segmento );

	      return string_from_format("%dx%dx%dx%d", n_segmento,desplazamiento1, pcb->pid, base);
	}


}



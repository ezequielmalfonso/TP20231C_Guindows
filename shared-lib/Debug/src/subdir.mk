################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/pcb.c \
../src/protocolo.c \
../src/socket.c 

C_DEPS += \
./src/pcb.d \
./src/protocolo.d \
./src/socket.d 

OBJS += \
./src/pcb.o \
./src/protocolo.o \
./src/socket.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/pcb.d ./src/pcb.o ./src/protocolo.d ./src/protocolo.o ./src/socket.d ./src/socket.o

.PHONY: clean-src


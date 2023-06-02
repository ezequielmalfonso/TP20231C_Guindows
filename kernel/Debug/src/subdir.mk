################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/kernel.c \
../src/kernelComunicacion.c \
../src/kernelConfig.c \
../src/planificacion.c 

C_DEPS += \
./src/kernel.d \
./src/kernelComunicacion.d \
./src/kernelConfig.d \
./src/planificacion.d 

OBJS += \
./src/kernel.o \
./src/kernelComunicacion.o \
./src/kernelConfig.o \
./src/planificacion.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2023-1c-Guindows/shared-lib/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/kernel.d ./src/kernel.o ./src/kernelComunicacion.d ./src/kernelComunicacion.o ./src/kernelConfig.d ./src/kernelConfig.o ./src/planificacion.d ./src/planificacion.o

.PHONY: clean-src


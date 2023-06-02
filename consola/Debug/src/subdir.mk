################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/consola.c \
../src/consolaComunicacion.c \
../src/consolaConfig.c 

C_DEPS += \
./src/consola.d \
./src/consolaComunicacion.d \
./src/consolaConfig.d 

OBJS += \
./src/consola.o \
./src/consolaComunicacion.o \
./src/consolaConfig.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2023-1c-Guindows/shared-lib/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/consola.d ./src/consola.o ./src/consolaComunicacion.d ./src/consolaComunicacion.o ./src/consolaConfig.d ./src/consolaConfig.o

.PHONY: clean-src


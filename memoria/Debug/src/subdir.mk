################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/memoria.c \
../src/memoriaComunicacion.c \
../src/memoriaConfig.c \
../src/memoria_utils.c 

C_DEPS += \
./src/memoria.d \
./src/memoriaComunicacion.d \
./src/memoriaConfig.d \
./src/memoria_utils.d 

OBJS += \
./src/memoria.o \
./src/memoriaComunicacion.o \
./src/memoriaConfig.o \
./src/memoria_utils.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2023-1c-Guindows/shared-lib/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/memoria.d ./src/memoria.o ./src/memoriaComunicacion.d ./src/memoriaComunicacion.o ./src/memoriaConfig.d ./src/memoriaConfig.o ./src/memoria_utils.d ./src/memoria_utils.o

.PHONY: clean-src


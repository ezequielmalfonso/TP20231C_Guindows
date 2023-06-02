################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/cpu.c \
../src/cpuComunicacion.c \
../src/cpuConfig.c 

C_DEPS += \
./src/cpu.d \
./src/cpuComunicacion.d \
./src/cpuConfig.d 

OBJS += \
./src/cpu.o \
./src/cpuComunicacion.o \
./src/cpuConfig.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2023-1c-Guindows/shared-lib/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/cpu.d ./src/cpu.o ./src/cpuComunicacion.d ./src/cpuComunicacion.o ./src/cpuConfig.d ./src/cpuConfig.o

.PHONY: clean-src


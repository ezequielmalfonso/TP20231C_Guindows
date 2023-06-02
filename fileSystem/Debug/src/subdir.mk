################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/fileSystem.c \
../src/fileSystemComunicacion.c \
../src/fileSystemConfig.c 

C_DEPS += \
./src/fileSystem.d \
./src/fileSystemComunicacion.d \
./src/fileSystemConfig.d 

OBJS += \
./src/fileSystem.o \
./src/fileSystemComunicacion.o \
./src/fileSystemConfig.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2023-1c-Guindows/shared-lib/src" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/fileSystem.d ./src/fileSystem.o ./src/fileSystemComunicacion.d ./src/fileSystemComunicacion.o ./src/fileSystemConfig.d ./src/fileSystemConfig.o

.PHONY: clean-src


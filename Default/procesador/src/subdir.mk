################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../procesador/src/procesador.c 

OBJS += \
./procesador/src/procesador.o 

C_DEPS += \
./procesador/src/procesador.d 


# Each subdirectory must supply rules for building sources it contributes
procesador/src/%.o: ../procesador/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



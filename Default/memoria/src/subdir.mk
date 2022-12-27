################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../memoria/src/memoria.c \
../memoria/src/utils.c 

OBJS += \
./memoria/src/memoria.o \
./memoria/src/utils.o 

C_DEPS += \
./memoria/src/memoria.d \
./memoria/src/utils.d 


# Each subdirectory must supply rules for building sources it contributes
memoria/src/%.o: ../memoria/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



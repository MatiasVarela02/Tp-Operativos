################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../terminal/src/terminal.c \
../terminal/src/utils.c 

OBJS += \
./terminal/src/terminal.o \
./terminal/src/utils.o 

C_DEPS += \
./terminal/src/terminal.d \
./terminal/src/utils.d 


# Each subdirectory must supply rules for building sources it contributes
terminal/src/%.o: ../terminal/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



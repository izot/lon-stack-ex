################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FtxlDev.c \
../FtxlHandlers.c \
../FtxlNvdFlashFs.c \
../Main.c 

OBJS += \
./FtxlDev.o \
./FtxlHandlers.o \
./FtxlNvdFlashFs.o \
./Main.o 

C_DEPS += \
./FtxlDev.d \
./FtxlHandlers.d \
./FtxlNvdFlashFs.d \
./Main.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -DIZOT_PLATFORM -D_DEBUG -I../../../Source -I../../../Templates -I../../../Source/Shared/include -I../../../Source/VxLayer/include -O0 -g3 -Wall -c -fmessage-length=0 -pthread -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



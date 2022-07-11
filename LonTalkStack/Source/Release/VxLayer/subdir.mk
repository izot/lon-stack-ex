################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../VxLayer/PosixOsal.c \
../VxLayer/VxLQues.c \
../VxLayer/VxLayer.c \
../VxLayer/VxMsgQ.c \
../VxLayer/VxSemaph.c \
../VxLayer/VxTimers.c 

OBJS += \
./VxLayer/PosixOsal.o \
./VxLayer/VxLQues.o \
./VxLayer/VxLayer.o \
./VxLayer/VxMsgQ.o \
./VxLayer/VxSemaph.o \
./VxLayer/VxTimers.o 

C_DEPS += \
./VxLayer/PosixOsal.d \
./VxLayer/VxLQues.d \
./VxLayer/VxLayer.d \
./VxLayer/VxMsgQ.d \
./VxLayer/VxSemaph.d \
./VxLayer/VxTimers.d 


# Each subdirectory must supply rules for building sources it contributes
VxLayer/%.o: ../VxLayer/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -DIZOT_PLATFORM -DNDEBUG -I../LonLinkIzoT/include -I../Lre/include -I../Shared/include -I../ShareIp/include -I../Stack/include -I../VxLayer/include -I../Target/include -I../Target/Drivers/Linux/SMIP/include -I../../Templates -I../FtxlApi/include -I../XMLParser -I../Pa/include -O3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



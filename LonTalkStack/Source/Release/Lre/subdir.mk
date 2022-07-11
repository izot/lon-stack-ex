################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Lre/LtIpPortClient.cpp \
../Lre/LtLre.cpp 

OBJS += \
./Lre/LtIpPortClient.o \
./Lre/LtLre.o 

CPP_DEPS += \
./Lre/LtIpPortClient.d \
./Lre/LtLre.d 


# Each subdirectory must supply rules for building sources it contributes
Lre/%.o: ../Lre/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	arm-linux-gnueabihf-g++ -DIZOT_PLATFORM -DNDEBUG -I../LonLinkIzoT/include -I../Lre/include -I../Shared/include -I../ShareIp/include -I../Stack/include -I../VxLayer/include -I../Target/include -I../Target/Drivers/Linux/SMIP/include -I../../Templates -I../FtxlApi/include -I../XMLParser -I../Pa/include -O3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../LonLinkIzoT/IzoTDevSocketMaps.cpp \
../LonLinkIzoT/IzoTLsIpMapping.cpp \
../LonLinkIzoT/IzoTNiConfig.cpp \
../LonLinkIzoT/LonLinkIzoT.cpp \
../LonLinkIzoT/LonLinkIzoTDev.cpp \
../LonLinkIzoT/LonLinkIzoTLtLink.cpp \
../LonLinkIzoT/LonLinkIzoTRNIEthLink.cpp \
../LonLinkIzoT/LonLinkIzoTRNILtLink.cpp 

C_SRCS += \
../LonLinkIzoT/ipv6_ls_to_udp.c 

OBJS += \
./LonLinkIzoT/IzoTDevSocketMaps.o \
./LonLinkIzoT/IzoTLsIpMapping.o \
./LonLinkIzoT/IzoTNiConfig.o \
./LonLinkIzoT/LonLinkIzoT.o \
./LonLinkIzoT/LonLinkIzoTDev.o \
./LonLinkIzoT/LonLinkIzoTLtLink.o \
./LonLinkIzoT/LonLinkIzoTRNIEthLink.o \
./LonLinkIzoT/LonLinkIzoTRNILtLink.o \
./LonLinkIzoT/ipv6_ls_to_udp.o 

C_DEPS += \
./LonLinkIzoT/ipv6_ls_to_udp.d 

CPP_DEPS += \
./LonLinkIzoT/IzoTDevSocketMaps.d \
./LonLinkIzoT/IzoTLsIpMapping.d \
./LonLinkIzoT/IzoTNiConfig.d \
./LonLinkIzoT/LonLinkIzoT.d \
./LonLinkIzoT/LonLinkIzoTDev.d \
./LonLinkIzoT/LonLinkIzoTLtLink.d \
./LonLinkIzoT/LonLinkIzoTRNIEthLink.d \
./LonLinkIzoT/LonLinkIzoTRNILtLink.d 


# Each subdirectory must supply rules for building sources it contributes
LonLinkIzoT/%.o: ../LonLinkIzoT/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DIZOT_PLATFORM -DLINUX32_GCC=1 -D_DEBUG -I../LonLinkIzoT/include -I../Pa/include -I../Lre/include -I../Shared/include -I../ShareIp/include -I../Stack/include -I../VxLayer/include -I../Target/include -I../Target/Drivers/Linux/SMIP/include -I../../Templates -I../FtxlApi/include -I../XMLParser -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

LonLinkIzoT/%.o: ../LonLinkIzoT/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -DIZOT_PLATFORM -DLINUX32_GCC=1 -D_DEBUG -I../LonLinkIzoT/include -I../Pa/include -I../Lre/include -I../Shared/include -I../ShareIp/include -I../Stack/include -I../VxLayer/include -I../Target/include -I../Target/Drivers/Linux/SMIP/include -I../../Templates -I../FtxlApi/include -I../XMLParser -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



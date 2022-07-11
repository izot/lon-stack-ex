################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../ShareIp/IpLink.cpp \
../ShareIp/LtIpBase.cpp \
../ShareIp/LtIpChannel.cpp \
../ShareIp/LtIpEchPackets.cpp \
../ShareIp/LtIpMaster.cpp \
../ShareIp/LtIpPackets.cpp \
../ShareIp/LtIpPersist.cpp \
../ShareIp/LtIpPlatform.cpp \
../ShareIp/LtLreIpClient.cpp \
../ShareIp/LtMD5.cpp \
../ShareIp/LtPktReorderQue.cpp \
../ShareIp/SegSupport.cpp \
../ShareIp/Segment.cpp \
../ShareIp/iLonSntp.cpp 

C_SRCS += \
../ShareIp/VxSockets.c \
../ShareIp/md5c.c \
../ShareIp/sntpcLib.c \
../ShareIp/vxlTarget.c 

OBJS += \
./ShareIp/IpLink.o \
./ShareIp/LtIpBase.o \
./ShareIp/LtIpChannel.o \
./ShareIp/LtIpEchPackets.o \
./ShareIp/LtIpMaster.o \
./ShareIp/LtIpPackets.o \
./ShareIp/LtIpPersist.o \
./ShareIp/LtIpPlatform.o \
./ShareIp/LtLreIpClient.o \
./ShareIp/LtMD5.o \
./ShareIp/LtPktReorderQue.o \
./ShareIp/SegSupport.o \
./ShareIp/Segment.o \
./ShareIp/VxSockets.o \
./ShareIp/iLonSntp.o \
./ShareIp/md5c.o \
./ShareIp/sntpcLib.o \
./ShareIp/vxlTarget.o 

C_DEPS += \
./ShareIp/VxSockets.d \
./ShareIp/md5c.d \
./ShareIp/sntpcLib.d \
./ShareIp/vxlTarget.d 

CPP_DEPS += \
./ShareIp/IpLink.d \
./ShareIp/LtIpBase.d \
./ShareIp/LtIpChannel.d \
./ShareIp/LtIpEchPackets.d \
./ShareIp/LtIpMaster.d \
./ShareIp/LtIpPackets.d \
./ShareIp/LtIpPersist.d \
./ShareIp/LtIpPlatform.d \
./ShareIp/LtLreIpClient.d \
./ShareIp/LtMD5.d \
./ShareIp/LtPktReorderQue.d \
./ShareIp/SegSupport.d \
./ShareIp/Segment.d \
./ShareIp/iLonSntp.d 


# Each subdirectory must supply rules for building sources it contributes
ShareIp/%.o: ../ShareIp/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	arm-linux-gnueabihf-g++ -DIZOT_PLATFORM -D_DEBUG -I../LonLinkIzoT/include -I../Lre/include -I../Shared/include -I../ShareIp/include -I../Stack/include -I../VxLayer/include -I../Target/include -I../Target/Drivers/Linux/SMIP/include -I../../Templates -I../FtxlApi/include -I../XMLParser -I../Pa/include -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

ShareIp/%.o: ../ShareIp/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -DIZOT_PLATFORM -D_DEBUG -I../LonLinkIzoT/include -I../Lre/include -I../Shared/include -I../ShareIp/include -I../Stack/include -I../VxLayer/include -I../Target/include -I../Target/Drivers/Linux/SMIP/include -I../../Templates -I../FtxlApi/include -I../XMLParser -I../Pa/include -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Shared/LonLink.cpp \
../Shared/LtBlob.cpp \
../Shared/LtChannel.cpp \
../Shared/LtDomain.cpp \
../Shared/LtFailSafeFile.cpp \
../Shared/LtHashTable.cpp \
../Shared/LtLinkBase.cpp \
../Shared/LtNetwork.cpp \
../Shared/LtNvRam.cpp \
../Shared/LtPersistence.cpp \
../Shared/LtPktAllocator.cpp \
../Shared/LtPktAllocatorOne.cpp \
../Shared/LtPktInfo.cpp \
../Shared/LtProgramId.cpp \
../Shared/LtTaskOwner.cpp \
../Shared/LtUniqueId.cpp \
../Shared/LtUri.cpp \
../Shared/LtVector.cpp \
../Shared/RefQues.cpp 

C_SRCS += \
../Shared/LtCUtil.c \
../Shared/fdelt_chk.c 

OBJS += \
./Shared/LonLink.o \
./Shared/LtBlob.o \
./Shared/LtCUtil.o \
./Shared/LtChannel.o \
./Shared/LtDomain.o \
./Shared/LtFailSafeFile.o \
./Shared/LtHashTable.o \
./Shared/LtLinkBase.o \
./Shared/LtNetwork.o \
./Shared/LtNvRam.o \
./Shared/LtPersistence.o \
./Shared/LtPktAllocator.o \
./Shared/LtPktAllocatorOne.o \
./Shared/LtPktInfo.o \
./Shared/LtProgramId.o \
./Shared/LtTaskOwner.o \
./Shared/LtUniqueId.o \
./Shared/LtUri.o \
./Shared/LtVector.o \
./Shared/RefQues.o \
./Shared/fdelt_chk.o 

C_DEPS += \
./Shared/LtCUtil.d \
./Shared/fdelt_chk.d 

CPP_DEPS += \
./Shared/LonLink.d \
./Shared/LtBlob.d \
./Shared/LtChannel.d \
./Shared/LtDomain.d \
./Shared/LtFailSafeFile.d \
./Shared/LtHashTable.d \
./Shared/LtLinkBase.d \
./Shared/LtNetwork.d \
./Shared/LtNvRam.d \
./Shared/LtPersistence.d \
./Shared/LtPktAllocator.d \
./Shared/LtPktAllocatorOne.d \
./Shared/LtPktInfo.d \
./Shared/LtProgramId.d \
./Shared/LtTaskOwner.d \
./Shared/LtUniqueId.d \
./Shared/LtUri.d \
./Shared/LtVector.d \
./Shared/RefQues.d 


# Each subdirectory must supply rules for building sources it contributes
Shared/%.o: ../Shared/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	arm-linux-gnueabihf-g++ -DIZOT_PLATFORM -D_DEBUG -I../LonLinkIzoT/include -I../Lre/include -I../Shared/include -I../ShareIp/include -I../Stack/include -I../VxLayer/include -I../Target/include -I../Target/Drivers/Linux/SMIP/include -I../../Templates -I../FtxlApi/include -I../XMLParser -I../Pa/include -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Shared/%.o: ../Shared/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	arm-linux-gnueabihf-gcc -DIZOT_PLATFORM -D_DEBUG -I../LonLinkIzoT/include -I../Lre/include -I../Shared/include -I../ShareIp/include -I../Stack/include -I../VxLayer/include -I../Target/include -I../Target/Drivers/Linux/SMIP/include -I../../Templates -I../FtxlApi/include -I../XMLParser -I../Pa/include -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



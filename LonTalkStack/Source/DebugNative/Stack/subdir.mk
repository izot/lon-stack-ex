################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Stack/DynamicNvs.cpp \
../Stack/LonTalkNode.cpp \
../Stack/LonTalkStack.cpp \
../Stack/LtAddressConfiguration.cpp \
../Stack/LtAddressConfigurationTable.cpp \
../Stack/LtApdu.cpp \
../Stack/LtBitMap.cpp \
../Stack/LtConfigData.cpp \
../Stack/LtConfigurationEntity.cpp \
../Stack/LtDescription.cpp \
../Stack/LtDeviceStack.cpp \
../Stack/LtDomainConfiguration.cpp \
../Stack/LtDomainConfigurationTable.cpp \
../Stack/LtLayer4.cpp \
../Stack/LtLayer6.cpp \
../Stack/LtMip.cpp \
../Stack/LtMipApp.cpp \
../Stack/LtMsgOverride.cpp \
../Stack/LtNetworkImage.cpp \
../Stack/LtNetworkManager.cpp \
../Stack/LtNetworkManager2.cpp \
../Stack/LtNetworkStats.cpp \
../Stack/LtNetworkVariable.cpp \
../Stack/LtNetworkVariableConfiguration.cpp \
../Stack/LtNetworkVariableConfigurationTable.cpp \
../Stack/LtOutgoingAddress.cpp \
../Stack/LtPlatform.cpp \
../Stack/LtProxy.cpp \
../Stack/LtReadOnlyData.cpp \
../Stack/LtRouteMap.cpp \
../Stack/LtStackClient.cpp \
../Stack/LtStatus.cpp \
../Stack/LtTransactions.cpp \
../Stack/LtXcvrId.cpp \
../Stack/LtaBase.cpp \
../Stack/NdNetworkVariable.cpp \
../Stack/nodedef.cpp 

OBJS += \
./Stack/DynamicNvs.o \
./Stack/LonTalkNode.o \
./Stack/LonTalkStack.o \
./Stack/LtAddressConfiguration.o \
./Stack/LtAddressConfigurationTable.o \
./Stack/LtApdu.o \
./Stack/LtBitMap.o \
./Stack/LtConfigData.o \
./Stack/LtConfigurationEntity.o \
./Stack/LtDescription.o \
./Stack/LtDeviceStack.o \
./Stack/LtDomainConfiguration.o \
./Stack/LtDomainConfigurationTable.o \
./Stack/LtLayer4.o \
./Stack/LtLayer6.o \
./Stack/LtMip.o \
./Stack/LtMipApp.o \
./Stack/LtMsgOverride.o \
./Stack/LtNetworkImage.o \
./Stack/LtNetworkManager.o \
./Stack/LtNetworkManager2.o \
./Stack/LtNetworkStats.o \
./Stack/LtNetworkVariable.o \
./Stack/LtNetworkVariableConfiguration.o \
./Stack/LtNetworkVariableConfigurationTable.o \
./Stack/LtOutgoingAddress.o \
./Stack/LtPlatform.o \
./Stack/LtProxy.o \
./Stack/LtReadOnlyData.o \
./Stack/LtRouteMap.o \
./Stack/LtStackClient.o \
./Stack/LtStatus.o \
./Stack/LtTransactions.o \
./Stack/LtXcvrId.o \
./Stack/LtaBase.o \
./Stack/NdNetworkVariable.o \
./Stack/nodedef.o 

CPP_DEPS += \
./Stack/DynamicNvs.d \
./Stack/LonTalkNode.d \
./Stack/LonTalkStack.d \
./Stack/LtAddressConfiguration.d \
./Stack/LtAddressConfigurationTable.d \
./Stack/LtApdu.d \
./Stack/LtBitMap.d \
./Stack/LtConfigData.d \
./Stack/LtConfigurationEntity.d \
./Stack/LtDescription.d \
./Stack/LtDeviceStack.d \
./Stack/LtDomainConfiguration.d \
./Stack/LtDomainConfigurationTable.d \
./Stack/LtLayer4.d \
./Stack/LtLayer6.d \
./Stack/LtMip.d \
./Stack/LtMipApp.d \
./Stack/LtMsgOverride.d \
./Stack/LtNetworkImage.d \
./Stack/LtNetworkManager.d \
./Stack/LtNetworkManager2.d \
./Stack/LtNetworkStats.d \
./Stack/LtNetworkVariable.d \
./Stack/LtNetworkVariableConfiguration.d \
./Stack/LtNetworkVariableConfigurationTable.d \
./Stack/LtOutgoingAddress.d \
./Stack/LtPlatform.d \
./Stack/LtProxy.d \
./Stack/LtReadOnlyData.d \
./Stack/LtRouteMap.d \
./Stack/LtStackClient.d \
./Stack/LtStatus.d \
./Stack/LtTransactions.d \
./Stack/LtXcvrId.d \
./Stack/LtaBase.d \
./Stack/NdNetworkVariable.d \
./Stack/nodedef.d 


# Each subdirectory must supply rules for building sources it contributes
Stack/%.o: ../Stack/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -DIZOT_PLATFORM -DLINUX32_GCC=1 -D_DEBUG -I../LonLinkIzoT/include -I../Lre/include -I../Shared/include -I../ShareIp/include -I../Stack/include -I../VxLayer/include -I../Target/include -I../Target/Drivers/Linux/SMIP/include -I../../Templates -I../FtxlApi/include -I../XMLParser -I../Pa/include -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '



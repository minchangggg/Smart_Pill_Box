################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/DFPlayer.c \
../Core/Src/delay.c \
../Core/Src/ds1307_for_stm32_hal.c \
../Core/Src/fonts.c \
../Core/Src/main.c \
../Core/Src/sensirion_common.c \
../Core/Src/sensirion_i2c_hal.c \
../Core/Src/sht4x.c \
../Core/Src/sim.c \
../Core/Src/ssd1306.c \
../Core/Src/stm32f1xx_hal_msp.c \
../Core/Src/stm32f1xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32f1xx.c 

OBJS += \
./Core/Src/DFPlayer.o \
./Core/Src/delay.o \
./Core/Src/ds1307_for_stm32_hal.o \
./Core/Src/fonts.o \
./Core/Src/main.o \
./Core/Src/sensirion_common.o \
./Core/Src/sensirion_i2c_hal.o \
./Core/Src/sht4x.o \
./Core/Src/sim.o \
./Core/Src/ssd1306.o \
./Core/Src/stm32f1xx_hal_msp.o \
./Core/Src/stm32f1xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32f1xx.o 

C_DEPS += \
./Core/Src/DFPlayer.d \
./Core/Src/delay.d \
./Core/Src/ds1307_for_stm32_hal.d \
./Core/Src/fonts.d \
./Core/Src/main.d \
./Core/Src/sensirion_common.d \
./Core/Src/sensirion_i2c_hal.d \
./Core/Src/sht4x.d \
./Core/Src/sim.d \
./Core/Src/ssd1306.d \
./Core/Src/stm32f1xx_hal_msp.d \
./Core/Src/stm32f1xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32f1xx.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/DFPlayer.cyclo ./Core/Src/DFPlayer.d ./Core/Src/DFPlayer.o ./Core/Src/DFPlayer.su ./Core/Src/delay.cyclo ./Core/Src/delay.d ./Core/Src/delay.o ./Core/Src/delay.su ./Core/Src/ds1307_for_stm32_hal.cyclo ./Core/Src/ds1307_for_stm32_hal.d ./Core/Src/ds1307_for_stm32_hal.o ./Core/Src/ds1307_for_stm32_hal.su ./Core/Src/fonts.cyclo ./Core/Src/fonts.d ./Core/Src/fonts.o ./Core/Src/fonts.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/sensirion_common.cyclo ./Core/Src/sensirion_common.d ./Core/Src/sensirion_common.o ./Core/Src/sensirion_common.su ./Core/Src/sensirion_i2c_hal.cyclo ./Core/Src/sensirion_i2c_hal.d ./Core/Src/sensirion_i2c_hal.o ./Core/Src/sensirion_i2c_hal.su ./Core/Src/sht4x.cyclo ./Core/Src/sht4x.d ./Core/Src/sht4x.o ./Core/Src/sht4x.su ./Core/Src/sim.cyclo ./Core/Src/sim.d ./Core/Src/sim.o ./Core/Src/sim.su ./Core/Src/ssd1306.cyclo ./Core/Src/ssd1306.d ./Core/Src/ssd1306.o ./Core/Src/ssd1306.su ./Core/Src/stm32f1xx_hal_msp.cyclo ./Core/Src/stm32f1xx_hal_msp.d ./Core/Src/stm32f1xx_hal_msp.o ./Core/Src/stm32f1xx_hal_msp.su ./Core/Src/stm32f1xx_it.cyclo ./Core/Src/stm32f1xx_it.d ./Core/Src/stm32f1xx_it.o ./Core/Src/stm32f1xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32f1xx.cyclo ./Core/Src/system_stm32f1xx.d ./Core/Src/system_stm32f1xx.o ./Core/Src/system_stm32f1xx.su

.PHONY: clean-Core-2f-Src


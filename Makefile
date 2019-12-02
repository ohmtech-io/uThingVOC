######################################
# target
######################################
TARGET = USBthingVOC

######################################
# building variables
######################################
# debug build?
DEBUG := 0
# optimization (s=size, g=debug)
# OPT = -Os
OPT = -Os

#######################################
# paths
#######################################
# Build path
BUILD_DIR = build

######################################
# source
######################################
# C sources
C_SOURCES =  \
Src/main.c \
Src/usb_device.c \
Src/usbd_conf.c \
Src/usbd_desc.c \
Src/usbd_cdc_if.c \
Src/stm32f0xx_it.c \
Src/stm32f0xx_hal_msp.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_pcd.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_pcd_ex.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_i2c.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_i2c_ex.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_tim.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_tim_ex.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_rcc.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_rcc_ex.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_gpio.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_dma.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_cortex.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_pwr.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_pwr_ex.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_flash.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_flash_ex.c \
Src/system_stm32f0xx.c \
Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c \
Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c \
Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c \
Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_uart.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_uart_ex.c \
Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_iwdg.c \
Drivers/BME680_driver/bme680.c \
Drivers/BME680_driver/SelfTest/bme680_selftest.c \
Src/syscalls.c \
Src/thConfig.c \
Src/thBsec.c \
Middlewares/Bosch/bsec_serialized_configurations_iaq.c


# ASM sources
ASM_SOURCES =  \
startup_stm32f072xb.s

#######################################
# binaries
#######################################
GCC_PATH? = /opt/gcc-arm-none-eabi-8-2018-q4-major/bin

PREFIX = arm-none-eabi-
# The gcc compiler bin path can be either defined in make command via GCC_PATH variable (> make GCC_PATH=xxx)
# either it can be added to the PATH environment variable.
ifdef GCC_PATH
CC = $(GCC_PATH)/$(PREFIX)gcc
AS = $(GCC_PATH)/$(PREFIX)gcc -x assembler-with-cpp
CP = $(GCC_PATH)/$(PREFIX)objcopy
SZ = $(GCC_PATH)/$(PREFIX)size
NM = $(GCC_PATH)/$(PREFIX)nm
A2L = $(GCC_PATH)/$(PREFIX)addr2line
OBJDUMP = $(GCC_PATH)/$(PREFIX)objdump
else
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
endif
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

#######################################
# CFLAGS
#######################################
# cpu
CPU = -mcpu=cortex-m0  -march=armv6-m

# fpu
# NONE for Cortex-M0/M0+/M3

# float-abi
FLOAT-ABI = -mfloat-abi=soft

# mcu
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# macros for gcc
# AS defines
AS_DEFS =

# C defines
C_DEFS =  \
-DUSE_HAL_DRIVER \
-DSTM32F072xB \
-DAPP_DEBUG_LEVEL=$(DEBUG)

# AS includes
AS_INCLUDES =

# C includes
C_INCLUDES =  \
-IInc \
-IDrivers/STM32F0xx_HAL_Driver/Inc \
-IDrivers/STM32F0xx_HAL_Driver/Inc/Legacy \
-IMiddlewares/ST/STM32_USB_Device_Library/Core/Inc \
-IMiddlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc \
-IDrivers/CMSIS/Device/ST/STM32F0xx/Include \
-IDrivers/CMSIS/Include \
-IDrivers/BME680_driver \
-IDrivers/BME680_driver/SelfTest \
-IMiddlewares/Bosch

# compile gcc flags
ASFLAGS = $(MCU) $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
CFLAGS+= -fmessage-length=0 -fno-exceptions -fno-builtin -fomit-frame-pointer -fuse-linker-plugin

ifeq ($(DEBUG), 1)
CFLAGS += -g -gdwarf-2
endif

# Generate dependency information
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

#######################################
# LDFLAGS
#######################################
# link script
LDSCRIPT = STM32F072CBUx_FLASH.ld

# libraries
LIBS = -lc -lalgobsec -lm -lnosys
LIBDIR = -L Middlewares/Bosch
LDFLAGS = $(MCU)  -specs=nano.specs -u _printf_float -T$(LDSCRIPT) $(LIBDIR) $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections
# LDFLAGS+= -nostartfiles -nodefaultlibs -lc -lm  -lnosys

# default action: build all
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

#######################################
# build the application
#######################################
# list of objects
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
# list of ASM program objects
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	@$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@
	@echo "CC $<"

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	@echo "ASM... $<"
	@$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo "Linking... $<"
	@echo "-------------------------------------------------------------------------------"
	@$(SZ) $@
	@echo "-------------------------------------------------------------------------------"

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

$(BUILD_DIR):
	mkdir $@

#-----------------------------------------------------------------------------#
# print the size of the objects and the .elf file
#-----------------------------------------------------------------------------#
print_size :
	@echo 'Size of modules:'
	$(SZ) -B -t --common $(OBJECTS)
	@echo ' '
	@echo 'Size of target .elf file:'
	$(SZ) -B $(BUILD_DIR)/$(TARGET).elf
	@echo ' '

#-----------------------------------------------------------------------------#
# memory dump - elf -> dmp
#-----------------------------------------------------------------------------#
$(BUILD_DIR)/$(TARGET).dmp : $(BUILD_DIR)/$(TARGET).elf
	@echo 'Creating memory dump: $(BUILD_DIR)/$(TARGET).dmp'
	$(OBJDUMP) -x --syms $< > $@
	@echo ' '

#######################################
# Use JLINK to program the device
#######################################
flash:
	JLinkexe flash.jlink
#######################################
# clean up
#######################################
clean:
	-rm -fR $(BUILD_DIR)

#######################################
# dependencies
#######################################
-include $(wildcard $(BUILD_DIR)/*.d)

# *** EOF ***

################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

-include makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/r_fw_up_rz/subdir.mk
-include src/ethercat/renesas/subdir.mk
-include src/ethercat/beckhoff/Src/subdir.mk
-include src/common/subdir.mk
-include src/subdir.mk
-include rzn_gen/subdir.mk
-include rzn/fsp/src/rm_ethercat_ssc_port/subdir.mk
-include rzn/fsp/src/r_xspi_qspi/subdir.mk
-include rzn/fsp/src/r_sci_uart/subdir.mk
-include rzn/fsp/src/r_ioport/subdir.mk
-include rzn/fsp/src/r_ether_selector/subdir.mk
-include rzn/fsp/src/r_ether_phy/subdir.mk
-include rzn/fsp/src/r_cmt/subdir.mk
-include rzn/fsp/src/bsp/mcu/rzn2l/subdir.mk
-include rzn/fsp/src/bsp/mcu/all/cr/subdir.mk
-include rzn/fsp/src/bsp/mcu/all/subdir.mk
-include rzn/fsp/src/bsp/cmsis/Device/RENESAS/Source/cr/subdir.mk
-include rzn/fsp/src/bsp/cmsis/Device/RENESAS/Source/subdir.mk
-include rzn/board/rzn2l_rsk/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(JMP_DEPS)),)
-include $(JMP_DEPS)
endif
ifneq ($(strip $(FSY_DEPS)),)
-include $(FSY_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(SRC_UPPER_DEPS)),)
-include $(SRC_UPPER_DEPS)
endif
ifneq ($(strip $(P_UPPER_DEPS)),)
-include $(P_UPPER_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(PP_UPPER_DEPS)),)
-include $(PP_UPPER_DEPS)
endif
ifneq ($(strip $(P_DEPS)),)
-include $(P_DEPS)
endif
ifneq ($(strip $(FSY_UPPER_DEPS)),)
-include $(FSY_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(SRC_DEPS)),)
-include $(SRC_DEPS)
endif
ifneq ($(strip $(JMP_UPPER_DEPS)),)
-include $(JMP_UPPER_DEPS)
endif
ifneq ($(strip $(PP_DEPS)),)
-include $(PP_DEPS)
endif
ifneq ($(strip $(SX_DEPS)),)
-include $(SX_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(SX_UPPER_DEPS)),)
-include $(SX_UPPER_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
RZN2L_xspi0_app1.bin \

SECONDARY_SIZE += \
RZN2L_xspi0_app1.siz \


# All Target
all:
	$(MAKE) --no-print-directory pre-build
	$(MAKE) --no-print-directory main-build
	$(MAKE) --no-print-directory post-build

# Main-build Target
main-build: RZN2L_xspi0_app1.elf secondary-outputs

# Tool invocations
RZN2L_xspi0_app1.elf: $(OBJS) $(USER_OBJS) $(LIBRARY_GENERATOR_OUTPUTTYPE_OUTPUTS) $(LINKER_SCRIPT)
	$(file > RZN2L_xspi0_app1.elf.in,-mcpu=cortex-r52 -mthumb -mfloat-abi=hard -mfpu=neon-fp-armv8 -fdiagnostics-parseable-fixits -O1 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-strict-aliasing -Wunused -Wuninitialized -Wall -Wextra -Wmissing-declarations -Wconversion -Wpointer-arith -Wshadow -Wlogical-op -Waggregate-return -Wfloat-equal -Wnull-dereference -g -o "RZN2L_xspi0_app1.elf" -Wl,--start-group $(OBJS) $(USER_OBJS) -Wl,--end-group -T "fsp_xspi0_boot_app1_bank1.ld" -L"E:\\RS_workspace\\RZN2L_Multi-protocol_FW_Upgrade\\RZN2L_xspi0_app1/script" -Wl,--start-group  -Wl,--end-group -nostartfiles -Xlinker --gc-sections -Wl,-Map,"RZN2L_xspi0_app1.map" --specs=nano.specs -u _printf_float --specs=nosys.specs -Wl,-esystem_init)
	@echo Building target: $@ && arm-none-eabi-gcc @"RZN2L_xspi0_app1.elf.in"
RZN2L_xspi0_app1.bin: RZN2L_xspi0_app1.elf
	arm-none-eabi-objcopy -O binary --gap-fill 0xff "RZN2L_xspi0_app1.elf"  "RZN2L_xspi0_app1.bin"
RZN2L_xspi0_app1.siz: RZN2L_xspi0_app1.elf
	arm-none-eabi-size --format=berkeley "RZN2L_xspi0_app1.elf"
# Other Targets
clean:
	-$(file > clean_file.tmp,  $(JMP_DEPS)  $(FSY_DEPS)  $(C_UPPER_DEPS)  $(SECONDARY_SIZE)  $(SRC_UPPER_DEPS)  $(P_UPPER_DEPS)  $(S_DEPS)  $(LST)  $(PP_UPPER_DEPS)  $(P_DEPS)  $(FSY_UPPER_DEPS)  $(C_DEPS)  $(SRC_DEPS)  $(JMP_UPPER_DEPS)  $(PP_DEPS)  $(SX_DEPS)  $(ASM_UPPER_DEPS)  $(SX_UPPER_DEPS)  $(OBJS)  $(SECONDARY_FLASH)  $(ASM_DEPS)  $(S_UPPER_DEPS)  $(MAP) )
	-@ xargs -P 2 -s 32000 -t -a clean_file.tmp $(RM)
	-@$(RM) clean_file.tmp
	-$(RM)  RZN2L_xspi0_app1.elf
	-@echo ' '

pre-build:
	-cmd /c del RZN2L_xspi0_app1.bin
	-@echo ' '

post-build:
	-cmd /c copy RZN2L_xspi0_app1.bin RZN2L_xspi0_app1_bank1.bin && python "E:/RS_workspace/RZN2L_Multi-protocol_FW_Upgrade/RZN2L_xspi0_app1\attach_crc_bank1.py"
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)

.PHONY: all clean dependents  main-build pre-build post-build

-include ../makefile.targets

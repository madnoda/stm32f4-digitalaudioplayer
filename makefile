TCHAIN  = arm-m4-eabi
#TCHAIN  = arm-none-eabi
TOOLDIR = /bin
REMOVAL = /bin/rm
WSHELL  = /bin/bash
MSGECHO = /bin/echo

# OPTIMIZE Definition
OPTIMIZE		= 2
#OPTIMIZE		= 0

# FPU Definition
USING_FPU		= -mfloat-abi=softfp  -mfpu=fpv4-sp-d16
#USING_FPU		= -mfloat-abi=soft

# Apprication Version
APP_VER = W.I.P

SUBMODEL		= STM32F405RGT6

MPU_DENSITY		= STM32F4xx
HSE_CLOCK 		= 8000000
USE_CLOCK		= USE_HSE
#USE_CLOCK		= USE_HSI
PERIF_DRIVER	= USE_STDPERIPH_DRIVER
USE_ADC			= NO_ADC
USING_USART		= USE_USART
USING_PRINTF		= USE_PRINTF
UART_DEFAULT_NUM	= 1
USE_MP3			= MP3
USE_MP3FPM		= FPM_DEFAULT

# Use FreeRTOS?

# Synthesis makefile Defines
DEFZ =	\
	$(USE_PHY) $(PERIF_DRIVER) $(USE_MP3) $(USE_MP3FPM)	\
	$(USE_FPU) $(SUBMODEL) $(USE_ADC)						\
	$(USE_CLOCK) $(USING_PRINTF) $(USING_USART)

SYNTHESIS_DEFS	= $(addprefix -D,$(DEFZ))						\
		 -DPACK_STRUCT_END=__attribute\(\(packed\)\) 			\
		 -DALIGN_STRUCT_END=__attribute\(\(aligned\(4\)\)\) 	\
		 -DMPU_SUBMODEL=\"$(SUBMODEL)\"						\
		 -DAPP_VERSION=\"$(APP_VER)\"							\
		 -DHSE_VALUE=$(HSE_CLOCK)UL							\
		 -DUART_DEFAULT_NUM=$(UART_DEFAULT_NUM) 

# TARGET definition
TARGET 	    = main
TARGET_ELF  = $(TARGET).elf
TARGET_SREC = $(TARGET).s19
TARGET_HEX  = $(TARGET).hex
TARGET_BIN  = $(TARGET).bin
TARGET_LSS  = $(TARGET).lss
TARGET_SYM  = $(TARGET).sym

# define Cortex-M4 LIBRARY PATH
FWLIB  		= ../STM32F4-Discovery_FW_V1.1.0/Libraries/STM32F4xx_StdPeriph_Driver
CM4LIB 		= ../STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/
CM4_DEVICE 	= $(CM4LIB)/ST/STM32F4xx
CM4_CORE	= $(CM4LIB)/Include

FATFS		= ../STM32F407xGT6_FatFS_DISP_20120710
FATFSLIB	= $(FATFS)/lib/ff

MP3LIB		= ../libmad-0.15.1b

# include PATH


INCPATHS	 =															\
 	./																	\
	./inc																\
	../STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/ST/STM32F4xx/Include			\
	../STM32F4-Discovery_FW_V1.1.0/Libraries/CMSIS/Include				\
	../STM32F4-Discovery_FW_V1.1.0/Libraries/STM32F4xx_StdPeriph_Driver/inc

INCPATHS += \
	$(FATFSLIB)

ifeq ($(USE_MP3),MP3)
INCPATHS += \
	$(MP3LIB)
endif


INCLUDES     = $(addprefix -I ,$(INCPATHS))

# Set library PATH
LIBPATHS     = $(FWLIB) $(CM4LIB)
LIBRARY_DIRS = $(addprefix -L,$(LIBPATHS))
# if you use math-library, put "-lm" 
# MATH_LIB	 =	-lm

# LinkerScript PATH
LINKER_PATH =  $(FATFS)/lib/linker
LINKER_DIRS = $(addprefix -L,$(LINKER_PATH)) 

# Object definition
OBJS 	 = $(CFILES:%.c=%.o) 
LIBOBJS  = $(LIBCFILES:%.c=%.o) $(SFILES:%.s=%.o)

# C code PATH
SOURCE  = ./src
CFILES = \
 $(SOURCE)/main.c						\
 $(SOURCE)/sd.c							\
 $(FATFSLIB)/ff.c 						\
 $(FATFSLIB)/sdio_stm32f4.c 			\
 $(FATFSLIB)/diskio_sdio.c 			\
 $(FATFSLIB)/option/cc932.c

ifeq ($(USE_MP3),MP3)
CFILES += \
 $(SOURCE)/system_stm32f4xxMP3.c
else
CFILES += \
 $(SOURCE)/system_stm32f4xx.c
endif

ifeq ($(USE_MP3),MP3)
CFILES += \
 $(MP3LIB)/stream.c						\
 $(MP3LIB)/bit.c						\
 $(MP3LIB)/timer.c						\
 $(MP3LIB)/huffman.c					\
 $(MP3LIB)/synth.c						\
 $(MP3LIB)/layer12.c					\
 $(MP3LIB)/layer3.c						\
 $(MP3LIB)/frame.c						\
 $(MP3LIB)/decoder.c
endif

ifeq ($(USING_PRINTF),USE_PRINTF)
CFILES += \
 $(FATFS)/src/syscalls.c
endif

ifeq ($(USING_USART),USE_USART)
CFILES += \
 $(SOURCE)/uart_support.c			\
 $(SOURCE)/stm32f4xx_it.c
endif


#/*----- STARTUP code PATH -----*/
STARTUP_DIR = $(CM4_DEVICE)/Source/Templates/gcc_ride7
SFILES += \
	$(STARTUP_DIR)/startup_stm32f4xx.s

#/*----- STM32 library PATH -----*/
LIBCFILES = \
 $(FWLIB)/src/stm32f4xx_syscfg.c		\
 $(FWLIB)/src/misc.c					\
 $(FWLIB)/src/stm32f4xx_dma.c			\
 $(FWLIB)/src/stm32f4xx_exti.c			\
 $(FWLIB)/src/stm32f4xx_gpio.c			\
 $(FWLIB)/src/stm32f4xx_i2c.c			\
 $(FWLIB)/src/stm32f4xx_rcc.c			\
 $(FWLIB)/src/stm32f4xx_spi.c			\
 $(FWLIB)/src/stm32f4xx_usart.c 		\
 $(FWLIB)/src/stm32f4xx_adc.c			\
 $(FWLIB)/src/stm32f4xx_pwr.c			\
 $(FWLIB)/src/stm32f4xx_sdio.c

#/*----- STM32 Debug library -----*/
ifeq ($(OPTIMIZE),0)
CFILES += \
 ./lib/IOView/stm32f4xx_io_view.c
endif


# TOOLCHAIN SETTING
CC 			= $(TCHAIN)-gcc
CPP 		= $(TCHAIN)-g++
OBJCOPY 	= $(TCHAIN)-objcopy
OBJDUMP 	= $(TCHAIN)-objdump
SIZE 		= $(TCHAIN)-size
AR 			= $(TCHAIN)-ar
LD 			= $(TCHAIN)-gcc
NM 			= $(TCHAIN)-nm
REMOVE		= $(REMOVAL) -f
REMOVEDIR 	= $(REMOVAL) -rf

# C and ASM FLAGS
CFLAGS  = -MD -mcpu=cortex-m4 -march=armv7e-m -mtune=cortex-m4
#CFLAGS  = -MD -mcpu=cortex-m3
CFLAGS += -mthumb -mlittle-endian $(ALIGNED_ACCESS)
CFLAGS += -mapcs-frame -mno-sched-prolog $(USING_FPU)
CFLAGS += -std=gnu99
CFLAGS += -gdwarf-2 -O$(OPTIMIZE) $(USE_LTO)
CFLAGS += -fno-strict-aliasing -fsigned-char
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -fno-schedule-insns2
CFLAGS += --param max-inline-insns-single=1000
CFLAGS += -fno-common -fno-hosted
CFLAGS += -Wall
CFLAGS += -Wa,-adhlns=$(subst $(suffix $<),.lst,$<) 
CFLAGS += $(SYNTHESIS_DEFS)  

# Linker FLAGS -mfloat-abi=softfp -msoft-float
LDFLAGS  = -mcpu=cortex-m4 -march=armv7e-m -mthumb
#LDFLAGS  = -mcpu=cortex-m3 -march=armv7e-m-mthumb
LDFLAGS += -u g_pfnVectors -Wl,-static -Wl,--gc-sections -nostartfiles
LDFLAGS += -Wl,-Map=$(TARGET).map
LDFLAGS += $(LIBRARY_DIRS) $(LINKER_DIRS) $(MATH_LIB)

ifeq ($(USE_EXT_SRAM),DATA_IN_ExtSRAM)
LDFLAGS +=-T$(LINKER_PATH)/$(MPU_DENSITY)_EXTRAM.ld
else
LDFLAGS +=-T$(LINKER_PATH)/$(MPU_DENSITY).ld
endif

# Object Copy generation FLAGS
OBJCPFLAGS = -O
OBJDUMPFLAGS = -h -S -C
DFLAGS = -w

# Build Object
all: gccversion build sizeafter

# Object Size Infomations
ELFSIZE = $(SIZE) -A -x $(TARGET).elf
sizeafter:
	@$(MSGECHO) 
	@$(MSGECHO) Size After:
	$(SIZE) $(TARGET).elf
	@$(SIZE) -A -x $(TARGET).elf
	
# Display compiler version information.
gccversion : 
	@$(CC) --version
	@$(MSGECHO) "BUILD_TYPE = "$(OS_SUPPORT)
	@$(MSGECHO) 


build: $(TARGET_ELF) $(TARGET_LSS) $(TARGET_SYM) $(TARGET_HEX) $(TARGET_SREC) $(TARGET_BIN)

.SUFFIXES: .o .c .s   

$(TARGET_LSS): $(TARGET_ELF)
	@$(MSGECHO)
	@$(MSGECHO) Disassemble: $@
	$(OBJDUMP) $(OBJDUMPFLAGS) $< > $@ 
$(TARGET_SYM): $(TARGET_ELF)
	@$(MSGECHO)
	@$(MSGECHO) Symbol: $@
	$(NM) -n $< > $@
$(TARGET).hex: $(TARGET).elf
	@$(MSGECHO)
	@$(MSGECHO) Objcopy: $@
	$(OBJCOPY) $(OBJCPFLAGS) ihex $^ $@    
$(TARGET).s19: $(TARGET).elf
	@$(MSGECHO)
	@$(MSGECHO) Objcopy: $@
	$(OBJCOPY) $(OBJCPFLAGS) srec $^ $@ 
$(TARGET).bin: $(TARGET).elf
	@$(MSGECHO)
	@$(MSGECHO) Objcopy: $@
	$(OBJCOPY) $(OBJCPFLAGS) binary $< $@ 
$(TARGET).elf: $(OBJS) stm32.a
	@$(MSGECHO) Link: $@
	$(LD) $(CFLAGS) $(LDFLAGS) $^ -o $@
	@$(MSGECHO)

stm32.a: $(LIBOBJS)
	@$(MSGECHO) Archive: $@
	$(AR) cr $@ $(LIBOBJS)    
	@$(MSGECHO)
.c.o:
	@$(MSGECHO) Compile: $<
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@
	@$(MSGECHO)
.s.o:
	@$(MSGECHO) Assemble: $<
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@
	@$(MSGECHO)

# Drop files into dust-shoot
clean:
	$(REMOVE) $(TARGET).hex
	$(REMOVE) $(TARGET).bin
	$(REMOVE) $(TARGET).obj
	$(REMOVE) stm32.a
	$(REMOVE) $(TARGET).elf
	$(REMOVE) $(TARGET).map
	$(REMOVE) $(TARGET).s19
	$(REMOVE) $(TARGET).obj
	$(REMOVE) $(TARGET).a90
	$(REMOVE) $(TARGET).sym
	$(REMOVE) $(TARGET).lnk
	$(REMOVE) $(TARGET).lss
	$(REMOVE) $(wildcard *.stackdump)
	$(REMOVE) $(OBJS)
	$(REMOVE) $(LIBOBJS)
	$(REMOVE) $(CFILES:.c=.lst)
	$(REMOVE) $(CFILES:.c=.d)
	$(REMOVE) $(LIBCFILES:.c=.lst)
	$(REMOVE) $(LIBCFILES:.c=.d)
	$(REMOVE) $(SFILES:.s=.lst)
	$(REMOVE) $(wildcard $(CM4_DEVICE)/*.d)
	$(REMOVE) $(wildcard $(CM4_DEVICE)/*.lst)
	$(REMOVEDIR) .dep
	@$(MSGECHO)


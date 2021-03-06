#
# Build the EBLmon application.
#

#
# Application sources
#
SRCS		 = $(wildcard src/*.cpp) \
		   $(wildcard src/*.c)
INCDIRS		 = src

#
# Serial device
#
UPLOAD_DEV	 = /dev/tty.SLAB_USBtoUART

#
# Build this
#
PRODUCT		 = EBLmon
SUFFIXES	 = .bin .elf
BUILDDIR	 = build
PRODUCTS	 = $(addprefix $(BUILDDIR)/$(PRODUCT),$(SUFFIXES))
all: $(PRODUCTS)

#
# Toolchain
#
PREFIX		 = arm-none-eabi-
CC		 = $(PREFIX)gcc
AS		 = $(PREFIX)gcc
CXX		 = $(PREFIX)g++
LD		 = $(PREFIX)g++
OBJCOPY		 = $(PREFIX)objcopy
SIZE		 = $(PREFIX)size
ARCH_FLAGS	 = -mthumb -mcpu=cortex-m3 -msoft-float

EXTRA_CFLAGS	 =
EXTRA_DEFINES	 =
EXTRA_DEPS	 = 

#
# scmRTOS
#
SCMRTOS		 = scmRTOS
SRCS		+= $(wildcard $(SCMRTOS)/Common/*.cpp) \
		   $(wildcard $(SCMRTOS)/CortexM3/*.cpp) \
		   $(wildcard $(SCMRTOS)/CortexM3/*.S)
INCDIRS		+= $(SCMRTOS)/Common \
		   $(SCMRTOS)/CortexM3 \
		   $(SCMRTOS)
EXTRA_DEFINES	+= -DSTM32F10X_MD

#
# External projects, pulled into ext and built either
# separately using their own build infrastructure, or by
# selective inclusion into SRCS.
#
ifeq ($(wildcard $(ext)),)
R		:= $(shell mkdir -p ext)
endif

#
# libopencm3
#
# Rebuilding the whole thing every time is a bit gross, but
# better than the alternatives. Should find something simpler...
#
LCM3		 = ext/libopencm3
LCM3_URL	 = https://github.com/libopencm3/libopencm3.git
ifeq ($(wildcard $(LCM3)),)
$(info FETCH libopencm3)
R		:= $(shell cd ext && git clone $(LCM3_URL))
endif
LCM3_LIB	 = opencm3_stm32f1
LCM3_TARGETS	 = stm32/f1
INCDIRS		+= $(LCM3)/include
EXTRA_DEFINES	+= -DSTM32F1
EXTRA_DEPS	+= $(LCM3)

.PHONY: $(LCM3)
$(LCM3):
	$(Q) make -C $(LCM3) TARGETS=$(LCM3_TARGETS) lib

#
# U8glib
#
U8G		 = ext/u8glib
U8G_URL		 = https://code.google.com/p/u8glib/
ifeq ($(wildcard $(U8G)),)
$(info FETCH u8g)
R		:= $(shell mkdir -p ext)
R		:= $(shell cd ext && hg clone $(U8G_URL))
endif
U8G_EXCLUDE	 = $(U8G)/csrc/chessengine.c \
		   $(U8G)/csrc/u8g_com_api_16% \
		   $(U8G)/csrc/u8g_com_arduino% \
		   $(U8G)/csrc/u8g_com_atmega% \
		   $(U8G)/csrc/u8g_com_i2c% \
		   $(U8G)/csrc/u8g_com_io% \
		   $(U8G)/csrc/u8g_com_null% \
		   $(U8G)/csrc/u8g_delay.c \
		   $(U8G)/csrc/u8g_dev_% \
		   $(U8G)/csrc/u8g_pb14% \
		   $(U8G)/csrc/u8g_pb16% \
		   $(U8G)/csrc/u8g_pb8h% \
		   $(U8G)/csrc/u8g_pb8v2%
U8G_SRCS	 = $(wildcard $(U8G)/csrc/*.c)
SRCS		+= $(filter-out $(U8G_EXCLUDE),$(U8G_SRCS))
SRCS		+= $(wildcard $(U8G)/sfntsrc/*.c)
INCDIRS		+= $(U8G)/csrc
EXTRA_CFLAGS	+= -Wno-unused

#
# m2tklib
#
M2TK		 = ext/m2tklib
M2TK_URL	 = https://code.google.com/p/m2tklib/
ifeq ($(wildcard $(M2TK)),)
$(info FETCH m2tk)
R		:= $(shell cd ext && hg clone $(M2TK_URL))
endif
M2TK_EXCLUDE	 = $(M2TK)/src/mas%
M2TK_SRCS	+= $(wildcard \
			$(M2TK)/src/*.c \
			$(M2TK)/dev/u8glib/*.c)
SRCS		+= $(filter-out $(M2TK_EXCLUDE),$(M2TK_SRCS))
INCDIRS		+= $(M2TK)/src \
		   $(M2TK)/dev/u8glib

#
# stm32flash
#
STM32FLASH	 = ext/stm32flash
STM32FLASH_URL	 = git://gitorious.org/stm32flash/stm32flash.git
ifeq ($(wildcard $(STM32FLASH)),)
$(info FETCH stm32flash)
R		:= $(shell cd ext && git clone $(STM32FLASH_URL))
endif
UPLOADER	 = $(STM32FLASH)/stm32flash

$(UPLOADER):
	$(Q) make -C $(STM32FLASH)

#
# Build controls
#
OBJS		 = $(addprefix $(BUILDDIR)/,$(addsuffix .o,$(basename $(SRCS))))
DEPS		 = $(OBJS:.o=.d)
GLOBAL_DEPS	 = $(MAKEFILE_LIST)

CFLAGS		 = $(ARCH_FLAGS) \
		   -std=c11 \
		   -Os \
		   -Wall -Wextra \
		   -Wno-unused-parameter \
		   -fno-common \
		   -ffreestanding \
		   -ffunction-sections -fdata-sections \
		   -fno-builtin \
		   -MD \
		   $(addprefix -I,$(INCDIRS)) \
		   $(EXTRA_DEFINES) \
		   $(EXTRA_CFLAGS)

CXXFLAGS	 = $(ARCH_FLAGS) \
		   -std=c++11 \
		   -Os \
		   -Wall -Wextra \
		   -fno-common \
		   -ffreestanding \
		   -ffunction-sections -fdata-sections \
		   -fno-builtin \
		   -fno-exceptions \
		   -fno-rtti \
		   -fno-threadsafe-statics \
		   -funsigned-bitfields \
		   -fshort-enums \
		   -Wpointer-arith \
		   -Wredundant-decls \
		   -Wno-unused-parameter \
		   -Wshadow \
		   -Wcast-qual \
		   -Wcast-align \
		   -MD \
		   $(addprefix -I,$(INCDIRS)) \
		   -DSTM32F1 \
		   $(EXTRA_CXXFLAGS)

ASFLAGS		 = $(ARCH_FLAGS) \
		   -g \
		   $(addprefix -I,$(INCDIRS)) \
		   -D__ASSEMBLER__

STDLIBS		 = m c gcc nosys
LDSCRIPT	 = stm32f103rbt6.ld ext/libopencm3/lib/stm32/f1/libopencm3_stm32f1.ld
LDFLAGS		 = $(ARCH_FLAGS) \
		   --static \
		   --specs=nano.specs \
		   -L $(LCM3)/lib \
		   -l $(LCM3_LIB) \
		   -nostartfiles \
		   -nostdlib \
		   $(addprefix -l,$(STDLIBS)) \
		   -Wl,--gc-sections \
		   $(addprefix -T,$(LDSCRIPT)) \
		   -Wl,-Map=$(BUILDDIR)/$(PRODUCT).map,--cref


# Build debugging
ifeq ($(V),)
Q		 = @
endif

#
# Rules
#

upload: $(BUILDDIR)/$(PRODUCT).bin $(UPLOADER)
	@echo UPLOAD $<
	$(Q) $(UPLOADER) -R -w $< $(UPLOAD_DEV)

$(BUILDDIR)/$(PRODUCT).elf: $(OBJS) $(LDSCRIPT) $(GLOBAL_DEPS) $(LCM3)
	@echo LD $(notdir $@)
	$(Q) $(LD) -o $@ $(OBJS) $(LDFLAGS)
	@$(SIZE) $@

$(BUILDDIR)/%.o: %.c $(GLOBAL_DEPS)
	@echo CC $(notdir $@)
	@mkdir -p $(dir $@)
	$(Q) $(CC) $(CFLAGS) -o $@ -c $<

$(BUILDDIR)/%.o: %.cpp $(GLOBAL_DEPS)
	@echo CXX $(notdir $@)
	@mkdir -p $(dir $@)
	$(Q) $(CXX) $(CXXFLAGS) -o $@ -c $<

$(BUILDDIR)/%.o: %.S $(GLOBAL_DEPS)
	@echo AS $(notdir $@)
	@mkdir -p $(dir $@)
	$(Q) $(AS) $(ASFLAGS) -o $@ -c $<

%.bin: %.elf
	@echo BIN $(notdir $@)
	$(Q) $(OBJCOPY) -O binary $^ $@

$(SRCS): $(EXTRA_DEPS)

.PHONY: clean
clean:
	$(Q) rm -rf $(BUILDDIR)

.PHONY: reallyclean
reallyclean: clean
	$(Q) make -C $(LCM3) TARGETS=$(LCM3_TARGETS) clean

.PHONY: reformat
reformat:
	$(Q) astyle \
	--style=kr \
	--indent=spaces=4 \
	--indent-preprocessor \
	--min-conditional-indent=0 \
	--pad-oper \
	--pad-header \
	--break-blocks=all \
	--unpad-paren \
	--keep-one-line-blocks \
	--keep-one-line-statements \
	--align-pointer=name \
	--align-reference=name \
	--lineend=linux \
	--suffix=none \
	--ignore-exclude-errors-x \
	src/*.cpp src/*.h

-include $(DEPS)

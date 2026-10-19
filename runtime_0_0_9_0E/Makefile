# SA3DS 0.0.9.0E — Real Sonic 3DS Runtime Gate
.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM. Example: export DEVKITPRO=/opt/devkitpro; export DEVKITARM=$$DEVKITPRO/devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET      := sa3ds_0_0_9_0E1_real_sonic
BUILD       := build
SOURCES     := source ../src/sa3c
INCLUDES    := include ../src/sa3c
ROMFS       := romfs
APP_TITLE   := SA3DS 0.0.9.0E.1
APP_DESCRIPTION := Real Sonic 3DS Runtime Build Fix
APP_AUTHOR  := SA3DS project

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := -g -Wall -Wextra -Werror=implicit-function-declaration -O2 -mword-relocations \
          -ffunction-sections $(ARCH)
CFLAGS += $(INCLUDE) -D__3DS__
ASFLAGS := -g $(ARCH)
LDFLAGS = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS := -lcitro3d -lctru -lm
LIBDIRS := $(CTRULIB)

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))

export LD := $(CC)
export OFILES_SOURCES := $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES_BIN := $(PICAFILES:.v.pica=.shbin.o)
export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)
export HFILES := $(PICAFILES:.v.pica=_shbin.h)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export _3DSXDEPS := $(if $(NO_SMDH),,$(OUTPUT).smdh)
export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
ifeq ($(strip $(NO_SMDH)),)
export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh
endif

.PHONY: all clean check-assets
all: check-assets $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

check-assets:
	@test -f $(ROMFS)/sa3ds/sonic.sa3c || (echo "Missing owner-generated romfs/sa3ds/sonic.sa3c"; echo "Run: python3 ../tools/prepare_3ds_owner_assets.py <CHRMODELS.DLL> <SONIC.PVM>"; exit 1)
	@test -f $(ROMFS)/sa3ds/sonic.sa3t || (echo "Missing owner-generated romfs/sa3ds/sonic.sa3t"; echo "Run: python3 ../tools/prepare_3ds_owner_assets.py <CHRMODELS.DLL> <SONIC.PVM>"; exit 1)

$(BUILD):
	@mkdir -p $@

clean:
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).elf $(TARGET).smdh $(TARGET).map

else

$(OUTPUT).3dsx: $(OUTPUT).elf $(_3DSXDEPS)
$(OFILES_SOURCES): $(HFILES)
$(OUTPUT).elf: $(OFILES)

.PRECIOUS: %.shbin
%.shbin.o %_shbin.h: %.shbin
	$(SILENTMSG) $(notdir $<)
	$(bin2o)

-include $(DEPSDIR)/*.d
endif

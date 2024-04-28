all: dependencies allfiles lib install
reduced: files lib install

LIB = libpyblk.a
LIBDIR=$(CURDIR)/../lib
GENERIC_INC = $(PYSUPSICTRL)/CodeGen/Common/include
TARGET_INC = ../include
INCLUDE =  -I$(GENERIC_INC) -I$(TARGET_INC)

COMMON_DIR = $(PYSUPSICTRL)/CodeGen/Common
EXT_LIBS = $(PYSUPSICTRL)/ExtLibs
GENERATED_INC = $(CURDIR)/../include-generated

OMK_COMPILED_INC_PATH = $(GENERATED_INC)
OMK_COMPILED_LIB_PATH = $(LIBDIR)

RTEMS_MAKEFILE_PATH=/opt/rtems/7/arm-rtems7/xilinx_zynq_zedboard

ifndef RTEMS_MAKEFILE_PATH
$(warning Specify RTEMS_MAKEFILE_PATH)
$(warning make RTEMS_MAKEFILE_PATH=/path/to/rtems/BSP/directory)
$(error RTEMS_MAKEFILE_PATH not defined, cannot continue)
endif

ifeq ($(wildcard $(RTEMS_MAKEFILE_PATH)/Makefile.inc),)
$(error RTEMS Makefile.inc not found at RTEMS_MAKEFILE_PATH)
endif

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
include $(RTEMS_CUSTOM)
include $(CONFIG.CC)

INCLUDE +=  -I$(GENERIC_INC) -I$(TARGET_INC)

SRCALL = $(wildcard $(COMMON_DIR)/common_dev/*.c)
SRCALL += $(wildcard $(COMMON_DIR)/posix/*.c)
SRCALL += $(wildcard $(COMMON_DIR)/can/*.c)

dependencies:
	@mkdir -p $(GENERATED_INC)

ifneq ($(wildcard $(RTEMS_ROOT)/$(RTEMS_TARGET)/$(RTEMS_BSP)/lib/include/dev/can/ctucanfd.h),)
SRCALL += can.c
endif

OBJ = $(notdir $(SRCALL:.c=.o))

CWD = $(shell pwd)

OBJEX = $(SRC:.c=.o)
DEFINES = -D PLOTTER_SCRIPT=\"$(CWD)/scope.py\"
CC_FLAGS = -c $(DBG) $(INCLUDE) $(CFLAGS) $(CPU_CFLAGS) $(CPU_DEFINES) $(CFLAGS_OPTIMIZE_V)
CC_FLAGS += -I$(GENERATED_INC)
ifeq ($(SHV),1)
CC_FLAGS += -I$(COMMON_DIR)/shv/include
endif
CC_FLAGS_SCP = -c $(DBG) $(INCLUDE) $(CFLAGS) $(DEFINES)

EXTLIB_CFLAGS=$(CC_FLAGS)
EXTLIB_CXXFLAGS=$(CC_FLAGS)
EXTLIB_LDFLAGS=$(CC_FLAGS)

ifeq ($(SHV),1)
SRCALL += $(wildcard $(COMMON_DIR)/shv/*.c)

CONFIG_SHV_LIBS4C_PLATFORM="rtems"
include $(COMMON_DIR)/rules/make-ulut.inc
include $(COMMON_DIR)/rules/make-shv-libs4c.inc
dependencies: make-ulut make-shv-libs4c
endif

allfiles:

files:
EXCLUDE = comedi_analog_input.c comedi_analog_output.c comedi_digital_input.c comedi_digital_output.c
SRC=$(filter-out $(EXCLUDE),$(SRCALL))

scope.o: scope.c
	$(CC) $(CC_FLAGS_SCP) $<

%.o: %.c
	$(CC) $(CC_FLAGS) $<

%.o: sensors/%.c
	$(CC) $(CC_FLAGS) $<

ifeq ($(SHV),1)
%.o: $(COMMON_DIR)/shv/%.c
	$(CC) $(CC_FLAGS) $<
endif

%.o: $(COMMON_DIR)/shv/%.c
	$(CC) $(CC_FLAGS) $<

%.o: $(COMMON_DIR)/CAN_dev/%.c
	$(CC) $(CC_FLAGS) $<

%.o: $(COMMON_DIR)/common_dev/%.c
	$(CC) $(CC_FLAGS) $<

%.o: $(COMMON_DIR)/Faulhaber_dev/%.c
	$(CC) $(CC_FLAGS) $<

%.o: $(COMMON_DIR)/Maxon_dev/%.c
	$(CC) $(CC_FLAGS) $<

%.o: $(COMMON_DIR)/posix/%.c
	$(CC) $(CC_FLAGS) $<

%.o: $(COMMON_DIR)/can/%.c
	$(CC) $(CC_FLAGS) $<

lib: $(OBJ)
	$(AR) r $(LIB) $(OBJ)

install:
	mkdir -p ../lib
	cp $(LIB) ../lib

clean:
	@echo "RM: _build _compiled *.omk-default include-generated"
	@echo "RM: $(LIB)"
	@echo "RM: *.o"
	@rm -rf _build _compiled *.omk-default include-generated
	@rm -f $(LIB) $(OBJ)

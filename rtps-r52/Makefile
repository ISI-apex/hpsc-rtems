#
#  RTEMS_MAKEFILE_PATH is typically set in an environment variable
#

PGM=${ARCH}/rtps-r52.exe

# optional managers required
MANAGERS=all

CONFIG_FLAGS = \
	TEST_RTPS_TRCH_MAILBOX \
	CONFIG_HPPS_RTPS_MAILBOX \

CONFIG_ARGS = $(foreach m,$(CONFIG_FLAGS),-D$(m)=$($(m)))

# Enable disable tests here:
TEST_RTPS_TRCH_MAILBOX  	= 1

# Set build configuration here
CONFIG_HPPS_RTPS_MAILBOX  	= 1

# C source names
CSRCS = \
	server.c \
	main.c
COBJS = $(CSRCS:%.c=${ARCH}/%.o)

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
include $(RTEMS_CUSTOM)
include $(PROJECT_ROOT)/make/leaf.cfg

OBJS= $(COBJS) $(CXXOBJS) $(ASOBJS)

all:    ${ARCH} $(PGM)

CFLAGS += \
	$(CONFIG_ARGS) \

LIBMBOX = ../mailbox/${ARCH}/libhpsc-mbox.a
LIBHPSC = ../hpsc/${ARCH}/libhpsc.a

LD_LIBS += \
	$(LIBMBOX) \
	$(LIBHPSC)

$(PGM): $(LIBMBOX) $(LIBHPSC) $(OBJS)
	$(make-exe)

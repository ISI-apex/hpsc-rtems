#
#  RTEMS_MAKEFILE_PATH is typically set in an environment variable
#

PGM=${ARCH}/rtps-r52.exe

# optional managers required
MANAGERS=all

CONFIG_FLAGS = \
	TEST_RTPS_TRCH_MAILBOX \
	TEST_SHMEM \
	CONFIG_HPPS_RTPS_MAILBOX \

CONFIG_ARGS = $(foreach m,$(CONFIG_FLAGS),-D$(m)=$($(m)))

# Enable disable tests here:
TEST_RTPS_TRCH_MAILBOX  	= 1
TEST_SHMEM			= 1

# Set build configuration here
CONFIG_HPPS_RTPS_MAILBOX  	= 1

# C source names
CSRCS = \
	server.c \
	main.c
COBJS = $(CSRCS:%.c=${ARCH}/%.o)

ifeq ($(strip $(TEST_RTPS_TRCH_MAILBOX)),1)
CSRCS += tests/mailbox.c
endif
ifeq ($(strip $(TEST_SHMEM)),1)
CSRCS += tests/shmem.c
endif

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
include $(RTEMS_CUSTOM)
include $(PROJECT_ROOT)/make/leaf.cfg

OBJS = $(COBJS) $(CXXOBJS) $(ASOBJS)

${ARCH}/tests:
	mkdir -p ${ARCH}/tests

all:    ${ARCH} ${ARCH}/tests $(PGM)

CFLAGS += \
	$(CONFIG_ARGS) \
	-I. -Itests -I../drivers -I../hpsc -I../plat \

LIBDRIVERS = ../drivers/${ARCH}/libhpsc-drivers.a
LIBHPSC = ../hpsc/${ARCH}/libhpsc.a

LD_LIBS += \
	$(LIBDRIVERS) \
	$(LIBHPSC)

$(PGM): $(LIBDRIVERS) $(LIBHPSC) $(OBJS)
	$(make-exe)

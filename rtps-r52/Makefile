#
#  RTEMS_MAKEFILE_PATH is typically set in an environment variable
#

PGM=${ARCH}/rtps-r52.exe

# optional managers required
MANAGERS=all

CONFIG_FLAGS = \
	TEST_RTI_TIMER \
	TEST_RTPS_TRCH_MAILBOX \
	TEST_SHMEM \
	CONFIG_MBOX_LSIO \
	CONFIG_MBOX_HPPS \
	CONFIG_WDT \

CONFIG_ARGS = $(foreach m,$(CONFIG_FLAGS),-D$(m)=$($(m)))

# Enable disable tests here:
TEST_RTI_TIMER			= 0 # TODO: currently broken
TEST_RTPS_TRCH_MAILBOX		= 1
TEST_SHMEM			= 1

# Set build configuration here
CONFIG_MBOX_LSIO		= 1
CONFIG_MBOX_HPPS		= 1
CONFIG_WDT 			= 1

# C source names
CSRCS = \
	devices.c \
	server.c \
	main.c \
	watchdog.c
CSRCS += \
	tests/mailbox.c \
	tests/rti-timer.c \
	tests/shmem.c
COBJS = $(CSRCS:%.c=${ARCH}/%.o)

H_FILES = \
	devices.h \
	gic.h \
	test.h \
	watchdog.h \

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
include $(RTEMS_CUSTOM)
include $(PROJECT_ROOT)/make/leaf.cfg

OBJS = $(COBJS) $(CXXOBJS) $(ASOBJS)

${ARCH}/tests:
	mkdir -p ${ARCH}/tests

all:    ${ARCH} ${ARCH}/tests $(PGM)

CFLAGS += \
	$(CONFIG_ARGS) \
	-I. -Itests -I../drivers -I../bist -I../hpsc -I../plat \

LIBDRIVERS = ../drivers/${ARCH}/libhpsc-drivers.a
LIBBIST = ../bist/${ARCH}/libhpsc-bist.a
LIBHPSC = ../hpsc/${ARCH}/libhpsc.a

LD_LIBS += \
	$(LIBHPSC) \
	$(LIBBIST) \
	$(LIBDRIVERS) \

$(OBJS): $(LIBDRIVERS) $(LIBBIST) $(LIBHPSC) ${H_FILES} ../plat/* Makefile

$(PGM): $(OBJS)
	$(make-exe)

#
#  RTEMS_MAKEFILE_PATH is typically set in an environment variable
#

PGM=${ARCH}/rtps-r52.exe

# optional managers required
MANAGERS=all

CONFIG_FLAGS = \
	TEST_RTI_TIMER \
	TEST_MBOX_LSIO_LOOPBACK \
	TEST_MBOX_LSIO_TRCH \
	TEST_SHMEM \
	CONFIG_MBOX_LSIO \
	CONFIG_MBOX_HPPS \
	CONFIG_SHELL \
	CONFIG_WDT \

CONFIG_ARGS = $(foreach m,$(CONFIG_FLAGS),-D$(m)=$($(m)))

# Set build configuration here
CONFIG_MBOX_LSIO		= 1
CONFIG_MBOX_HPPS		= 1
CONFIG_SHELL			= 1
CONFIG_WDT			= 1

# Enable/disable tests here (tests often require related configs):
TEST_MBOX_LSIO_LOOPBACK		= 1
TEST_MBOX_LSIO_TRCH		= 1
TEST_RTI_TIMER			= 0 # TODO: Re-enable when RTEMS frequency fixed
TEST_SHMEM			= 1

# C source names
CSRCS = \
	devices.c \
	shell.c \
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
	shell.h \
	server.h \
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
	-I. -Itests -I../plat \
	-I../lib/drivers -I../lib/drivers-selftest -I../lib/hpsc

LIBDRIVERS = ../lib/drivers/${ARCH}/libhpsc-drivers.a
LIBDRIVERS_TEST = ../lib/drivers-selftest/${ARCH}/libhpsc-drivers-selftest.a
LIBHPSC = ../lib/hpsc/${ARCH}/libhpsc.a

LD_LIBS += \
	$(LIBHPSC) \
	$(LIBDRIVERS_TEST) \
	$(LIBDRIVERS) \

$(OBJS): $(LIBDRIVERS) $(LIBDRIVERS_TEST) $(LIBHPSC) ${H_FILES} ../plat/* Makefile

$(PGM): $(OBJS)
	$(make-exe)

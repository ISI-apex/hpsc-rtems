#
#  RTEMS_MAKEFILE_PATH is typically set in an environment variable
#

PGM=${ARCH}/rtps-r52.exe

# optional managers required
MANAGERS=all

# Drivers
CONFIG_FLAGS = \
	CONFIG_MBOX_LSIO \
	CONFIG_MBOX_HPPS_RTPS \
	CONFIG_RTI_TIMER \
	CONFIG_WDT \
# Links
CONFIG_FLAGS += \
	CONFIG_LINK_MBOX_TRCH_CLIENT \
	CONFIG_LINK_MBOX_HPPS_SERVER \
	CONFIG_LINK_SHMEM_TRCH_CLIENT \
	CONFIG_LINK_SHMEM_TRCH_SERVER \
# Additional tasks
CONFIG_FLAGS += \
	CONFIG_SHELL \
# Standalone tests
CONFIG_FLAGS += \
	TEST_COMMAND \
	TEST_MBOX_LSIO_LOOPBACK \
	TEST_RTI_TIMER \
	TEST_SHMEM \
# Runtime tests
CONFIG_FLAGS += \
	TEST_COMMAND_SERVER \
	TEST_LINK_SHMEM \
# External tests
CONFIG_FLAGS += \
	TEST_MBOX_LINK_TRCH \
	TEST_SHMEM_LINK_TRCH \

CONFIG_ARGS = $(foreach m,$(CONFIG_FLAGS),-D$(m)=$($(m)))

# Set build configuration here
# Drivers
CONFIG_MBOX_LSIO		= 1
CONFIG_MBOX_HPPS_RTPS		= 1
CONFIG_RTI_TIMER		= 1
CONFIG_WDT			= 1
# Links
CONFIG_LINK_MBOX_TRCH_CLIENT	= 1 # send requests to TRCH
CONFIG_LINK_MBOX_HPPS_SERVER	= 1 # listen for requests from HPPS
CONFIG_LINK_SHMEM_TRCH_CLIENT	= 1 # send requests to TRCH
CONFIG_LINK_SHMEM_TRCH_SERVER	= 1 # listen for requests from TRCH
# Additional tasks
CONFIG_SHELL			= 1

# Enable/disable tests here (some tests require certain CONFIG options):
# Standalone
TEST_COMMAND			= 1
TEST_MBOX_LSIO_LOOPBACK		= 1
TEST_RTI_TIMER			= 1
TEST_SHMEM			= 1
# Runtime
TEST_COMMAND_SERVER		= 1
TEST_LINK_SHMEM			= 1
# External
TEST_MBOX_LINK_TRCH		= 1
TEST_SHMEM_LINK_TRCH		= 1

# C source names
CSRCS = \
	devices.c \
	init.c \
	server.c \
	shutdown.c \
	watchdog.c
CSRCS += \
	tests/command.c \
	tests/command-server.c \
	tests/link-external.c \
	tests/link-shmem.c \
	tests/mailbox.c \
	tests/rti-timer.c \
	tests/shmem.c
COBJS = $(CSRCS:%.c=${ARCH}/%.o)

H_FILES = \
	devices.h \
	gic.h \
	link-names.h \
	server.h \
	shutdown.h \
	test.h \
	watchdog.h \

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
include $(RTEMS_CUSTOM)
include $(PROJECT_ROOT)/make/leaf.cfg

OBJS = $(COBJS) $(CXXOBJS) $(ASOBJS)

${ARCH}/tests: ${ARCH}
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

$(OBJS): $(LIBDRIVERS) $(LIBDRIVERS_TEST) $(LIBHPSC) ${ARCH} ${H_FILES} ../plat/* Makefile

$(PGM): $(OBJS)
	$(make-exe)

PGM_IMG = $(addsuffix .img,$(basename ${PGM}))
install:  all
	test -d ${PROJECT_BIN} || $(MKDIR) ${PROJECT_BIN}
	$(INSTALL_VARIANT) -m 644 ${PGM} ${PGM_IMG} ${PROJECT_BIN}

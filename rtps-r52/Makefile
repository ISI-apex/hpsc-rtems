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
	TEST_LSIO_SRAM_SYSCFG \
	TEST_LSIO_SRAM_DMA_SYSCFG \
	TEST_MBOX_LSIO_LOOPBACK \
	TEST_RTI_TIMER \
	TEST_RTPS_DMA \
	TEST_RTPS_MMU \
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

include Makefile.defconfig
include Makefile.config

# C source names
CSRCS = \
	gic.c \
	init.c \
	server.c \
	shell-tests.c \
	shutdown.c \
	watchdog.c
CSRCS += \
	tests/hpsc-test-wrappers.c \
	tests/link-external.c \
	tests/lsio-sram.c \
	tests/mailbox.c \
	tests/rti-timer.c \
	tests/rtps-dma.c \
	tests/rtps-mmu.c
COBJS = $(CSRCS:%.c=${ARCH}/%.o)

H_FILES = \
	gic.h \
	link-names.h \
	server.h \
	shell-tests.h \
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
	-I../lib/drivers -I../lib/drivers-selftest \
	-I../lib/hpsc -I../lib/hpsc-test

LIBDRIVERS = ../lib/drivers/${ARCH}/libhpsc-drivers.a
LIBDRIVERS_TEST = ../lib/drivers-selftest/${ARCH}/libhpsc-drivers-selftest.a
LIBHPSC = ../lib/hpsc/${ARCH}/libhpsc.a
LIBHPSC_TEST = ../lib/hpsc-test/${ARCH}/libhpsc-test.a

LD_LIBS += \
	$(LIBHPSC_TEST) \
	$(LIBHPSC) \
	$(LIBDRIVERS_TEST) \
	$(LIBDRIVERS) \

# Empty config means all settings come from defconfig
Makefile.config:
	touch $@

$(OBJS): $(LIBDRIVERS) $(LIBDRIVERS_TEST) $(LIBHPSC) $(LIBHPSC_TEST) ${H_FILES} \
	 ../plat/* Makefile Makefile.defconfig Makefile.config | ${ARCH} ${ARCH}/tests

$(PGM): $(OBJS)
	$(make-exe)

PGM_IMG = $(addsuffix .img,$(basename ${PGM}))
install:  all
	$(INSTALL_CHANGE) ${INSTDIRFLAGS} ${PROJECT_BIN}
	$(INSTALL_VARIANT) ${INSTBINFLAGS} ${PGM} ${PGM_IMG} ${PROJECT_BIN}

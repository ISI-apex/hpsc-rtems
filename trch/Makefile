#
#  RTEMS_MAKEFILE_PATH is typically set in an environment variable
#
include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
include $(RTEMS_CUSTOM)
include $(RTEMS_SHARE)/make/directory.cfg

PGM=${ARCH}/trch.exe

# optional managers required
MANAGERS=all

# C source names
CSRCS = init.c
COBJS = $(CSRCS:%.c=${ARCH}/%.o)

H_FILES = 

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
include $(RTEMS_CUSTOM)
include $(PROJECT_ROOT)/make/leaf.cfg

OBJS = $(COBJS) $(CXXOBJS) $(ASOBJS)

all:    ${ARCH} $(PGM)

CFLAGS += 

$(OBJS): ${H_FILES}

$(PGM): $(OBJS)
	$(make-exe)

PGM_IMG = $(addsuffix .img,$(basename ${PGM}))
install:  all
	$(INSTALL_CHANGE) ${INSTDIRFLAGS} ${PROJECT_BIN}
	$(INSTALL_VARIANT) ${INSTBINFLAGS} ${PGM} ${PGM_IMG} ${PROJECT_BIN}

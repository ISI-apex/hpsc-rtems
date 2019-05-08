#
#  RTEMS_MAKEFILE_PATH is typically set in an environment variable
#

LIBNAME=libhpsc-test.a        # xxx- your library names goes here
LIB=${ARCH}/${LIBNAME}

# C and C++ source names, if any, go here -- minus the .c or .cc
C_PIECES= \
	command \
	command-server \
	link \
	link-shmem \
	shmem
C_FILES=$(C_PIECES:%=%.c)
C_O_FILES=$(C_FILES:%.c=${ARCH}/%.o)

CC_PIECES=
CC_FILES=$(CC_PIECES:%=%.cc)
CC_O_FILES=$(CC_PIECES:%=${ARCH}/%.o)

H_FILES = hpsc-test.h

# Assembly source names, if any, go here -- minus the .S
S_PIECES=
S_FILES=$(S_PIECES:%=%.S)
S_O_FILES=$(S_FILES:%.S=${ARCH}/%.o)

SRCS=$(C_FILES) $(CC_FILES) $(H_FILES) $(S_FILES)
OBJS=$(C_O_FILES) $(CC_O_FILES) $(S_O_FILES)

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc

include $(RTEMS_CUSTOM)
include $(RTEMS_SHARE)/make/lib.cfg

#
# Add local stuff here using +=
#

DEFINES  +=
CPPFLAGS +=
CFLAGS   += \
	-I../hpsc \


#
# Add your list of files to delete here.  The config files
#  already know how to delete some stuff, so you may want
#  to just run 'make clean' first to see what gets missed.
#  'make clobber' already includes 'make clean'
#

CLEAN_ADDITIONS +=
CLOBBER_ADDITIONS +=

all:	${ARCH} $(SRCS) $(LIB)

LIBHPSC = ../hpsc/${ARCH}/libhpsc.a

$(OBJS): $(LIBHPSC) ${H_FILES} Makefile | ${ARCH}

$(LIB): ${OBJS}
	$(make-library)

# Install the library, appending _g or _p as appropriate.
# for include files, just use $(INSTALL_CHANGE)
H_INSTALL_DIR = $(addprefix ${PROJECT_INCLUDE}/,$(subst lib,,$(basename $(LIBNAME))))
install:  all
	$(INSTALL_VARIANT) ${INSTLIBFLAGS} ${LIB} ${PROJECT_RELEASE}/lib
	$(INSTALL_CHANGE) ${INSTDIRFLAGS} ${H_INSTALL_DIR}
	$(INSTALL_CHANGE) ${INSTINCFLAGS} ${H_FILES} ${H_INSTALL_DIR}

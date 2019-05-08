include $(RTEMS_MAKEFILE_PATH)/Makefile.inc
include $(RTEMS_CUSTOM)
include $(RTEMS_ROOT)/make/leaf.cfg

H_FILES = \
	hpsc-busids.dtsh \
	hpsc-irqs.dtsh \
	hwinfo.h \
	mailbox-map.h \
	mem-map.h \

# for include files, just use $(INSTALL_CHANGE)
H_INSTALL_DIR = $(addprefix ${PROJECT_INCLUDE}/,hpsc-plat)
install:
	$(INSTALL_CHANGE) ${INSTDIRFLAGS} ${H_INSTALL_DIR}
	$(INSTALL_CHANGE) ${INSTINCFLAGS} ${H_FILES} ${H_INSTALL_DIR}

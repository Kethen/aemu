all:
	mv pspnet_shims.prx ../dist/kd/pspnet_shims.prx

TARGET = pspnet_shims

#IMPORTS = \
#imports/sceNetAdhoc.o

OBJS = main.o ../littlec.o ../tinyalloc-master/tinyalloc.o ../memab/ATPRO.o $(LIBRARYOBJS) $(IMPORTS)

CFLAGS = -Os -fno-builtin -G0 -Wall -fno-pic -DHEAP_SIZE=0

ifeq ($(CONFIG_620), 1)
CFLAGS += -DCONFIG_620=1
endif

ifeq ($(CONFIG_63X), 1)
CFLAGS += -DCONFIG_63X=1
endif

ifeq ($(CONFIG_660), 1)
CFLAGS += -DCONFIG_660=1
endif

ifeq ($(RELEASE), 1)
CFLAGS += -DRELEASE=1
else
CFLAGS += -DDEBUG=1
endif

ifeq ($(TRACE), 1)
CFLAGS += -DTRACE=1
endif

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

BUILD_PRX = 1
PRX_EXPORTS = exports.exp

LDFLAGS = -nostartfiles
LIBS = -nostdlib -lpspuser -lpspsdk -lpspmodinfo -lpspnet_resolver

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

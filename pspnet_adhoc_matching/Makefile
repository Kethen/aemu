all:
	mv pspnet_adhoc_matching.prx ../dist/kd/pspnet_adhoc_matching.prx

TARGET = pspnet_adhoc_matching

LIBRARYOBJS = \
library/functions.o \
library/calls/init.o \
library/calls/term.o \
library/calls/create.o \
library/calls/start.o \
library/calls/stop.o \
library/calls/delete.o \
library/calls/select.o \
library/calls/cancel.o \
library/calls/send.o \
library/calls/abortsend.o \
library/calls/sethello.o \
library/calls/gethello.o \
library/calls/getmembers.o \
library/calls/getpoolmaxalloc.o \
library/calls/getpoolstat.o

#IMPORTS = \
#imports/sceNetAdhoc.o

OBJS = main.o ../littlec.o ../tinyalloc-master/tinyalloc.o ../memab/ATPRO.o $(LIBRARYOBJS) $(IMPORTS)
#OBJS = main.o ../memab/ATPRO.o $(LIBRARYOBJS) $(IMPORTS)

# we need a lot of buffers for this one, because games can make huge opts, and we have a two thread architecture that processes messages
# an example is dissidia final fantasy (like, 5k hellos and joins)
CFLAGS = -Os -fno-builtin -G0 -Wall -fno-pic -DHEAP_SIZE=100 -DTINYALLOC_USE_PARTITION_MEM=1

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
LIBS = -nostdlib -lpspuser -lpspnet -lpspnet_adhoc -lpspmodinfo -lpspnet_adhocctl
#LIBS = -lpspnet_adhoc

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

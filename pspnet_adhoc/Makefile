all:
	mv pspnet_adhoc.prx ../dist/kd/pspnet_adhoc.prx

TARGET = pspnet_adhoc

LIBRARYOBJS = \
library/functions.o \
library/calls/init.o \
library/calls/term.o \
library/calls/setalert.o \
library/calls/getalert.o \
library/calls/pdp/poll.o \
library/calls/pdp/create.o \
library/calls/pdp/delete.o \
library/calls/pdp/send.o \
library/calls/pdp/recv.o \
library/calls/pdp/getstat.o \
library/calls/ptp/open.o \
library/calls/ptp/connect.o \
library/calls/ptp/listen.o \
library/calls/ptp/accept.o \
library/calls/ptp/send.o \
library/calls/ptp/recv.o \
library/calls/ptp/flush.o \
library/calls/ptp/close.o \
library/calls/ptp/getstat.o \
library/calls/gamemode/createmaster.o \
library/calls/gamemode/createreplica.o \
library/calls/gamemode/updatemaster.o \
library/calls/gamemode/updatereplica.o \
library/calls/gamemode/deletemaster.o \
library/calls/gamemode/deletereplica.o

IMPORTS = \
imports/sceNetAdhocctl.o \
imports/sceNetAdhocctlIOLock.o \
imports/sceNetAdhocctlInternal.o \
imports/sceNetMiniUPnP.o \
imports/pspnet_inet_kermit_redirect.o

#OBJS = main.o ../memab/ATPRO.o $(LIBRARYOBJS) $(IMPORTS)
OBJS = main.o ../littlec.o ../tinyalloc-master/tinyalloc.o ../memab/ATPRO.o $(LIBRARYOBJS) $(IMPORTS)

#CFLAGS = -Os -G0 -Wall -fno-pic
CFLAGS = -Os -fno-builtin -G0 -Wall -fno-pic -DHEAP_SIZE=25

ifeq ($(CONFIG_620), 1)
CFLAGS += -DCONFIG_620=1
endif

ifeq ($(CONFIG_63X), 1)
CFLAGS += -DCONFIG_63X=1
endif

ifeq ($(CONFIG_660), 1)
CFLAGS += -DCONFIG_660=1
endif

ifeq ($(PDP_DIRTY_MAGIC), 1)
CFLAGS += -DPDP_DIRTY_MAGIC=1
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

USE_PSPSDK_LIBC = 1

LDFLAGS = -nostartfiles
LIBS = -nostdlib -lpspuser -lpspnet_inet -lpspmodinfo -lpsputility -lpspnet_adhocctl -lpspnet_adhocmatching

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

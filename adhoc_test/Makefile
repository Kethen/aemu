TARGET = adhoc_test
OBJS = main.o
LIBS = -lpspnet -lpspnet_adhoc -lpspnet_adhocctl -lpspnet_adhocmatching
BUILD_PRX=1
PSP_FW_VERSION = 660

CFLAGS = -O2 -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)
ENCRYPT=1
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = adhoc test

PSP_LARGE_MEMORY=0

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

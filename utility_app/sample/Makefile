LVGL_DIR = ..
LVGL_DIR_NAME ?=lvgl
include $(LVGL_DIR)/$(LVGL_DIR_NAME)/lvgl.mk
COBJS = $(CSRCS:.c=.o)

TARGET = demo
OBJS = $(COBJS)  ../lv_port_disp.o  ../lv_port_indev.o ../mouse_cursor_icon.o ../printk.o main.o

INCDIR = -I$(LVGL_DIR)/$(LVGL_DIR_NAME)
CFLAGS = -Wall -O2
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR = 
LDFLAGS = 
LIBS= -lpspgu -lpspvram -lpspdmac

BUILD_PRX=1
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = LVGL Demo

PSP_FW_VERSION = 660
PSP_LARGE_MEMORY = 0

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak


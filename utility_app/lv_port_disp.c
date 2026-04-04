/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include "lvgl/lvgl.h"
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspge.h>
#include <vram.h>
#include <pspkerneltypes.h>
#include <pspdmac.h>
#include <psputils.h>
#include <pspthreadman.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "printk.h"

/*********************
 *      DEFINES
 *********************/

#define FBSIZE (PSP_BUF_WIDTH * PSP_VERT_RES * 4)
#define LVGL_FBSIZE (PSP_HOR_RES * PSP_VERT_RES * 4)
#define PROFILE 0

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//        const lv_area_t * fill_area, lv_color_t color);

/**********************
 *  STATIC VARIABLES
 **********************/

static char* disp_buf_0;
static char* disp_buf_1;
static char draw_buf[LVGL_FBSIZE];

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-----------------------------
     * Create buffers for drawing
     *----------------------------*/

    disp_buf_0 = (char *)vramalloc(FBSIZE);
    disp_buf_1 = (char *)vramalloc(FBSIZE);

    /*-------------------------
     * Initialize your display
     * -----------------------*/

    disp_init();

    static lv_disp_draw_buf_t draw_buf_dsc_3;
    lv_disp_draw_buf_init(&draw_buf_dsc_3, draw_buf, NULL,
                          PSP_HOR_RES * PSP_VERT_RES);   /*Initialize the display buffer*/

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = PSP_HOR_RES;
    disp_drv.ver_res = PSP_VERT_RES;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc_3;
    //disp_drv.full_refresh = 1;
    disp_drv.direct_mode = 1;

    /* Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.*/
    //disp_drv.gpu_fill_cb = gpu_fill;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
	sceDisplaySetMode(PSP_DISPLAY_MODE_LCD, PSP_HOR_RES, PSP_VERT_RES);
	sceDisplayWaitVblankStart();
}

static void dcache_writeback(uint32_t addr, int size){
	static const int alignment = 64;

	int addr_mod = (addr % alignment);
	size += addr_mod;
	addr -= addr_mod;

	int size_mod = size % alignment;
	if (size_mod != 0){
		size += (alignment - size_mod);
	}

	sceKernelDcacheWritebackRange((void *)addr, size);
}

static void log_frame_time(){
	static uint64_t last_tick = 0;
	uint64_t now = sceKernelGetSystemTimeWide();
	int time_spent_us = now - last_tick;
	last_tick = sceKernelGetSystemTimeWide();
}

/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{	
	static int buf_toggle = 0;
	char *dfb = (char *)color_p;
	char *fb = buf_toggle ? disp_buf_0 : disp_buf_1;
	buf_toggle = !buf_toggle;

	#if PROFILE
	uint64_t begin = sceKernelGetSystemTimeWide();
	#endif


	for(int i = 0;i < PSP_VERT_RES;i++){
		memcpy(&fb[i * PSP_BUF_WIDTH * 4], &dfb[i * PSP_HOR_RES * 4], PSP_HOR_RES * 4);
	}
	dcache_writeback((uint32_t)fb, LVGL_FBSIZE);

	#if PROFILE
	int time_spent_us = sceKernelGetSystemTimeWide() - begin;
	printk("%s: time spent copying frame buffer %d us\n", __func__, time_spent_us);
	#endif

	#if PROFILE
	begin = sceKernelGetSystemTimeWide();
	#endif

	sceDisplaySetFrameBuf(fb, PSP_BUF_WIDTH, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_NEXTVSYNC);

	#if PROFILE
	time_spent_us = sceKernelGetSystemTimeWide() - begin;
	printk("%s: time spent setting frame buffer %d us\n", __func__, time_spent_us);
    #endif

    sceDisplayWaitVblank();

	#if PROFILE
	log_frame_time();
	#endif

    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    lv_disp_flush_ready(disp_drv);
}

/*OPTIONAL: GPU INTERFACE*/

/*If your MCU has hardware accelerator (GPU) then you can use it to fill a memory with a color*/
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//                    const lv_area_t * fill_area, lv_color_t color)
//{
//    /*It's an example code which should be done by your GPU*/
//    int32_t x, y;
//    dest_buf += dest_width * fill_area->y1; /*Go to the first line*/
//
//    for(y = fill_area->y1; y <= fill_area->y2; y++) {
//        for(x = fill_area->x1; x <= fill_area->x2; x++) {
//            dest_buf[x] = color;
//        }
//        dest_buf+=dest_width;    /*Go to the next line*/
//    }
//}


/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;


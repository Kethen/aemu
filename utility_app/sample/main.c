//#include "../lvgl/examples/lv_examples.h"
#include "../lvgl/demos/lv_demos.h"
#include "../lvgl/demos/keypad_encoder/lv_demo_keypad_encoder.h"
#include "../lv_port_disp.h"
#include "../lv_port_indev.h"
#include "../printk.h"

#include <pspkernel.h>

#include <stdio.h>
#include <stdint.h>

#define PROFILE 0

PSP_MODULE_INFO("LVGL Sample", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

int main(int argc, char** argv)
{
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    //lv_example_get_started_3();
    //lv_demo_music();
    lv_demo_keypad_encoder();

    while (true) {
		#if $PROFILE
    	uint64_t begin = sceKernelGetSystemTimeWide();
    	#endif

        lv_timer_handler();

		#if $PROFILE
        int time_spent = sceKernelGetSystemTimeWide() - begin;
        printk("%s: lv_timer_handler took %d us\n", __func__, time_spent);
		#endif

        // we yield in display driver
        //sceKernelDelayThread(0);
    }
}

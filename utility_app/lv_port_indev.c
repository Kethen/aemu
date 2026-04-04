/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"
#include "lvgl/lvgl.h"
#include <pspctrl.h>

/*********************
 *      DEFINES
 *********************/
#define MOUSE_DEADZONE 10
#define MOUSE_MAX_SPEED 10
#define MOUSE_SPEED_MODIFIER (127 / MOUSE_MAX_SPEED)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void mouse_init(void);
static void mouse_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static bool mouse_is_pressed(void);
static void mouse_get_xy(lv_coord_t * x, lv_coord_t * y);

static void keypad_init(void);
static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static uint32_t keypad_get_key(void);

/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t * indev_mouse;
lv_indev_t * indev_keypad;

static int mouse_x; // in screen coordinates
static int mouse_y; // in screen coordinates

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    static lv_indev_drv_t indev_drv;
    static lv_indev_drv_t indev_drv2;

    /*------------------
     * Mouse
     * -----------------*/

    /*Initialize your mouse if you have*/
    mouse_init();

    //[>Register a mouse input device<]
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = mouse_read;
    indev_mouse = lv_indev_drv_register(&indev_drv);

    //[>Set cursor. For simplicity set a HOME symbol now.<]
    LV_IMG_DECLARE(mouse_cursor_icon); //[>Declare the image file.<]
    lv_obj_t * mouse_cursor = lv_img_create(lv_scr_act());
    lv_img_set_src(mouse_cursor, &mouse_cursor_icon);
    lv_indev_set_cursor(indev_mouse, mouse_cursor);

    /*------------------
     * Keypad
     * -----------------*/

    /*Initialize your keypad or keyboard if you have*/
    keypad_init();

    //Register a keypad input device
    lv_indev_drv_init(&indev_drv2);
    indev_drv2.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv2.read_cb = keypad_read;
    indev_keypad = lv_indev_drv_register(&indev_drv2);

    /*Later you should create group(s) with `lv_group_t * group = lv_group_create()`,
     *add objects to the group with `lv_group_add_obj(group, obj)`
     *and assign this input device to group to navigate in it:
     *`lv_indev_set_group(indev_keypad, group);`*/
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*------------------
 * Mouse
 * -----------------*/

/*Initialize your mouse*/
static void mouse_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the mouse*/
static void mouse_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    /*Get the current x and y coordinates*/
    mouse_get_xy(&data->point.x, &data->point.y);

    /*Get whether the mouse button is pressed or released*/
    if(mouse_is_pressed()) {
        data->state = LV_INDEV_STATE_PR;
    }
    else {
        data->state = LV_INDEV_STATE_REL;
    }
}

/*Return true is the mouse button is pressed*/
static bool mouse_is_pressed(void)
{
    /*Your code comes here*/
    SceCtrlData pad;
    sceCtrlPeekBufferPositive(&pad, 1);

    if (pad.Buttons & PSP_CTRL_CROSS)
    {
        return true;
    }
    else 
    {
        return false;
    }
}

int mouse_get_delta(uint8_t axis) 
{
    int delta_val = (int)axis - 127;
    int distance_without_deadzone;
    if (delta_val > -MOUSE_DEADZONE && delta_val < MOUSE_DEADZONE)
    {
        distance_without_deadzone = 0;
    } 
    else if (delta_val < -MOUSE_DEADZONE) 
    {
        distance_without_deadzone = delta_val + MOUSE_DEADZONE;
    } 
    else
    {
        distance_without_deadzone = delta_val - MOUSE_DEADZONE;
    }

    return distance_without_deadzone / MOUSE_SPEED_MODIFIER;
}


/*Get the x and y coordinates if the mouse is pressed*/
static void mouse_get_xy(lv_coord_t * x, lv_coord_t * y)
{
    /*Your code comes here*/
    SceCtrlData pad;
    sceCtrlPeekBufferPositive(&pad, 1);

    mouse_x += mouse_get_delta(pad.Lx);
    if (mouse_x < 0)
    {
        mouse_x = 0;
    }
    if (mouse_x > 480)
    {
        mouse_x = 480;
    }
    mouse_y += mouse_get_delta(pad.Ly);
    if (mouse_y < 0)
    {
        mouse_y = 0;
    }
    if (mouse_y > 272)
    {
        mouse_y = 272;
    }


    (*x) = mouse_x;
    (*y) = mouse_y;
}

/*------------------
 * Keypad
 * -----------------*/

/*Initialize your keypad*/
static void keypad_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the mouse*/
static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{
    static uint32_t last_key = 0;

    /*Get the current x and y coordinates*/
    mouse_get_xy(&data->point.x, &data->point.y);

    /*Get whether the a key is pressed and save the pressed key*/
    uint32_t act_key = keypad_get_key();
    if(act_key != 0) {
        data->state = LV_INDEV_STATE_PR;

        /*Translate the keys to LVGL control characters according to your key definitions*/
        switch(act_key) {
            case PSP_CTRL_CROSS:
                act_key = LV_KEY_ENTER;
                break;
            case PSP_CTRL_LEFT:
                act_key = LV_KEY_LEFT;
                break;
            case PSP_CTRL_RIGHT:
                act_key = LV_KEY_RIGHT;
                break;
            case PSP_CTRL_UP:
                act_key = LV_KEY_UP;
                break;
            case PSP_CTRL_DOWN:
                act_key = LV_KEY_DOWN;
                break;
            case PSP_CTRL_CIRCLE:
                act_key = LV_KEY_ESC;
                break;
            case PSP_CTRL_LTRIGGER:
                act_key = LV_KEY_PREV;
                break;
            case PSP_CTRL_RTRIGGER:
                act_key = LV_KEY_NEXT;
                break;
            case PSP_CTRL_SQUARE:
                act_key = LV_KEY_BACKSPACE;
                break;
            case PSP_CTRL_START:
                act_key = LV_KEY_HOME;
                break;
            case PSP_CTRL_TRIANGLE:
                act_key = LV_KEY_DEL;
                break;
            case PSP_CTRL_SELECT:
                act_key = LV_KEY_END;
                break;
            default:
                // it seems like lvgl doesn't support multiple keys being pressed at once potentially
                // so any of those will act as no keypress :/
                act_key = 0;
                break;
        }

        last_key = act_key;
    }
    else {
        data->state = LV_INDEV_STATE_REL;
    }

    data->key = last_key;
}

/*Get the currently being pressed key.  0 if no key is pressed*/
static uint32_t keypad_get_key(void)
{
    /*Your code comes here*/
    SceCtrlData pad;
    sceCtrlPeekBufferPositive(&pad, 1);

    return pad.Buttons;
}

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;

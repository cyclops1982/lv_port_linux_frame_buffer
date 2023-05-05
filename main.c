#include "lvgl/lvgl.h"
//#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include "fbink.h"
#include "fbink_types.h"

#define DISP_BUF_SIZE (128 * 1024)

int fbfd = -1;

static void kindle_flush(lv_disp_drv_t *disp, const lv_area_t * area, lv_color_t * color_p) {
    printf("=== Kindle flush\n");
    fbdev_flush(disp, area, color_p);
    printf("===  DONE flush, doing refresh\n");
    FBInkConfig fbink_cfg = { 0U };
    fbink_cfg.no_refresh = false;
    fbink_cfg.is_cleared = true;
    
    if (lv_disp_flush_is_last(disp)) {
        fbink_refresh(fbfd, 0U, 0U, 0U, 0U, &fbink_cfg);
        printf("== Forcing refresh in kindle_flush\n");
    } else {
        printf("== NOT the last flush");
    }
}





 static void fbdev_monitor(lv_disp_drv_t *disp_drv, uint32_t time, uint32_t px) {
     /* The eInk screen on this hardware must be manually "kicked" after the framebuffer contents are changed */
         
	FBInkConfig fbink_cfg = { 0U };
    fbink_cfg.no_refresh = false;
    fbink_cfg.is_cleared = true;

    fbink_refresh(fbfd, 0U, 0U, 0U, 0U, &fbink_cfg);
    printf("== REFRESHED THE SCREEN in %u for %u pixers\n", time, px);

 }




static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {
        static uint8_t cnt = 0;
        cnt++;

        /*Get the first child of the button which is the label and change its text*/
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Button: %d", cnt);
    }
}

int main(void)
{
    /*LittlevGL init*/
    lv_init();

    /*Linux frame buffer device init*/
    fbdev_init();

// Init FBInk
    FBInkConfig fbink_cfg = { 0U };
    fbink_cfg.is_verbose = true;

	// Open framebuffer and keep it around, then setup globals.
	if ((fbfd = fbink_open()) == 1) {
		fprintf(stderr, "Failed to open the framebuffer, aborting . . .\n");
	}
	if (fbink_init(fbfd, &fbink_cfg) != 0) {
		fprintf(stderr, "Failed to initialize FBInk, aborting . . .\n");
	}

    /*A small buffer for LittlevGL to draw the screen's content*/
    static lv_color_t buf[DISP_BUF_SIZE];

    /*Initialize a descriptor for the buffer*/
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);
    fprintf(stderr, "Initialized buffer\n");

    /*Initialize and register a display driver*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
        fprintf(stderr, "Initialized driver\n");
    disp_drv.draw_buf   = &disp_buf;
    //disp_drv.flush_cb   = fbdev_flush;
    disp_drv.flush_cb = kindle_flush;
    disp_drv.monitor_cb = fbdev_monitor;
    disp_drv.hor_res    = 600;
    disp_drv.ver_res    = 800;
    lv_disp_drv_register(&disp_drv);
        fprintf(stderr, "Initialized drv register\n");

    evdev_init();
    static lv_indev_drv_t indev_drv_1;
    lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/
    indev_drv_1.type = LV_INDEV_TYPE_POINTER;


    /*This function will be called periodically (by the library) to get the mouse position and state*/
    indev_drv_1.read_cb = evdev_read;
    lv_indev_t *mouse_indev = lv_indev_drv_register(&indev_drv_1);


    /*Set a cursor for the mouse*/
    LV_IMG_DECLARE(mouse_cursor_icon)
    lv_obj_t * cursor_obj = lv_img_create(lv_scr_act()); /*Create an image object for the cursor */
    lv_img_set_src(cursor_obj, &mouse_cursor_icon);           /*Set the image source*/
    lv_indev_set_cursor(mouse_indev, cursor_obj);             /*Connect the image  object to the driver*/


    /*Create a Demo*/
    lv_demo_widgets();


    /*Handle LitlevGL tasks (tickless mode)*/
    while(1) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}
/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}




// void kindle_force_update(lv_area_t *area)
// {

//     printf("Doing forced refresh\n");
// 	/* Setup the update region data structure. */
// 	if(area != NULL) {
// 		/*
// 		wait_for_update_data.update_marker = fullUpdRegion.update_marker;
// 		wait_for_update_data.collision_test = 0;
// 		ioctl(fb0fd,  MXCFB_WAIT_FOR_UPDATE_COMPLETE, (unsigned long int)&wait_for_update_data);
// 		*/
// 		//fullUpdRegion.update_region.top = area->y1;
// 		//fullUpdRegion.update_region.left = area->x1;
// 		//fullUpdRegion.update_region.width = (area->x2 - area->x1 + 1);
// 		//fullUpdRegion.update_region.height = (area->y2 - area->y1 + 1);
// 	}


// 	FBInkConfig fbink_cfg = { 0U };
//     fbink_cfg.no_refresh = false;
//     fbink_cfg.is_cleared = true;

//     fbink_refresh(fbfd, 0U, 0U, 0U, 0U, &fbink_cfg);

// 	//fullUpdRegion.update_mode = UPDATE_MODE_PARTIAL;
// 	//fullUpdRegion.flags = 0;
// 	//fullUpdRegion.update_marker = current_update++;
// 	/* Send the update request to the eInk chip */
// 	//ioctl(fb0fd , MXCFB_SEND_UPDATE, (unsigned long int)&fullUpdRegion);
// }


// static void fbdev_monitor(lv_disp_drv_t *disp_drv, uint32_t time, uint32_t px) {
//     /* The eInk screen on this hardware must be manually "kicked" after the framebuffer contents are changed */
//     kindle_force_update(&refresh_area);
//     /* Zero the dirty area (as it has now been updated) */
//     lv_area_set(&refresh_area, 0, 0, 0, 0);
// }

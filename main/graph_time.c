/*
 * @Descripttion: 
 * @version: 
 * @Author: Newt
 * @Date: 2022-11-13 20:27:34
 * @LastEditors: Newt
 * @LastEditTime: 2022-12-10 23:25:48
 */
#include <stdint.h>
#include "aiot_ntp_api.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

extern xQueueHandle ntp_time_qhandle;
extern xQueueHandle request_time_qhandle;


typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint16_t msec;
} calender_time_t;

static calender_time_t ali_oclock;

static void graph_timer_init(const aiot_ntp_recv_t *packet, calender_time_t *oclock)
{
    oclock->year = (uint16_t)packet->data.local_time.year;
    oclock->month = (uint8_t)packet->data.local_time.mon;
    oclock->day = (uint8_t)packet->data.local_time.day;
    oclock->hour = (uint8_t)packet->data.local_time.hour;
    oclock->min = (uint8_t)packet->data.local_time.min;
    oclock->sec = (uint8_t)packet->data.local_time.sec;
    oclock->msec = (uint16_t)packet->data.local_time.msec;
}

static lv_style_t style_text;
static void graph_oclock_display_init(lv_obj_t *t_date, lv_obj_t *t_time)
{
    lv_style_init(&style_text);
    lv_style_set_text_color(&style_text, lv_color_black());
    lv_obj_add_style(t_date, &style_text, 0);
    lv_obj_add_style(t_time, &style_text, 0);

    lv_obj_set_pos(t_date, 80, 10);
    lv_obj_set_pos(t_time, 80, 30);
    
    lv_label_set_text_fmt(t_date, "%04u-%02u-%02u", ali_oclock.year, ali_oclock.month, ali_oclock.day);
    lv_label_set_text_fmt(t_time, "%02u:%02u:%02u", ali_oclock.hour, ali_oclock.min, ali_oclock.sec);
}

static void graph_oclock_display_update(lv_obj_t *t_date, lv_obj_t *t_time)
{
    lv_label_set_text_fmt(t_date, "%4u-%02u-%02u", ali_oclock.year, ali_oclock.month, ali_oclock.day);
    lv_label_set_text_fmt(t_time, "%02u:%02u:%02u", ali_oclock.hour, ali_oclock.min, ali_oclock.sec);
}

static void graph_one_secend_handler(lv_timer_t * timer)
{
    ali_oclock.sec++;
    if (ali_oclock.sec >= 60) {
        ali_oclock.min++;
        ali_oclock.sec = 0;
    }
    if (ali_oclock.min >= 60) {
        ali_oclock.hour++;
    }
    if (ali_oclock.hour >= 24) {
        ali_oclock.hour = 0;
    }
}

void graph_ntp_display_task(void *parameter)
{
    aiot_ntp_recv_t ntp_package;
    lv_obj_t *text_date = lv_label_create(lv_scr_act());
    lv_obj_t *text_time = lv_label_create(lv_scr_act());
    lv_timer_t *lv_timer = lv_timer_create(graph_one_secend_handler, 1000,  NULL);
    
    printf("111111111111\r\n");
    if(ntp_time_qhandle != NULL) {
        if (xQueueReceive(ntp_time_qhandle, &ntp_package, portMAX_DELAY) == pdTRUE) {
            graph_timer_init(&ntp_package, &ali_oclock);
        }
    }
    
    uint8_t tmp = ali_oclock.hour;
    uint8_t request_flag = 1;
    graph_oclock_display_init(text_date, text_time);
    lv_timer_ready(lv_timer);
    printf("2222222222222\r\n");
    while (1) {
        graph_oclock_display_update(text_date, text_time);
        // printf("333333333333333333\r\n");
        if ((ali_oclock.hour <= 21) && ((ali_oclock.hour - tmp) >= 3)) {
            if (xQueueSend(request_time_qhandle, &request_flag, portMAX_DELAY)) {
                if (xQueueReceive(ntp_time_qhandle, &ntp_package, portMAX_DELAY) == pdTRUE) {
                    graph_timer_init(&ntp_package, &ali_oclock);
                }
                tmp = ali_oclock.hour;
            }
            else {
                printf("get time requeset failed\r\n");
                vTaskDelete(NULL);
            }
        }
        vTaskDelay(500/portTICK_PERIOD_MS);
    }
    printf("Delte graph_ntp_display_task\r\n");
    vTaskDelete(NULL);
}


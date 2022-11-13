/*
 * @Descripttion: 
 * @version: 
 * @Author: Newt
 * @Date: 2022-10-02 16:25:18
 * @LastEditors: Newt
 * @LastEditTime: 2022-10-03 17:16:15
 */
#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lv_port_disp.h"
#include "hw_timer.h"

void lv_tick_hw_cd()
{
    lv_tick_inc(1);
}

void lv_tick_task()
{
    hw_timer_init();
    hw_timer_set_func(lv_tick_hw_cd);
    hw_timer_arm(1, 1);

    while (1) {
        lv_task_handler();
        vTaskDelay(30);
    }
    vTaskDelete(NULL);
}

void lv_tick_handle_init()
{
    xTaskHandle lv_mian_task;
    xTaskCreate(lv_tick_task, "lv_mian_task", 1024, NULL, 15, &lv_mian_task);
    printf("create lv main task\r\n");
}



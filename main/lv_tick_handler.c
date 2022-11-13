/*
 * @Descripttion: 
 * @version: 
 * @Author: Newt
 * @Date: 2022-10-02 16:25:18
 * @LastEditors: Newt
 * @LastEditTime: 2022-11-13 16:51:02
 */
#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lv_port_disp.h"
#include "driver/timer.h"

#define TIMER_DIVIDER         (16)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER / 1000)  // convert counter value to seconds

typedef struct {
    int timer_group;
    int timer_idx;
    int alarm_interval;
    bool auto_reload;
} example_timer_info_t;

/**
 * @brief A sample structure to pass events from the timer ISR to task
 *
 */
typedef struct {
    example_timer_info_t info;
    uint64_t timer_counter_value;
} example_timer_event_t;

static bool IRAM_ATTR timer_group_isr_callback(void *args)
{
    lv_tick_inc(1);
    return pdTRUE;
}

static void example_tg_timer_init(int group, int timer, bool auto_reload, int timer_interval_sec)
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_EN,
        .auto_reload = auto_reload,
    }; // default clock source is APB
    timer_init(group, timer, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(group, timer, 0);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(group, timer, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(group, timer);

    example_timer_info_t *timer_info = calloc(1, sizeof(example_timer_info_t));
    timer_info->timer_group = group;
    timer_info->timer_idx = timer;
    timer_info->auto_reload = auto_reload;
    timer_info->alarm_interval = timer_interval_sec;
    timer_isr_callback_add(group, timer, timer_group_isr_callback, timer_info, 0);

    timer_start(group, timer);
}

void lv_tick_task()
{
    example_tg_timer_init(TIMER_GROUP_0, TIMER_0, true, 1);

    while (1) {
        lv_task_handler();
        vTaskDelay(30/portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void lv_tick_handle_init()
{
    xTaskHandle lv_mian_task;
    xTaskCreate(lv_tick_task, "lv_mian_task", 3 * 1024, NULL, 15, &lv_mian_task);
    printf("create lv main task\r\n");
}



/*
 * @Descripttion:
 * @version:
 * @Author: Newt
 * @Date: 2022-11-06 22:50:51
 * @LastEditors: Newt
 * @LastEditTime: 2022-12-10 23:18:02
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"

#include "tftlcd.h"
#include "lvgl.h"
#include "lv_port_disp.h"

#include "freertos/FreeRTOSConfig.h"
#include "esp_task_wdt.h"


extern void wifi_init_sta(void);
extern int ali_ntp_connect(void);
extern void ESP_NVS_init(void);
extern void station_example_function(void);

void boot_print_info()
{
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());
}

void lvgl_init()
{
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_SEL_4 | GPIO_SEL_5 | GPIO_SEL_18 | GPIO_SEL_19;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    // if (esp_tft_init() == 0) {
    //     printf("tft screen init success\r\n");
    // }
    // esp_tft_single_color(GREEN);

    lv_init();
    lv_port_disp_init();

    extern void lv_tick_handle_init();
    lv_tick_handle_init();
}

void app_main(void)
{
    boot_print_info();
    lvgl_init();

    printf("Start create task ali iot");
    xTaskHandle station_example_handle;
    xTaskCreate(station_example_function, "ali iot", 4 * 1024, NULL, 14, &station_example_handle);
}

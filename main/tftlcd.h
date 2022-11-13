/*
 * @Descripttion: 
 * @version: 
 * @Author: Newt
 * @Date: 2022-08-04 11:09:35
 * @LastEditors: Newt
 * @LastEditTime: 2022-11-12 13:12:53
 */

#ifndef TFTLCD_H
#define TFTLDC_H

#include "driver/gpio.h"


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define     RED          0XF800	  //红色
#define     GREEN        0X07E0	  //绿色
#define     BLUE         0X001F	  //蓝色
#define     WHITE        0XFFFF	  //白色

#define TFT_COLUMN_NUMBER 240
#define TFT_LINE_NUMBER 240
#define TFT_COLUMN_OFFSET 0

#define PIC_NUM 28800			//图片数据大小

#define RES 
#define DC  
#define BLK

#define ENABLE_RES  
#define ENABLE_DC   
#define ENABLE_BLK

#define SET_RES_LOW  gpio_set_level(GPIO_NUM_18, 0)         
#define SET_RES_HIGH gpio_set_level(GPIO_NUM_18, 1)
#define SET_DC_LOW   gpio_set_level(GPIO_NUM_19, 0)         
#define SET_DC_HIGH  gpio_set_level(GPIO_NUM_19, 1)     
#define SET_BLK_HIGH     
#define SET_BLK_HIGH   

/**
 * @brief: 
 * @param {unsigned char} parameter
 * @return {*}
 */
void esp_tft_send_cmd(unsigned char parameter);

/**
 * @brief: 
 * @param {unsigned char} parameter
 * @return {*}
 */
void esp_tft_send_data(unsigned char parameter);

/**
 * @brief: 
 * @return {*}
 */
int esp_tft_init();

/**
 * @brief: 
 * @return {*}
 */
void esp_tft_clear();

/**
 * @brief: 
 * @param {unsigned int} color
 * @return {*}
 */
void esp_tft_single_color(unsigned int color);

/**
 * @brief: 
 * @param {unsigned int} x
 * @param {unsigned int} y
 * @param {unsigned long} color
 * @param {unsigned char} word_serial_number
 * @return {*}
 */
void esp_tft_display_CN(unsigned int x, unsigned int y, unsigned long color, unsigned char word_serial_number);

/**
 * @brief: 
 * @param {unsigned int} x
 * @param {unsigned int} y
 * @param {unsigned long} color
 * @param {unsigned char} character_len
 * @return {*}
 */
void esp_tft_display_char(unsigned int x, unsigned int y, unsigned long color, unsigned char character_len);

/**
 * @brief: 
 * @param {unsigned char} *ptr_pic
 * @return {*}
 */
void esp_tft_display_pic(const unsigned char *ptr_pic);

/**
 * @brief: 
 * @param {uint16_t} x
 * @param {uint16_t} y
 * @param {uint16_t} color
 * @return {*}
 */
int esp_tft_draw_point(uint16_t x, uint16_t y, uint16_t color);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* TFTLCD_H */
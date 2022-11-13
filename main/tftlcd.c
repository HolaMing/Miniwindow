/*
 * @Descripttion:
 * @version:
 * @Author: Newt
 * @Date: 2022-08-04 11:09:35
 * @LastEditors: Newt
 * @LastEditTime: 2022-11-12 13:02:55
 */

#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tftlcd.h"
#include "simulate_spi.h"

/**
 * @brief:
 * @param {unsigned char} data
 * @return {*}
 */
void esp_tft_send_cmd(unsigned char data)
{
    SET_DC_LOW;
    SET_SCK_LOW;
    simulate_spi_transmit(&data, 1);
    // esp_spi_dma_write(data);
}

/**
 * @brief:
 * @param {unsigned char} data
 * @return {*}
 */
void esp_tft_send_data(unsigned char data)
{
    SET_DC_HIGH;
    SET_SCK_LOW;
    simulate_spi_transmit(&data, 1);
    // esp_spi_dma_write(data);
}

/**
 * @brief:
 * @return {*}
 */
static void esp_tft_gpio_init()
{
    ENABLE_RES;
    ENABLE_DC;
    simulate_spi_init();
}

/**
 * @brief:
 * @return {*}
 */
int esp_tft_init()
{
    esp_tft_gpio_init();

    SET_SCK_HIGH; //特别注意！！
    SET_RES_LOW;
    vTaskDelay(1000);
    SET_RES_HIGH;
    vTaskDelay(1000);

    esp_tft_send_cmd(0x11); // Sleep Out
    vTaskDelay(120);

    esp_tft_send_cmd(0x3A); // 65k mode
    esp_tft_send_data(0x05);

    esp_tft_send_cmd(0xC5); // VCOM
    esp_tft_send_data(0x1A);
    esp_tft_send_cmd(0x36); // 屏幕显示方向设置
    // esp_tft_send_data(0x40);
    esp_tft_send_data(0x00);
    //-------------ST7789V Frame rate setting-----------//
    esp_tft_send_cmd(0xb2); // Porch Setting
    esp_tft_send_data(0x05);
    esp_tft_send_data(0x05);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0x33);
    esp_tft_send_data(0x33);

    esp_tft_send_cmd(0xb7);  // Gate Control
    esp_tft_send_data(0x05); // 12.2v   -10.43v
    //--------------ST7789V Power setting---------------//
    esp_tft_send_cmd(0xBB); // VCOM
    esp_tft_send_data(0x3F);

    esp_tft_send_cmd(0xC0); // Power control
    esp_tft_send_data(0x2c);

    esp_tft_send_cmd(0xC2); // VDV and VRH Command Enable
    esp_tft_send_data(0x01);

    esp_tft_send_cmd(0xC3);  // VRH Set
    esp_tft_send_data(0x0F); // 4.3+( vcom+vcom offset+vdv)

    esp_tft_send_cmd(0xC4);  // VDV Set
    esp_tft_send_data(0x20); // 0v

    esp_tft_send_cmd(0xC6);  // Frame Rate Control in Normal Mode
    esp_tft_send_data(0X01); // 111Hz

    esp_tft_send_cmd(0xd0); // Power Control 1
    esp_tft_send_data(0xa4);
    esp_tft_send_data(0xa1);

    esp_tft_send_cmd(0xE8); // Power Control 1
    esp_tft_send_data(0x03);

    esp_tft_send_cmd(0xE9); // Equalize time control
    esp_tft_send_data(0x09);
    esp_tft_send_data(0x09);
    esp_tft_send_data(0x08);
    //---------------ST7789V gamma setting-------------//
    esp_tft_send_cmd(0xE0); // Set Gamma
    esp_tft_send_data(0xD0);
    esp_tft_send_data(0x05);
    esp_tft_send_data(0x09);
    esp_tft_send_data(0x09);
    esp_tft_send_data(0x08);
    esp_tft_send_data(0x14);
    esp_tft_send_data(0x28);
    esp_tft_send_data(0x33);
    esp_tft_send_data(0x3F);
    esp_tft_send_data(0x07);
    esp_tft_send_data(0x13);
    esp_tft_send_data(0x14);
    esp_tft_send_data(0x28);
    esp_tft_send_data(0x30);

    esp_tft_send_cmd(0XE1); // Set Gamma
    esp_tft_send_data(0xD0);
    esp_tft_send_data(0x05);
    esp_tft_send_data(0x09);
    esp_tft_send_data(0x09);
    esp_tft_send_data(0x08);
    esp_tft_send_data(0x03);
    esp_tft_send_data(0x24);
    esp_tft_send_data(0x32);
    esp_tft_send_data(0x32);
    esp_tft_send_data(0x3B);
    esp_tft_send_data(0x14);
    esp_tft_send_data(0x13);
    esp_tft_send_data(0x28);
    esp_tft_send_data(0x2F);

    esp_tft_send_cmd(0x21); //反显

    esp_tft_send_cmd(0x29); //开启显示
    return 0;
}

/**
 * @brief:
 * @return {*}
 */
void esp_tft_clear()
{
    unsigned int row, column;

    esp_tft_send_cmd(0x2a);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0xF0);

    esp_tft_send_cmd(0x2b);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0xF0);
    esp_tft_send_cmd(0x2C);
    // Memory write
    for (row = 0; row < TFT_LINE_NUMBER; row++) {                // row loop
        for (column = 0; column < TFT_COLUMN_NUMBER; column++) { // column loop
            esp_tft_send_data(0xFF);
            esp_tft_send_data(0xFF);
        }
    }
}

/**
 * @brief:
 * @param {unsigned int} color
 * @return {*}
 */
void esp_tft_single_color(unsigned int color)
{
    unsigned int row, column;

    esp_tft_send_cmd(0x2a);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0xF0);

    esp_tft_send_cmd(0x2b);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0xF0);
    esp_tft_send_cmd(0x2C);
    // Memory write
    for (row = 0; row < TFT_LINE_NUMBER; row++) {                // row loop
        for (column = 0; column < TFT_COLUMN_NUMBER; column++) { // column loop
            esp_tft_send_data(color >> 8);
            esp_tft_send_data(color);
        }
    }
}
#if 0
/**
 * @brief:
 * @param {unsigned int} x
 * @param {unsigned int} y
 * @param {unsigned long} color
 * @param {unsigned char} word_serial_number
 * @return {*}
 */
void esp_tft_display_CN(unsigned int x, unsigned int y, unsigned long color, unsigned char word_serial_number)
{
    unsigned int column;
    unsigned char temp = 0, num = 0;

    esp_tft_send_cmd(0x2a);    
    esp_tft_send_data(x >> 8); 
    esp_tft_send_data(x);
    x = x + 15;
    esp_tft_send_data(x >> 8); 
    esp_tft_send_data(x);

    esp_tft_send_cmd(0x2b);    
    esp_tft_send_data(y >> 8); 
    esp_tft_send_data(y);
    y = y + 15;
    esp_tft_send_data(y >> 8);
    esp_tft_send_data(y);
    esp_tft_send_cmd(0x2C); // Memory write

    for (column = 0; column < 32; column++) { // column loop
        temp = word[word_serial_number][num];
        
        for (char i = 0; i < 8; i++) {
            if (temp & 0x01) {
                esp_tft_send_data(color >> 8);
                esp_tft_send_data(color);
            } else {
                esp_tft_send_data(0XFF);
                esp_tft_send_data(0XFF);
            }
            temp >>= 1;
        }
        
        num++;
    }
}

/**
 * @brief:
 * @param {unsigned int} x
 * @param {unsigned int} y
 * @param {unsigned long} color
 * @param {unsigned char} character_order
 * @return {*}
 */
void esp_tft_display_char(unsigned int x, unsigned int y, unsigned long color, unsigned char character_order)
{
    unsigned int column;
    unsigned char temp = 0, num = 0;

    esp_tft_send_cmd(0x2a);
    esp_tft_send_data(x >> 8);
    esp_tft_send_data(x);
    x = x + 7;
    esp_tft_send_data(x >> 8);
    esp_tft_send_data(x);

    esp_tft_send_cmd(0x2b);
    esp_tft_send_data(y >> 8);
    esp_tft_send_data(y);
    y = y + 15;
    esp_tft_send_data(y >> 8);
    esp_tft_send_data(y);
    esp_tft_send_cmd(0x2C);

    for (column = 0; column < 16; column++) {
        temp = character[character_order][num];

        for (char i = 0; i < 8; i++) {
            if (temp & 0x01) {
                esp_tft_send_data(color >> 8);
                esp_tft_send_data(color);
            } else {
                esp_tft_send_data(0XFF);
                esp_tft_send_data(0XFF);
            }
            temp >>= 1;
        }

        num++;
    }
}
#endif
/**
 * @brief:
 * @param {unsigned char} *ptr_pic
 * @return {*}
 */
void esp_tft_display_pic(const unsigned char *ptr_pic)
{
    unsigned long number;

    esp_tft_send_cmd(0x2a);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0x77);

    esp_tft_send_cmd(0x2b);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0x00);
    esp_tft_send_data(0x78);
    esp_tft_send_cmd(0x2C);

    for (number = 0; number < PIC_NUM; number++) {
        //            data=*ptr_pic++;
        //            data=~data;
        esp_tft_send_data(*ptr_pic++);
    }
}

int esp_tft_display_area(uint16_t xstart, uint16_t ystart, uint16_t xend, uint16_t yend)
{
    esp_tft_send_cmd(0x2a);
    esp_tft_send_data(xstart >> 8);
    esp_tft_send_data(xstart & 0xff);
    esp_tft_send_data((xend - 1) >> 8);
    esp_tft_send_data((xend - 1) & 0xff);

    esp_tft_send_cmd(0x2b);
    esp_tft_send_data(ystart >> 8);
    esp_tft_send_data(ystart & 0xff);
    esp_tft_send_data((yend - 1) >> 8);
    esp_tft_send_data((yend - 1) & 0xff);

    esp_tft_send_cmd(0x2C);

    return 0;
}

/**
 * @brief:
 * @param {uint16_t} x
 * @param {uint16_t} y
 * @param {uint16_t} color
 * @return {*}
 */
int esp_tft_draw_point(uint16_t x, uint16_t y, uint16_t color)
{
    esp_tft_send_cmd(0x2a);
    esp_tft_send_data(x >> 8);
    esp_tft_send_data(x & 0xff);
    esp_tft_send_data((x) >> 8);
    esp_tft_send_data((x)&0xff);

    esp_tft_send_cmd(0x2b);
    esp_tft_send_data(y >> 8);
    esp_tft_send_data(y & 0xff);
    esp_tft_send_data((y) >> 8);
    esp_tft_send_data((y)&0xff);

    esp_tft_send_cmd(0x2C);
    esp_tft_send_data(color >> 8);
    esp_tft_send_data(color);

    return 0;
}

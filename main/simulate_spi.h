/*
 * @Descripttion:
 * @version:
 * @Author: Newt
 * @Date: 2022-08-23 09:50:18
 * @LastEditors: Newt
 * @LastEditTime: 2022-11-12 12:58:10
 */
#ifndef SIMULATE_SPI_H
#define SIMULATE_SPI_H

#include <stdint.h>

#include "driver/gpio.h"

/* set spi mode and delay function*/
#define SPI_MODE 0
#define DELAY_US
/* add sumulate pin below here */
#define GPIO_SCK
#define GPIO_MOSI
#define GPIO_MISO
#define GPIO_CS


/* add ur enabel gpio API functions below here  */
#define ENABLE_SCK  
#define ENABLE_MOSI 
#define ENABLE_MISO
#define ENABLE_CS

/* add ur set gpio API functions below here */
#define SET_SCK_LOW gpio_set_level(GPIO_NUM_4, 0)
#define SET_SCK_HIGH gpio_set_level(GPIO_NUM_4, 1)
#define SET_MOSI_LOW gpio_set_level(GPIO_NUM_5, 0)
#define SET_MOSI_HIGH gpio_set_level(GPIO_NUM_5, 1)
#define SET_MISO_LOW
#define SET_MISO_HIGH
#define SET_CS_LOW
#define SET_CS_HIGH

int8_t simulate_spi_init(void);
int8_t simulate_spi_transmit(uint8_t *buffer, uint16_t size);

#endif /* SIMULATE_SPI_H */
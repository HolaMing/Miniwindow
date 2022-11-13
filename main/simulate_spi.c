/*
 * @Descripttion: 
 * @version: 
 * @Author: Newt
 * @Date: 2022-08-22 17:04:47
 * @LastEditors: Newt
 * @LastEditTime: 2022-11-12 13:04:10
 */
#include "simulate_spi.h"

/**
 * @brief: init simulate spi pin
 * @return {*}
 */
int8_t simulate_spi_init(void)
{  
    ENABLE_SCK;
    ENABLE_MOSI;
    ENABLE_MISO;
    ENABLE_CS;

    SET_CS_HIGH;
    if (SPI_MODE == 0 || SPI_MODE == 1) {
        SET_SCK_LOW;
    }
    else {
        SET_SCK_HIGH;        
    }

    return 0;
}

/**
 * @brief: send data
 * @param {uint8_t} *buffer
 * @param {uint16_t} size
 * @return {*}
 */
int8_t simulate_spi_transmit(uint8_t *buffer, uint16_t size)
{
    switch (SPI_MODE) {
        case 0:
            for (uint8_t i = 0; i < size; i++) {
                uint8_t value = buffer[i];
                for (uint8_t j = 0; j < 8; j++) {
                    if (value & 0x80) {
                        SET_MOSI_HIGH;
                    }
                    else {
                        SET_MOSI_LOW;
                    }
                    value = (value << 1);
                    DELAY_US;
                    SET_SCK_HIGH;
                    DELAY_US;
                    SET_SCK_LOW;
                }
            }
            break;
        case 1:
            for (uint8_t i = 0; i < size; i++) {
                uint8_t value = buffer[i];
                 for (uint8_t j = 0; j < 8; j++) {
                    SET_SCK_HIGH;
                    if (value & 0x80) {
                        SET_MOSI_HIGH;
                    }
                    else {
                        SET_MOSI_LOW;
                    }
                    value = (value << 1);
                    DELAY_US;
                    SET_SCK_LOW;
                    DELAY_US;
                 }
            }
            break;
        case 2:
            for (uint8_t i = 0; i < size; i++) {
                uint8_t value = buffer[i];
                 for (uint8_t j = 0; j < 8; j++) {
                    if (value & 0x80) {
                        SET_MOSI_HIGH;
                    }
                    else {
                        SET_MOSI_LOW;
                    }
                    value = (value << 1);
                    DELAY_US;
                    SET_SCK_LOW;
                    DELAY_US;
                    SET_SCK_HIGH;
                 }
            }
            break;
        case 3:
            for (uint8_t i = 0; i < size; i++) {
                uint8_t value = buffer[i];
                 for (uint8_t j = 0; j < 8; j++) {
                    SET_SCK_LOW;
                    if (value & 0x80) {
                        SET_MOSI_HIGH;
                    }
                    else {
                        SET_MOSI_LOW;
                    }
                    value = (value << 1);
                    DELAY_US;
                    SET_SCK_HIGH;
                    DELAY_US;
                 }
            }
            break;
        default:
            break;
    }
    return 0;
}
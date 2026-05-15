#include "rgb.h"
#include "stdlib.h"
#include <stdbool.h>
#include "tim.h"

// External RGB control registers (DO NOT MODIFY)
extern __IO uint8_t rgb_brightness_reg[2];
extern __IO uint8_t rgb1_status_reg[14][4];
extern __IO uint8_t rgb2_status_reg[14][4];
extern __IO uint8_t touch_button_status_reg[2];
extern __IO uint8_t rgb_send_complete_flag[2];

static uint8_t rgb1_color_buf[RGB_NUM + 10][24] = {0};
static uint8_t rgb2_color_buf[RGB_NUM + 10][24] = {0};
static uint8_t rgb_off_buf[RGB_NUM + 10][24]    = {0};

static void tim3_dma_init(void)
{
    // Set DMA data length (number of data transfers)
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, (RGB_NUM + 10) * 24);

    // Set the peripheral address (TIM3 CCR1 register address)
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_1, (uint32_t) & (TIM3->CCR1));

    // Clear DMA interrupt flags
    LL_DMA_ClearFlag_GI1(DMA1);  // Clear global interrupt flag for channel 1
    LL_DMA_ClearFlag_TC1(DMA1);  // Clear transfer complete flag for channel 1

    // Enable DMA transfer complete interrupt for channel 1
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_1);

    // Enable DMA request for TIM3 channel 1
    LL_TIM_EnableDMAReq_CC1(TIM3);

    // Enable all outputs of TIM3
    LL_TIM_EnableAllOutputs(TIM3);  // Enable TIM3 outputs

    // Enable TIM3 channel 1 (compare channel 1)
    LL_TIM_CC_EnableChannel(TIM3, LL_TIM_CHANNEL_CH1);  // Enable TIM3 channel 1
}

static void tim17_dma_init(void)
{
    // Set DMA data length (number of data transfers)
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, (RGB_NUM + 10) * 24);  // Set the number of data transfers for DMA

    // Set the peripheral address (TIM17 CCR1 register address)
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_2, (uint32_t) & (TIM17->CCR1));  // Set peripheral address (TIM17 CCR1)

    // Clear DMA interrupt flags
    LL_DMA_ClearFlag_GI2(DMA1);  // Clear global interrupt flag for channel 2
    LL_DMA_ClearFlag_TC2(DMA1);  // Clear transfer complete flag for channel 2

    // Enable DMA transfer complete interrupt for channel 2
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_2);  // Enable transfer complete interrupt for channel 2

    // Enable DMA request for TIM17 channel 1
    LL_TIM_EnableDMAReq_CC1(TIM17);

    // Enable all outputs of TIM17
    LL_TIM_EnableAllOutputs(TIM17);  // Enable TIM17 outputs

    // Enable TIM17 channel 1 (compare channel 1)
    LL_TIM_CC_EnableChannel(TIM17, LL_TIM_CHANNEL_CH1);  // Enable TIM17 channel 1
}

static void rgb1_send_array(uint32_t *buffer)
{
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_2, (RGB_NUM + 10) * 24);
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_2, (uint32_t)buffer);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_2);
    LL_TIM_EnableCounter(TIM17);
}

static void rgb2_send_array(uint32_t *buffer)
{
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_1, (RGB_NUM + 10) * 24);
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_1, (uint32_t)buffer);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_1);
    LL_TIM_EnableCounter(TIM3);
}

void rgb_init(void)
{
    tim3_dma_init();
    tim17_dma_init();
    for (uint8_t i = 0; i < RGB_NUM; i++) {
        for (uint8_t j = 0; j < 24; j++) {
            rgb_off_buf[i][j] = CODE_0;
        }
    }
    // 上电默认关闭所以灯
    rgb_off(RGB_DEVICE_1);
    HAL_Delay(1);
    rgb_off(RGB_DEVICE_2);
    HAL_Delay(1);
    rgb_off(RGB_DEVICE_1);
    HAL_Delay(1);
    rgb_off(RGB_DEVICE_2);
    HAL_Delay(1);
}

void rgb_off(rgb_device_t rgb_device)
{
    void (*send_rgb)(const uint8_t *) = (rgb_device == RGB_DEVICE_1) ? rgb1_send_array : rgb2_send_array;

    send_rgb(rgb_off_buf);
}

void set_rgb_color(uint8_t device, uint8_t led_id, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t i;
    uint8_t R, G, B;

    // Calculate the RGB values with brightness adjustment
    R = (uint8_t)(((uint32_t)r * rgb_brightness_reg[device] * 35) / 10000);  // 缩放到35%
    G = (uint8_t)(((uint32_t)g * rgb_brightness_reg[device] * 35) / 10000);
    B = (uint8_t)(((uint32_t)b * rgb_brightness_reg[device] * 35) / 10000);

    // Convert RGB data to PWM signal format and store in buffer
    uint8_t *buffer = (device == 0) ? rgb1_color_buf[led_id] : rgb2_color_buf[led_id];

    for (i = 0; i < 8; i++) buffer[i] = ((G & (1 << (7 - i))) ? (CODE_1) : CODE_0);     // Store G values (0-7)
    for (i = 8; i < 16; i++) buffer[i] = ((R & (1 << (15 - i))) ? (CODE_1) : CODE_0);   // Store R values (8-15)
    for (i = 16; i < 24; i++) buffer[i] = ((B & (1 << (23 - i))) ? (CODE_1) : CODE_0);  // Store B values (16-23)
}

void update_rgb_buffer(rgb_device_t rgb_device)
{
    if (rgb_device == RGB_DEVICE_1) {
        for (uint8_t i = 0; i < RGB_NUM; i++) {
            set_rgb_color(RGB_DEVICE_1, i, rgb1_status_reg[i][2], rgb1_status_reg[i][1], rgb1_status_reg[i][0]);
        }
    } else {
        for (uint8_t i = 0; i < RGB_NUM; i++) {
            set_rgb_color(RGB_DEVICE_2, i, rgb2_status_reg[i][2], rgb2_status_reg[i][1], rgb2_status_reg[i][0]);
        }
    }
}

void rgb_show(rgb_device_t rgb_device)
{
    if (rgb_device == RGB_DEVICE_1) {
        update_rgb_buffer(RGB_DEVICE_1);
        rgb1_send_array((uint32_t *)rgb1_color_buf);
    } else {
        update_rgb_buffer(RGB_DEVICE_2);
        rgb2_send_array((uint32_t *)rgb2_color_buf);
    }
}

void rgb_test(void)
{
    for (uint8_t i = 0; i < RGB_NUM; i++) {
        rgb1_status_reg[i][2] = 0x0f;
        set_rgb_color(RGB_DEVICE_1, i, rgb1_status_reg[i][2], rgb1_status_reg[i][1], rgb1_status_reg[i][0]);
    }
    for (uint8_t i = 0; i < RGB_NUM; i++) {
        rgb2_status_reg[i][2] = 0x0f;
        set_rgb_color(RGB_DEVICE_1, i, rgb1_status_reg[i][2], rgb1_status_reg[i][1], rgb1_status_reg[i][0]);
    }
    rgb1_send_array((uint32_t *)rgb1_color_buf);  // Send data to RGB1
    rgb2_send_array((uint32_t *)rgb2_color_buf);  // Send data to RGB2
    HAL_Delay(1000);

    for (uint8_t i = 0; i < RGB_NUM; i++) {
        rgb1_status_reg[i][1] = 0x0f;
        set_rgb_color(RGB_DEVICE_1, i, rgb1_status_reg[i][2], rgb1_status_reg[i][1], rgb1_status_reg[i][0]);
    }
    for (uint8_t i = 0; i < RGB_NUM; i++) {
        rgb2_status_reg[i][1] = 0x0f;
        set_rgb_color(RGB_DEVICE_2, i, rgb2_status_reg[i][2], rgb2_status_reg[i][1], rgb2_status_reg[i][0]);
    }
    rgb1_send_array((uint32_t *)rgb1_color_buf);  // Send data to RGB1
    rgb2_send_array((uint32_t *)rgb2_color_buf);  // Send data to RGB2
    HAL_Delay(1000);
    for (uint8_t i = 0; i < RGB_NUM; i++) {
        rgb1_status_reg[i][0] = 0x0f;
        set_rgb_color(RGB_DEVICE_1, i, rgb1_status_reg[i][2], rgb1_status_reg[i][1], rgb1_status_reg[i][0]);
    }
    for (uint8_t i = 0; i < RGB_NUM; i++) {
        rgb2_status_reg[i][0] = 0x0f;
        set_rgb_color(RGB_DEVICE_2, i, rgb2_status_reg[i][2], rgb2_status_reg[i][1], rgb2_status_reg[i][0]);
    }
    rgb1_send_array((uint32_t *)rgb1_color_buf);  // Send data to RGB1
    rgb2_send_array((uint32_t *)rgb2_color_buf);  // Send data to RGB2
}

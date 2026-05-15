/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "iwdg.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "i2c_ex.h"
#include "rgb.h"
#include "myflash.h"
#include "string.h"
#include "stm32g0xx_ll_adc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
__IO uint8_t touch_button_status_reg[4] = {0};
__IO uint8_t rgb_brightness_reg[2]      = {0};
__IO uint8_t rgb1_status_reg[14][4]     = {0};
__IO uint8_t rgb2_status_reg[14][4]     = {0};
__IO uint8_t spk_restart_reg            = 0;
__IO uint16_t usb_5v_reg                = 0;
__IO uint8_t flash_write_back_reg       = 0;
__IO uint8_t iap_update_reg             = 0;
__IO uint8_t sw_ver_reg                 = FIRMWARE_VERSION;
__IO uint8_t i2c_addr_reg               = I2C_ADDR_REG_DEFAULT;

__IO uint32_t i2c_stop_timeout_delay       = 0;
__IO uint32_t spk_restart_tick             = 0;
__IO uint8_t rgb_brightness_change_flag[2] = {0};
__IO uint8_t rgb_value_change_flag[2]      = {0};
__IO uint8_t rgb_send_complete_flag[2]     = {1, 1};
__IO uint32_t last_adc_tick                = 0;

static GPIO_TypeDef *const gpio_ports[TOUCH_BUTTON_COUNT] = {TP_4_GPIO_Port, TP_1_GPIO_Port, TP_2_GPIO_Port,
                                                             TP_3_GPIO_Port};

static const uint32_t gpio_pins[TOUCH_BUTTON_COUNT] = {TP_4_Pin, TP_1_Pin, TP_2_Pin, TP_3_Pin};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void iap_set()
{
    uint8_t i;
    uint32_t *pVecTab = (uint32_t *)(0x20000000);
    for (i = 0; i < 48; i++) {
        *(pVecTab++) = *(__IO uint32_t *)(APPLICATION_ADDRESS + (i << 2));
    }
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_SYSCFG_REMAPMEMORY_SRAM();
}

static void echo_pyramid_init(void)
{
    uint8_t flag1 = 0, flag2 = 0;

    // Process RGB1 brightness
    uint8_t rgb1_brightness = get_echo_pyramid_rgb1_brightness();
    if (rgb1_brightness == 0xFF) {
        flag1++;
        rgb_brightness_reg[0] = RGB_BRIGHTNESS_DEFAULT;
    } else {
        rgb_brightness_reg[0] = rgb1_brightness;
    }

    // Process RGB2 brightness
    uint8_t rgb2_brightness = get_echo_pyramid_rgb2_brightness();
    if (rgb2_brightness == 0xFF) {
        flag2++;
        rgb_brightness_reg[1] = RGB_BRIGHTNESS_DEFAULT;
    } else {
        rgb_brightness_reg[1] = rgb2_brightness;
    }

    // Update RGB1 settings if modified
    if (flag1) {
        set_echo_pyramid_rgb1_brightness(rgb_brightness_reg[0]);
    }

    // Update RGB2 settings if modified
    if (flag2) {
        set_echo_pyramid_rgb2_brightness(rgb_brightness_reg[1]);
    }

    // Process I2C address
    uint8_t i2c_addr = get_i2c_addr();
    if (i2c_addr == 0xFF) {
        i2c_addr_reg = I2C_ADDR_REG_DEFAULT;
        set_i2c_addr(I2C_ADDR_REG_DEFAULT);
    } else {
        i2c_addr_reg = i2c_addr;
    }
}

void Slave_Complete_Callback(uint8_t *rx_data, uint16_t len)
{
    uint8_t rx_buf[64]  = {0};
    uint8_t tx_buf[64]  = {0};
    uint8_t rx_mark[64] = {0};
    if (len > 1) {
        if (rx_data[0] >= RGB1_BRIGHTNESS_REG_ADDR && rx_data[0] <= RGB2_BRIGHTNESS_REG_ADDR && len <= 3) {
            uint8_t offset = rx_data[0] - RGB1_BRIGHTNESS_REG_ADDR;
            uint8_t limit  = len - 1;

            for (uint8_t i = 0; i < limit; i++) {
                uint8_t index  = offset + i;
                rx_buf[index]  = rx_data[i + 1];
                rx_mark[index] = 1;
            }

            for (uint8_t i = 0; i < 2; i++) {
                if (rx_mark[i] && rx_buf[i] <= 100 && rgb_brightness_reg[i] != rx_buf[i]) {
                    rgb_brightness_reg[i] = rx_buf[i];
                    (i == 0) ? (rgb_brightness_change_flag[0] = 1) : (rgb_brightness_change_flag[1] = 1);
                }
            }
        } else if (rx_data[0] >= RGB1_STATUS_REG_ADDR && rx_data[0] <= (RGB1_STATUS_REG_ADDR + 55) && len <= 57) {
            uint8_t offset = rx_data[0] - RGB1_STATUS_REG_ADDR;
            uint8_t limit  = len - 1;

            for (uint8_t i = 0; i < limit; i++) {
                uint8_t index  = offset + i;
                rx_buf[index]  = rx_data[i + 1];
                rx_mark[index] = 1;
            }

            for (uint8_t i = 0; i < 56; i++) {
                if (rx_mark[i]) {
                    uint8_t rgb_index = i / 4;
                    uint8_t sub_index = i % 4;
                    if (rgb1_status_reg[rgb_index][sub_index] != rx_buf[i]) {
                        rgb1_status_reg[rgb_index][sub_index] = rx_buf[i];
                        rgb_value_change_flag[0]              = 1;
                    }
                }
            }

        } else if (rx_data[0] >= RGB2_STATUS_REG_ADDR && rx_data[0] <= (RGB2_STATUS_REG_ADDR + 55) && len <= 57) {
            uint8_t offset = rx_data[0] - RGB2_STATUS_REG_ADDR;
            uint8_t limit  = len - 1;
            for (uint8_t i = 0; i < limit; i++) {
                uint8_t index  = offset + i;
                rx_buf[index]  = rx_data[i + 1];
                rx_mark[index] = 1;
            }

            for (uint8_t i = 0; i < 56; i++) {
                if (rx_mark[i]) {
                    uint8_t rgb_index = i / 4;
                    uint8_t sub_index = i % 4;

                    if (rgb2_status_reg[rgb_index][sub_index] != rx_buf[i]) {
                        rgb2_status_reg[rgb_index][sub_index] = rx_buf[i];
                        rgb_value_change_flag[1]              = 1;
                    }
                }
            }

        } else if (rx_data[0] == SPK_RESTART_REG_ADDR && len == 2) {
            if (rx_data[1] == SPK_RESTART_VALUE) {
                if (spk_restart_reg != SPK_RESTART_VALUE) {
                    spk_restart_reg = SPK_RESTART_VALUE;
                }
            }
        } else if (rx_data[0] == FLASH_WRITE_BACK_REG_ADDR && len == 2) {
            if (rx_data[1] == FLASH_WRITE_BACK_RGB1_VALUE) {
                if (rgb_brightness_reg[0] != get_echo_pyramid_rgb1_brightness()) {
                    set_echo_pyramid_rgb1_brightness(rgb_brightness_reg[0]);
                }
            } else if (rx_data[1] == FLASH_WRITE_BACK_RGB2_VALUE) {
                if (rgb_brightness_reg[1] != get_echo_pyramid_rgb2_brightness()) {
                    set_echo_pyramid_rgb2_brightness(rgb_brightness_reg[1]);
                }
            }
        } else if (rx_data[0] == IAP_UPDATE_REG_ADDR && len == 2) {
            if (rx_data[1] == IAP_UPDATE_VALUE) {
                NVIC_SystemReset();
            }
        } else if (rx_data[0] == I2C_ADDR_REG_ADDR && len == 2) {
            if (rx_data[1] >= I2C_ADDR_REG_MIN && rx_data[1] <= I2C_ADDR_REG_MAX) {
                if (i2c_addr_reg != rx_data[1]) {
                    i2c_addr_reg = rx_data[1];
                    set_i2c_addr(rx_data[1]);
                    user_i2c_init();
                }
            }
        }
    }
    if (len == 1) {
        if (rx_data[0] <= 0x03) {
            uint8_t *src_ptr = (uint8_t *)(touch_button_status_reg + rx_data[0] - 0x00);
            uint8_t data_len = TOUCH_BUTTON_COUNT - rx_data[0];
            memcpy(tx_buf, src_ptr, data_len);
            i2c2_set_send_data(tx_buf, data_len);
        } else if (rx_data[0] >= 0x10 && rx_data[0] <= 0x11) {
            uint8_t *src_ptr = (uint8_t *)(rgb_brightness_reg + rx_data[0] - 0x10);
            uint8_t data_len = 2 + 0x10 - rx_data[0];
            memcpy(tx_buf, src_ptr, data_len);
            i2c2_set_send_data(tx_buf, data_len);
        } else if (rx_data[0] >= 0x20 && rx_data[0] <= 0x57) {
            uint8_t *src_ptr = (uint8_t *)rgb1_status_reg + (rx_data[0] - 0x20);
            uint8_t data_len = 56 + 0x20 - rx_data[0];
            memcpy(tx_buf, src_ptr, data_len);
            i2c2_set_send_data(tx_buf, data_len);
        } else if (rx_data[0] >= 0x60 && rx_data[0] <= 0x97) {
            uint8_t *src_ptr = (uint8_t *)rgb2_status_reg + (rx_data[0] - 0x60);
            uint8_t data_len = 56 + 0x60 - rx_data[0];
            memcpy(tx_buf, src_ptr, data_len);
            i2c2_set_send_data(tx_buf, data_len);
        } else if (rx_data[0] >= USB_5V_REG_ADDR && rx_data[0] <= (USB_5V_REG_ADDR + 1)) {
            uint8_t usb_bytes[2] = {(uint8_t)(usb_5v_reg & 0xFF), (uint8_t)(usb_5v_reg >> 8)};
            uint8_t offset       = rx_data[0] - USB_5V_REG_ADDR;
            memcpy(tx_buf, usb_bytes + offset, 2 - offset);
            i2c2_set_send_data(tx_buf, 2 - offset);
        } else if (rx_data[0] == 0xFE) {
            tx_buf[0] = sw_ver_reg;
            i2c2_set_send_data(tx_buf, 1);
        } else if (rx_data[0] == 0xFF) {
            tx_buf[0] = i2c_addr_reg;
            i2c2_set_send_data(tx_buf, 1);
        }
    }
}

static void update_button_status(void)
{
    for (uint8_t i = 0; i < TOUCH_BUTTON_COUNT; i++) {
        uint8_t new_status =
            (LL_GPIO_IsInputPinSet(gpio_ports[i], gpio_pins[i]) == 0) ? TOUCH_BUTTON_PRESSED : TOUCH_BUTTON_RELEASED;

        if (touch_button_status_reg[i] != new_status) {
            touch_button_status_reg[i] = new_status;
        }
    }
}

static void app_handle_spk_restart(void)
{
    uint32_t tick = HAL_GetTick();
    if (spk_restart_tick == 0) {
        spk_restart_tick = tick;
        LL_GPIO_ResetOutputPin(SPK_RST_GPIO_Port, SPK_RST_Pin);
    } else if (tick - spk_restart_tick >= SPK_RESTART_TIME) {
        LL_GPIO_SetOutputPin(SPK_RST_GPIO_Port, SPK_RST_Pin);
        spk_restart_tick = 0;
        spk_restart_reg  = 0;
    }
}

static uint16_t user_adc_get_adc_value(uint32_t adc_channel)
{
    uint8_t valid_count            = 0;  // Record the number of successful collections / 记录成功采集的次数
    uint32_t sum                   = 0;  // Accumulator for the samples / 样本累加器
    ADC_ChannelConfTypeDef sConfig = {0};

    // Clear the Channel Selection Register (Critical for STM32G0/C0 series to reset sequence)
    // 清除通道选择寄存器（对于 STM32G0/C0 系列重置序列至关重要）
    hadc1.Instance->CHSELR = 0;

    sConfig.Channel      = adc_channel;
    sConfig.Rank         = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_79CYCLES_5;

    // Configure the ADC channel. If configuration fails, return error immediately.
    // 配置 ADC 通道。如果配置失败，直接返回错误。
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return 0xFFFF;
    }

    // Loop to sample 20 times / 循环采集 20 次
    for (uint8_t i = 0; i < 20; i++) {
        HAL_ADC_Start(&hadc1);

        // Wait for conversion completion, timeout set to 2ms (sufficient for single conversion)
        // 等待转换完成，超时设为 2ms (足够单次转换了)
        if (HAL_ADC_PollForConversion(&hadc1, 2) == HAL_OK) {
            sum += HAL_ADC_GetValue(&hadc1);
            valid_count++;
        } else {
            // If timeout occurs, stop to reset state and skip this sample (continue to next loop)
            // 如果发生超时，停止以复位状态并跳过此次采样（继续下一次循环）
            HAL_ADC_Stop(&hadc1);
        }
    }

    HAL_ADC_Stop(&hadc1);  // End sampling, stop ADC / 采集结束，停止 ADC

    // Calculate average only if there are enough successful samples
    // 只有当成功采集次数足够多才计算平均值
    if (valid_count > 0) {
        return (uint16_t)(sum / valid_count);  // The divisor is the actual successful count / 除数是实际成功的次数
    } else {
        return 0xFFFF;  // All failed, return error / 全部失败，返回错误
    }
}

void user_adc_update_usb_5v(void)
{
    uint32_t temp            = 0;
    uint16_t adc_raw         = 0;
    uint16_t vref_mv         = 0;
    uint16_t usb_5v_reg_temp = 0;

    // 1. 获取参考电压
    adc_raw = user_adc_get_adc_value(ADC_CHANNEL_VREFINT);
    if (adc_raw == 0xFFFF) {
        return;  // VrefInt 失败，直接退出，保留上次有效值
    }
    vref_mv = __HAL_ADC_CALC_VREFANALOG_VOLTAGE(adc_raw, LL_ADC_RESOLUTION_12B);

    // 2. 获取 USB 5V 电压
    usb_5v_reg_temp = user_adc_get_adc_value(ADC_CHANNEL_3);
    if (usb_5v_reg_temp == 0xFFFF) {
        return;  // USB 5V 采集失败，保留上次有效值
    }

    // 3. 计算实际电压并更新全局变量
    temp       = __HAL_ADC_CALC_DATA_TO_VOLTAGE(vref_mv, usb_5v_reg_temp, LL_ADC_RESOLUTION_12B);
    usb_5v_reg = (uint16_t)(temp * 2);
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* USER CODE BEGIN 1 */
    iap_set();
    echo_pyramid_init();
    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_I2C2_Init();
    MX_IWDG_Init();
    MX_TIM3_Init();
    MX_TIM17_Init();
    MX_ADC1_Init();
    /* USER CODE BEGIN 2 */
    rgb_init();
    user_i2c_init();
    i2c2_it_enable();
    // HAL_Delay(1000);
    // rgb_test();
    HAL_ADCEx_Calibration_Start(&hadc1);
    last_adc_tick = HAL_GetTick();
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        update_button_status();
        if (rgb_value_change_flag[RGB_DEVICE_1] || rgb_brightness_change_flag[RGB_DEVICE_1]) {
            if (rgb_send_complete_flag[RGB_DEVICE_1]) {
                rgb_send_complete_flag[RGB_DEVICE_1]     = 0;
                rgb_value_change_flag[RGB_DEVICE_1]      = 0;
                rgb_brightness_change_flag[RGB_DEVICE_1] = 0;
                rgb_show(RGB_DEVICE_1);
            }
        }

        if (rgb_value_change_flag[RGB_DEVICE_2] || rgb_brightness_change_flag[RGB_DEVICE_2]) {
            if (rgb_send_complete_flag[RGB_DEVICE_2]) {
                rgb_send_complete_flag[RGB_DEVICE_2]     = 0;
                rgb_value_change_flag[RGB_DEVICE_2]      = 0;
                rgb_brightness_change_flag[RGB_DEVICE_2] = 0;
                rgb_show(RGB_DEVICE_2);
            }
        }

        if (spk_restart_reg == SPK_RESTART_VALUE) {
            app_handle_spk_restart();
        }

        i2c_timeout_counter = 0;
        if (i2c_stop_timeout_flag) {
            if (i2c_stop_timeout_delay < HAL_GetTick()) {
                i2c_stop_timeout_counter++;
                i2c_stop_timeout_delay = HAL_GetTick() + 10;
            }
        }
        if (i2c_stop_timeout_counter > 50) {
            LL_I2C_DeInit(I2C2);
            LL_I2C_DisableAutoEndMode(I2C2);
            LL_I2C_Disable(I2C2);
            LL_I2C_DisableIT_ADDR(I2C2);
            user_i2c_init();
            i2c2_it_enable();
            HAL_Delay(500);
        }

        if (HAL_GetTick() - last_adc_tick >= 1000) {
            user_adc_update_usb_5v();
            last_adc_tick = HAL_GetTick();
        }

        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
        LL_IWDG_ReloadCounter(IWDG);
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Configure the main internal regulator output voltage
     */
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSIDiv              = RCC_HSI_DIV1;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.LSIState            = RCC_LSI_ON;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM            = RCC_PLLM_DIV1;
    RCC_OscInitStruct.PLL.PLLN            = 8;
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLR            = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"

#include "stm32g0xx_ll_i2c.h"
#include "stm32g0xx_ll_iwdg.h"
#include "stm32g0xx_ll_tim.h"
#include "stm32g0xx_ll_system.h"
#include "stm32g0xx_ll_gpio.h"
#include "stm32g0xx_ll_exti.h"
#include "stm32g0xx_ll_bus.h"
#include "stm32g0xx_ll_cortex.h"
#include "stm32g0xx_ll_rcc.h"
#include "stm32g0xx_ll_utils.h"
#include "stm32g0xx_ll_pwr.h"
#include "stm32g0xx_ll_dma.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
/**
 * @brief Touch button status enumeration.
 *
 * Defines the possible states of a touch button.
 */
typedef enum {
    TOUCH_BUTTON_RELEASED = 0x00, /**< Button is not pressed */
    TOUCH_BUTTON_PRESSED  = 0x01, /**< Button is pressed */
} touch_button_status_t;

/**
 * @brief RGB show mode enumeration.
 *
 * Defines the possible display modes for RGB lighting.
 */
typedef enum {
    RGB_SHOW_MODE_CUSTOM = 0x00, /**< Custom mode */
    RGB_SHOW_MODE_SYNC   = 0x01, /**< Synchronized mode */
} rgb_show_mode_t;

/**
 * @brief Flash storage mode enumeration.
 *
 * Defines whether data is saved to internal flash or not.
 */
typedef enum {
    FLASH_NOT_SAVED     = 0x00, /**< Do not save to internal flash */
    FLASH_SAVE_INTERNAL = 0x01, /**< Save to internal flash */
} flash_storage_mode_t;

/**
 * @brief RGB device enumeration.
 *
 * Defines the available RGB devices.
 */
typedef enum {
    RGB_DEVICE_1 = 0x00, /**< RGB device 1 */
    RGB_DEVICE_2 = 0x01, /**< RGB device 2 */
} rgb_device_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SPK_RST_Pin       LL_GPIO_PIN_7
#define SPK_RST_GPIO_Port GPIOB
#define TP_1_Pin          LL_GPIO_PIN_0
#define TP_1_GPIO_Port    GPIOA
#define TP_2_Pin          LL_GPIO_PIN_1
#define TP_2_GPIO_Port    GPIOA
#define TP_3_Pin          LL_GPIO_PIN_4
#define TP_3_GPIO_Port    GPIOA
#define TP_4_Pin          LL_GPIO_PIN_5
#define TP_4_GPIO_Port    GPIOA

/* USER CODE BEGIN Private defines */
#define APPLICATION_ADDRESS         (0x08001800)
#define FIRMWARE_VERSION            (0x02)
#define RGB_BRIGHTNESS_MAX          (0x64)
#define RGB_BRIGHTNESS_DEFAULT      (0x32)
#define SPK_RESTART_VALUE           (0x01)
#define FLASH_WRITE_BACK_RGB1_VALUE (0x01)
#define FLASH_WRITE_BACK_RGB2_VALUE (0x02)
#define IAP_UPDATE_VALUE            (0x01)
#define I2C_ADDR_REG_DEFAULT        (0x1A)
#define I2C_ADDR_REG_MAX            (0x77)
#define I2C_ADDR_REG_MIN            (0x08)
#define GPIO_LOW()                  GPIOA->BRR = GPIO_PIN_5
#define GPIO_HIGH()                 GPIOA->BSRR = GPIO_PIN_5
#define SPK_RESTART_TIME            (100)
#define BOOTLOADER_ADDRESS          ((uint32_t)0x08000000)
#define TOUCH_BUTTON_COUNT          (4)

// reg address
#define TOUCH_BUTTON_STATUS_REG_ADDR (0x00)
#define RGB1_BRIGHTNESS_REG_ADDR     (0x10)
#define RGB2_BRIGHTNESS_REG_ADDR     (0x11)
#define RGB1_STATUS_REG_ADDR         (0x20)
#define RGB2_STATUS_REG_ADDR         (0x60)
#define SPK_RESTART_REG_ADDR         (0xA0)
#define USB_5V_REG_ADDR              (0xB0)
#define FLASH_WRITE_BACK_REG_ADDR    (0xF0)
#define IAP_UPDATE_REG_ADDR          (0xFD)
#define SW_VER_REG_ADDR              (0xFE)
#define I2C_ADDR_REG_ADDR            (0xFF)
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

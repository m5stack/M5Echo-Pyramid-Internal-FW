#include "myflash.h"

/**
 * @brief  Get the page number based on the address
 * @param  addr Memory address
 * @retval Returns the page number
 */
static uint32_t GetPage(uint32_t addr)
{
    return (addr - STM32G0xx_FLASH_PAGE0_STARTADDR) / FLASH_PAGE_SIZE;
}

/**
 * @brief  Modify a specific byte of an 8-byte value
 * @param  data The data to modify
 * @param  byte_index The index of the byte to modify
 * @param  new_value The new value to set for the byte
 * @retval The modified value
 */
static void set_byte_in_uint64(uint64_t *data, uint8_t byte_index, uint8_t new_value)
{
    *data &= ~((uint64_t)(0xFF) << (byte_index * 8));
    // Then set the new byte value
    *data |= (uint64_t)new_value << (byte_index * 8);
}

/**
 * @brief  Read a double word from memory
 * @param  address Memory address
 * @retval The value read from memory
 */
static uint64_t my_flash_read_double_word(uint32_t address)
{
    return *((__IO uint64_t *)(address));
}

/**
 * @brief  Erase a page in flash memory
 * @param  page_address Memory address of the page to erase
 * @retval true if successful, false if failed
 */
static bool my_flash_earse_pages(uint32_t page_address)
{
    uint32_t page_error = 0;                     // Page_error will be set to the address of the error if one occurs
    FLASH_EraseInitTypeDef my_flash;             // Define the erase location
    my_flash.TypeErase = FLASH_TYPEERASE_PAGES;  // Specify page erase type
    my_flash.Page      = GetPage(page_address);
    my_flash.NbPages   = 1;  // Number of pages to erase
    HAL_FLASH_Unlock();
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&my_flash, &page_error);
    HAL_FLASH_Lock();
    if (status == HAL_OK) {
        return true;
    }
    return false;
}

/**
 * @brief  Write a double word to flash memory
 * @param  address Memory address
 * @param  data The data to write
 * @retval true if successful, false if failed
 */
static bool my_flash_write_double_word(uint32_t address, uint64_t data)
{
    HAL_FLASH_Unlock();
    HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, data);
    HAL_FLASH_Lock();
    if (status == HAL_OK) {
        return true;
    }
    return false;
}

bool set_echo_pyramid_rgb1_brightness(uint8_t brightness)
{
    __disable_irq();
    uint64_t temp = my_flash_read_double_word(STM32G0xx_FLASH_PAGE13_STARTADDR);
    set_byte_in_uint64(&temp, 0, brightness);
    my_flash_earse_pages(STM32G0xx_FLASH_PAGE13_STARTADDR);
    my_flash_write_double_word(STM32G0xx_FLASH_PAGE13_STARTADDR, temp);
    __enable_irq();
    uint8_t brightness_data = get_echo_pyramid_rgb1_brightness();
    if (brightness_data == brightness) {
        return true;
    }
    return false;
}
uint8_t get_echo_pyramid_rgb1_brightness(void)
{
    return *((__IO uint8_t *)(ECHO_PYRAMID_RGB1_BRIGHTNESS_ADDR));
}

bool set_echo_pyramid_rgb2_brightness(uint8_t brightness)
{
    __disable_irq();
    uint64_t temp = my_flash_read_double_word(STM32G0xx_FLASH_PAGE14_STARTADDR);
    set_byte_in_uint64(&temp, 0, brightness);
    my_flash_earse_pages(STM32G0xx_FLASH_PAGE14_STARTADDR);
    my_flash_write_double_word(STM32G0xx_FLASH_PAGE14_STARTADDR, temp);
    __enable_irq();
    uint8_t brightness_data = get_echo_pyramid_rgb2_brightness();
    if (brightness_data == brightness) {
        return true;
    }
    return false;
}
uint8_t get_echo_pyramid_rgb2_brightness(void)
{
    return *((__IO uint8_t *)(ECHO_PYRAMID_RGB2_BRIGHTNESS_ADDR));
}

bool set_i2c_addr(uint8_t data)
{
    __disable_irq();
    uint64_t temp = my_flash_read_double_word(STM32G0xx_FLASH_PAGE15_STARTADDR);
    set_byte_in_uint64(&temp, 0, data);
    my_flash_earse_pages(STM32G0xx_FLASH_PAGE15_STARTADDR);
    my_flash_write_double_word(STM32G0xx_FLASH_PAGE15_STARTADDR, temp);
    __enable_irq();
    uint8_t i2c_addr_data = get_i2c_addr();
    if (i2c_addr_data == data) {
        return true;
    }
    return false;
}
uint8_t get_i2c_addr(void)
{
    return *((__IO uint8_t *)(I2C_ADDR));
}

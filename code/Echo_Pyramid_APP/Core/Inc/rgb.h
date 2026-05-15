/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __RGB_H
#define __RGB_H

#ifdef __cplusplus

extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stdio.h"
#include "string.h"

#define CODE_1  (40)
#define CODE_0  (19)
#define RGB_NUM (14)

void rgb_init(void);
void update_rgb_buffer(rgb_device_t rgb_device);
void rgb_show(rgb_device_t rgb_device);
void rgb_off(rgb_device_t rgb_device);

#ifdef __cplusplus
}
#endif

#endif /*__ RGB_H */

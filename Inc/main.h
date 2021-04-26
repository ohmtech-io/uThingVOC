/***************************************************************************
*** MIT License ***
*
*** Copyright (c) 2020 Daniel Mancuso - OhmTech.io **
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.     
****************************************************************************/
#pragma once

#include "stm32l4xx_hal.h"
#include <stdio.h>

#define BLUE_LED_Pin GPIO_PIN_0
#define BLUE_LED_GPIO_Port GPIOA
#define RED_LED_Pin GPIO_PIN_1
#define RED_LED_GPIO_Port GPIOA


void Error_Handler(void);

#if (APP_DEBUG_LEVEL > 0)
#define  UartLog(...)   printf("\rDEBUG : ") ;\
                        printf(__VA_ARGS__);\
                        printf("\n\r");
#else
#define UartLog(...)                         
#endif

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

#include "stm32f0xx_hal.h"

#define PA3_Pin GPIO_PIN_3
#define PA3_GPIO_Port GPIOA
#define PA4_Pin GPIO_PIN_4
#define PA4_GPIO_Port GPIOA
#define BLUE_LED_Pin GPIO_PIN_12
#define BLUE_LED_GPIO_Port GPIOB
#define RED_LED_Pin GPIO_PIN_13
#define RED_LED_GPIO_Port GPIOB
#define PB6_Pin GPIO_PIN_6
#define PB6_GPIO_Port GPIOB
#define PB7_Pin GPIO_PIN_7
#define PB7_GPIO_Port GPIOB

void Error_Handler(void);

#if (APP_DEBUG_LEVEL > 0)
#define  UartLog(...)   printf("\rDEBUG : ") ;\
                        printf(__VA_ARGS__);\
                        printf("\n\r");
#else
#define UartLog(...)                         
#endif

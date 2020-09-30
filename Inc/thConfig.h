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
#include <stdint.h>
#include <stdbool.h>

typedef enum {
	JSON 	= 0,
	HUMAN 	= 1,
	CSV		= 2,
	BINARY	= 3
} outFormat_t;

#pragma pack ( 1 ) 
typedef struct _configs_t {
	uint8_t		reportingPeriodIdx;
	uint32_t	reportingPeriod; //ms
	bool		ledEnabled;
	outFormat_t	format;
	char 		serialNumberStr[17];
	float		temperatureOffset;	
} configs_t; 


#define SHELL_BUFFER_LENGTH 256
typedef struct shellBuffer_t {
	char Buf[SHELL_BUFFER_LENGTH];
	uint8_t idx;
	bool newLine;
} shellBuffer_t;


void processVCPinput(void);

int uprintf(const char *format, ...);

void initConfig(void);

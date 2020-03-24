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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include "thConfig.h"
#include "main.h" //for the UART_LOG
#include "usbd_cdc_if.h"
#include "version.h"
#include "jsmn.h"
#include "flashSave.h"



configs_t thConfig = { .format = JSON,
					 .reportingPeriodIdx = 0,
					 .reportingPeriod 	 = 3, /*default*/
					 .ledEnabled		 = true,
					 .temperatureOffset  = 0,
					};


static const char *FORMAT_STRING[] = {
    "JSON", "HUMAN", "CSV", "BINARY",
};

static const char *HW_ID = { "uThing::VOC rev.A"};

static const char *PERIOD_STRING[] = {
    "3 sec",  /*0*/
    "10 sec", 
    "30 sec", 
    "1 min",
    "10 min",
    "30 min",
    "1 hour"  /*6*/
};

static uint32_t hash32(uint32_t a);
static void processChar(uint8_t input);
static int processJson(const char *buffer);
static int jsoneq(const char *json, jsmntok_t *tok, const char *s);
static void jsonPrintStatus(void);
static char toUpperCase(const char ch);
static void jsonPrintDevInfo(void);

#define outBufferSize 	512
static char outBuffer[outBufferSize];
shellBuffer_t shellBuffer;


/* Obtain the serial number from the MCU UID */
void initConfig(void)
{
	uint32_t UID0, UID1;
  	/*	HAL_GetUIDw0: UID[31:0]: X and Y coordinates on the wafer expressed in BCD format
    	HAL_GetUIDw1: Bits 31:8 UID[63:40] : LOT_NUM[23:0] - Lot number (ASCII encoded)
        	          Bits 7:0  UID[39:32] : WAF_NUM[7:0]  - Wafer number (8-bit unsigned number)
  	*/ 
	UID0 = HAL_GetUIDw0();  
	UID1 = HAL_GetUIDw1();

	snprintf(thConfig.serialNumberStr, 17, "%lX%lX", hash32(UID1), hash32(UID0));

	/* Load config from Flash if available, otherwise keep default*/
	loadConfig(&thConfig);
}


int uprintf(const char *format, ...)
{
	va_list arguments;

	va_start(arguments, format);

	int len = vsprintf(outBuffer, format, arguments);
	// int len = snprintf(outBuffer, 100, format, arguments);
	// UartLog("len: %i, outBuffer: %s", len, outBuffer);
	
	// printf("%s", outBuffer);
	
	va_end(arguments);
	
	uint8_t res = CDC_Transmit_FS((uint8_t *)&outBuffer, len);
	if (res == USBD_BUSY)
	{
		UartLog("USB_BUSY");
	}	

	return len;
}

void processVCPinput(void)
{
	if (shellBuffer.newLine){
		if (shellBuffer.idx == 2){
			/* only 1 character, let's ignore longer strings (ModemManager or console echo issues) */
			processChar(shellBuffer.Buf[0]);
		} 
		else if ((shellBuffer.idx > 2) && (shellBuffer.Buf[0] == '{')){
			/* we have probably a JSON object */
			processJson(shellBuffer.Buf);
		}

		/* trailing end of lines confuse the jsmn parser, let's clean after always..*/
		for (int i = 0; i < sizeof(shellBuffer.Buf); ++i)
		{
			shellBuffer.Buf[i] = 0;
		}
		/* get ready for a new message */
		shellBuffer.idx = 0;
		shellBuffer.newLine = false;
	} 
}

static void showConfig()
{
	uint32_t timestamp = HAL_GetTick();
	uprintf("\n\r-------------------------------------------------------- \
			\n\r***  Device: *\"%s\"* -- Status: \
			\n\r Reporing period: %s, Format: %s, Temp.Offset: %2.2f C, Uptime: %lu ms, Serial #: %s, FW: v%d.%d.%d\
			\n\r-------------------------------------------------------- \n\r", 
			HW_ID,
			PERIOD_STRING[thConfig.reportingPeriodIdx], 
			FORMAT_STRING[thConfig.format], 
			thConfig.temperatureOffset,
			timestamp,
			thConfig.serialNumberStr,
			VERSION_MAJOR,
			VERSION_MINOR,
			VERSION_PATCH);
}	

void printPeriod()
{
	uprintf("\n\r***  Config: Sampling period set to %s.\n\r", 
			PERIOD_STRING[thConfig.reportingPeriodIdx]);
}


void processChar(uint8_t input)
{
	switch (toupper(input))
	{
		case 'J':
			thConfig.format = JSON;
			uprintf("\n\r*** Config: Set output format to JSON.\n\r");
			break;
		case 'M':
			thConfig.format = HUMAN;
			uprintf("\n\r*** Config: Set output format to Human Readable.\n\r");
			break;	
		case 'C':
			thConfig.format = CSV;
			uprintf("\n\r*** Config: Set output format to CSV. \
					\n\rFormat: [temperature], [pressure], [humitidy], [gasResistance], [IAQ], [accuracy], [eqCO2], [eqBreathVOC]\n\r");
			break;	
		case 'D':
			thConfig.ledEnabled = false;
			uprintf("\n\rConfig: Disable LED\n\r");
			break;		
		case 'E':
			thConfig.ledEnabled = true;
			uprintf("\n\rConfig: Enable LED\n\r");
			break;	
		case 'S':
			showConfig();
			break;
		case '1':
			thConfig.reportingPeriodIdx = 0; //3s
			thConfig.reportingPeriod = 3;
			printPeriod();
			break;
		case '2':
			thConfig.reportingPeriodIdx = 1; //10s
			thConfig.reportingPeriod = 10;
			printPeriod();
			break;
		case '3':
			thConfig.reportingPeriodIdx = 2; //30s
			thConfig.reportingPeriod = 30;
			printPeriod();
			break;
		case '4':
			thConfig.reportingPeriodIdx = 3; //1m
			thConfig.reportingPeriod = 60;
			printPeriod();
			break;
		case '5':
			thConfig.reportingPeriodIdx = 4; //10m
			thConfig.reportingPeriod = 600;
			printPeriod();
			break;
		case '6':
			thConfig.reportingPeriodIdx = 5; //30m
			thConfig.reportingPeriod = 1800;
			printPeriod();
			break;
		case '7':
			thConfig.reportingPeriodIdx = 6; //1h
			thConfig.reportingPeriod = 3600;
			printPeriod();
			break;

		default:
			uprintf("\n\r------------------------------------------------------- \
					\n\r***  Invalid option. \
					\n\r Use:              [m] Human readable, [j] JSON, [c] CSV, [s] Status, [e/d] enable/disable LED\
					\n\r Reporting Period: [1] 3 sec, [2] 10 sec, [3] 30 sec, [4] 1 min, [5] 10 min, [6] 30 min, [7] 1 hour. \
					\n\r-------------------------------------------------------- \n\r");
	}
}		

static int processJson(const char *buffer)
{
	jsmn_parser p;
	jsmntok_t tokens[10]; /* We expect no more than 10 tokens */
	char keyFirstChar = 0;

	jsmn_init(&p);
	int ret = jsmn_parse(&p, buffer, strlen(buffer), tokens,
	             sizeof(tokens) / sizeof(tokens[0]));
	
	if (ret < 0) {
		printf("Failed to parse JSON: %d\n\r", ret);
		return 1;
	}

	/* Assume the top-level element is an object */
	if (ret < 1 || tokens[0].type != JSMN_OBJECT) {
		printf("Object expected\n\r");
		return 1;
	}

 	/* Loop over all keys of the root object */
	for (int i = 1; i < ret; i++) {
	    if (jsoneq(buffer, &tokens[i], "status") == 0) {
	      /* reply with the status and finish */
	    	jsonPrintStatus();
	    	return ret;
	    } 
	    else if (jsoneq(buffer, &tokens[i], "led") == 0) {
			keyFirstChar = buffer[tokens[i + 1].start];
			
			if (keyFirstChar == 't'){
				/*true*/
				thConfig.ledEnabled = true;
			} else if (keyFirstChar == 'f'){
			/*false*/
				thConfig.ledEnabled = false;
			}
			i++;
	    } 
	    else if (jsoneq(buffer, &tokens[i], "format") == 0) {
	    	keyFirstChar = buffer[tokens[i + 1].start];

	    	if (toUpperCase(keyFirstChar) == 'C'){
	    		thConfig.format = CSV;
	    	} else if (toUpperCase(keyFirstChar) == 'J'){
	    		thConfig.format = JSON;
	    	} else if (toUpperCase(keyFirstChar) == 'H'){
	    		thConfig.format = HUMAN;
	    	}
	    	i++;
	    }
	    else if (jsoneq(buffer, &tokens[i], "reportingPeriod") == 0) {
	    	const char* start = buffer + tokens[i + 1].start;
	    	uint32_t value = strtoul(start, NULL, 10);

	    	if (value >= 1 && value <= 3600) 
	    		thConfig.reportingPeriod = value; 
	    	i++;
	    }
	    else if (jsoneq(buffer, &tokens[i], "temperatureOffset") == 0) {
	    	const char* start = buffer + tokens[i + 1].start;
	    	float value = strtof(start, NULL); 

	    	if (value >= -15.0f && value <= 15.0f){ 
	    		thConfig.temperatureOffset = value; 
	    	}
	    	i++;
	    }
	    else if (jsoneq(buffer, &tokens[i], "saveConfig") == 0) {
	    	/* store thConfig in Flash*/
	    	saveConfig(&thConfig);
	    	i++;
	    }
	    else if (jsoneq(buffer, &tokens[i], "info") == 0) {
	    	jsonPrintDevInfo();
	    	return ret;
	    }
	    
	}

	jsonPrintStatus();
	return ret;
}

static char toUpperCase(const char ch)
{
	if (ch >= 97 && ch <= 122){
		return (ch-32);
	}
	else return ch;
}

static void jsonPrintStatus(void)
{
	uint32_t timestamp = HAL_GetTick();
	uprintf("{\"status\":{\"reportingPeriod\":%lu,\"format\":\"%s\",\"temperatureOffset:\":%2.1f,\"upTime\":%lu}}\r\n",  
				thConfig.reportingPeriod,
				FORMAT_STRING[thConfig.format],
				thConfig.temperatureOffset,
				timestamp);
}

static void jsonPrintDevInfo(void)
{
	uprintf("{\"device\":\"%s\",\"serial\":\"%s\",\"firmware\":\"%d.%d.%d\"}\r\n",  
				HW_ID,
				thConfig.serialNumberStr,
				VERSION_MAJOR,
				VERSION_MINOR,
				VERSION_PATCH);
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) 
{
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

/* Robert Jenkins' 32 bit integer hash function */
static uint32_t hash32(uint32_t a)
{
   a = (a+0x7ed55d16) + (a<<12);
   a = (a^0xc761c23c) ^ (a>>19);
   a = (a+0x165667b1) + (a<<5);
   a = (a+0xd3a2646c) ^ (a<<9);
   a = (a+0xfd7046c5) + (a<<3);
   a = (a^0xb55a4f09) ^ (a>>16);
   return a;
}




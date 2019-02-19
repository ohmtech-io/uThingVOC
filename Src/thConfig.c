#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include "thConfig.h"
#include "main.h" //for the UART_LOG
#include "usbd_cdc_if.h"

configs_t thConfig = { .format = JSON,
					 .samplingPeriodIdx = 0,
					 .samplingPeriod = 3, /*default*/
					 .gasResEnabled  = 1,
					 .tempEnabled 	 = 1,
					 .humEnabled 	 = 1,
					 .pressEnabled 	 = 1,
					};


static const char *FORMAT_STRING[] = {
    "JSON", "HUMAN", "CSV", "BINARY",
};

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

char outBuffer[1024];

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
}


int uprintf(const char *format, ...)
{
	va_list arguments;

	va_start(arguments, format);

	int len = vsprintf(outBuffer, format, arguments);
	// int len = snprintf(outBuffer, 100, format, arguments);
	// UartLog("len: %i, outBuffer: %s", len, outBuffer);
	printf("%s", outBuffer);
	
	va_end(arguments);
	
	uint8_t res = CDC_Transmit_FS((uint8_t *)&outBuffer, len);
	if (res == USBD_BUSY)
	{
		UartLog("USB_BUSY");
	}	

	return len;
}

void showConfig()
{
	uint32_t timestamp = HAL_GetTick();
	uprintf("\n\r-------------------------------------------------------- \
			\n\r***  Status: \
			\n\r Serial #: %s, Samplig period = %s, Format = %s, Uptime = %lu ms \
			\n\r-------------------------------------------------------- \n\r", 
			thConfig.serialNumberStr,
			PERIOD_STRING[thConfig.samplingPeriodIdx], 
			FORMAT_STRING[thConfig.format], 
			timestamp);
}	

void printPeriod()
{
	uprintf("\n\r***  Config: Sampling period set to %s.\n\r", 
			PERIOD_STRING[thConfig.samplingPeriodIdx]);
}

void printHelp()
{
	uprintf("\n\r------------------------------------------------------- \n\r");
	uprintf("*********** Available commands: *********************** \n\r");
	uprintf("[h]: Show this help \n\r");
	uprintf("--- Output format:\n\r");
	uprintf("[j]: JSON (default)\n\r");
	uprintf("[m]: Human readable\n\r");
	uprintf("[b]: Binary \n\r");
	uprintf("---- \n\r");
	uprintf("[p]: Set sampling period in milliseconds (default: 1000)\n\r");
	uprintf("[s]: Start / stop sensors sampling\n\r");
	uprintf("------------------------------------------------------- \n\r");
}

void processChar(uint8_t input)
{
	static uint8_t receivingNumber = 0, digit = 0, dbuffer[10];

	// UartLog("received char: %c", input);

	if (!receivingNumber)
	{
		//Echo the received character
		// uprintf((char *)&input); 
		
		switch (toupper(input))
		{
			// case 'H':
			// case '?':
			// 	printHelp();
			// 	showConfig();
			// 	break;
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
						\n\rFormat: [temperature], [pressure], [humitidy], [gasResistance], [IAQ], [accuracy]\n\r");
				break;	
			// case 'B':
			// 	thConfig.format = BINARY;
			// 	uprintf("\n\rConfig: Set output format to Binary.\n\r");
			// 	break;		
			// case 'P':
			// 	receivingNumber = 1;
			// 	uprintf("\n\rIntroduce sampling period (min 100 ms): ");
			// 	break;

			case 'S':
				showConfig();
				break;

			case '1':
				thConfig.samplingPeriodIdx = 0; //3s
				thConfig.samplingPeriod = 3;
				printPeriod();
				break;
			case '2':
				thConfig.samplingPeriodIdx = 1; //10s
				thConfig.samplingPeriod = 10;
				printPeriod();
				break;
			case '3':
				thConfig.samplingPeriodIdx = 2; //30s
				thConfig.samplingPeriod = 30;
				printPeriod();
				break;
			case '4':
				thConfig.samplingPeriodIdx = 3; //1m
				thConfig.samplingPeriod = 60;
				printPeriod();
				break;
			case '5':
				thConfig.samplingPeriodIdx = 4; //10m
				thConfig.samplingPeriod = 600;
				printPeriod();
				break;
			case '6':
				thConfig.samplingPeriodIdx = 5; //30m
				thConfig.samplingPeriod = 1800;
				printPeriod();
				break;
			case '7':
				thConfig.samplingPeriodIdx = 6; //1h
				thConfig.samplingPeriod = 3600;
				printPeriod();
				break;

			default:
				uprintf("\n\r------------------------------------------------------- \
						\n\r***  Invalid option. \
						\n\r Use:    [m] Human readable, [j] JSON, [c] CSV, [s] Show configuration, \
						\n\r Period: [1] 3 sec, [2] 10 sec, [3] 30 sec, [4] 1 min, [5] 10 min, [6] 30 min, [7] 1 hour. \
						\n\r-------------------------------------------------------- \n\r");
		}
	} else {
		if (input>='0' && input<='9')
		{
			if (++digit < 9)
			{
				dbuffer[digit] = input;	
			} else {
				uprintf("\n\rERROR: too long. Introduce sampling period [ms]: ");	
				digit = 0;
			} 
			
		} 
		else if (input == 0x08 && digit>0) /*backspace*/
		{
			uprintf((char *)0x08); //TODO: verify if it's possible and works in different consoles!
			--digit;
		}
		else if (input == 0x0D) /*CR*/
		{
			++digit;
			dbuffer[digit] = 0;
			
			uint32_t number = strtoul((char *)dbuffer, (char **)NULL, 10);
			if (number < 100) 
			{
				uprintf("\n\rERROR: too small, using 100ms.\n\r");
				number = 100;
			}
			
			thConfig.samplingPeriod = number;
			uprintf("\n\rConfig: sampling period = %lu ms.\n\r", number);

			receivingNumber = 0;
		}

	}
}		


/*Robert Jenkins' 32 bit integer hash function*/
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




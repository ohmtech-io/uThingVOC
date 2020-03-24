#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
	JSON 	= 0,
	HUMAN 	= 1,
	CSV		= 2,
	BINARY	= 3
} outFormat_t;


typedef struct _configs_t {
	uint8_t		reportingPeriodIdx;
	uint32_t	reportingPeriod; //ms
	bool		gasResEnabled :1;
	bool		tempEnabled	  :1;
	bool		humEnabled	  :1;
	bool		pressEnabled  :1;
	bool		ledEnabled	  :1;
	outFormat_t	format;
	char 		serialNumberStr[17];
} configs_t; 

typedef struct shellBuffer_t {
	char Buf[100];
	uint8_t idx;
	bool newLine;
} shellBuffer_t;


void processVCPinput(void);

int uprintf(const char *format, ...);

void initConfig(void);

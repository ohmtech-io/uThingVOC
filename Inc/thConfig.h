#pragma once
#include <stdint.h>

typedef enum {
	JSON 	= 0,
	HUMAN 	= 1,
	CSV		= 2,
	BINARY	= 3
} outFormat_t;


typedef struct _configs_t {
	uint8_t		samplingPeriodIdx;
	uint32_t	samplingPeriod; //ms
	uint8_t		gasResEnabled :1;
	uint8_t		tempEnabled	  :1;
	uint8_t		humEnabled	  :1;
	uint8_t		pressEnabled  :1;
	outFormat_t	format;
} configs_t; 

void processChar(uint8_t input);

int uprintf(const char *format, ...);

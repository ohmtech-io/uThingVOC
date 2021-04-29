/***************************************************************************
*** MIT License ***
*
*** Copyright (c) 2021 Daniel Mancuso - OhmTech.io **
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
#include <stdint.h>
#include "main.h"
#include "flashSave.h"
#include "thConfig.h"

extern IWDG_HandleTypeDef   watchdogHandle;

/* Let's store the BME680 state data on the last 2KBs...*/
static const uint32_t bsecPageStartAddress = ADDR_FLASH_PAGE_63; 


/* thConfig storage STM32L412KB (128 KB Flash)*/
static const uint32_t configStartAddress = ADDR_FLASH_PAGE_62; 
static const uint8_t  pageNumber = 62;

/*NOTE: On STM32L4 series the FLash is only programmed 72 bits at a time (64 bits plus 8 ECC bits)*/

static const uint64_t MAGIC_NUMBER = 0xDDCCBBAADEADBEEF;

/*!
 * @brief           Load previous library state from non-volatile memory
 *
 * @param[in,out]   state_buffer    buffer to hold the loaded state string
 * @param[in]       n_buffer        size of the allocated state buffer
 *
 * @return          number of bytes copied to state_buffer
 */
uint32_t state_load(uint8_t *state_buffer, uint32_t n_buffer)
{
    // Load a previous library state from non-volatile memory, if available.
    // Return zero if loading was unsuccessful or no state was available, 
    // otherwise return length of loaded state string.
	volatile uint32_t flashAddress = bsecPageStartAddress;

	if (MAGIC_NUMBER != *(volatile uint64_t*)flashAddress){
		/* first time (nothing saved yet) or error */
		return 0;
	}
	flashAddress += 8;

	uint32_t length =  *(volatile uint64_t*)flashAddress;
	if (n_buffer <= length){
		/* error! the reserved size should be bigger than the saved length */
		return 0;
	}
	flashAddress += 8;

	volatile uint64_t *ptr = (volatile uint64_t* )state_buffer;

	for (int i=0; i < n_buffer; i += 8, ptr++, flashAddress += 8){   
		*ptr = *(volatile uint64_t*)flashAddress;
	}

	UartLog("BSEC Configuration loaded from Flash (%ld bytes).", length);

	return length;
}

/*!
 * @brief           Save library state to non-volatile memory
 *
 * @param[in]       state_buffer    buffer holding the state to be stored
 * @param[in]       length          length of the state string to be stored
 *
 * @return          none
 */
void state_save(const uint8_t *state_buffer, uint32_t length)
{
	int ret = 0;
	static FLASH_EraseInitTypeDef EraseInitStruct;
	uint32_t PageError;

    UartLog("Storing BSEC configuration in Flash (%ld bytes)...", length);

    /* Refresh IWDG: let's kick the watchdog,
     we don't want to be reset during a Flash write procedure!! */
    HAL_IWDG_Refresh(&watchdogHandle);

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR); 
    ret += HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);  
    
      /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Page = pageNumber;
    EraseInitStruct.Banks       = FLASH_BANK_1;
    EraseInitStruct.NbPages = 1;

  	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK){
      UartLog("ERROR Storing BSEC configuration in Flash!");
      Error_Handler(); //oops!
  	}

    volatile uint32_t flashAddress = bsecPageStartAddress;
    
    /* Store the magic number */
    SET_BIT(FLASH->CR, FLASH_CR_PG);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);  
    ret += HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flashAddress, MAGIC_NUMBER);
    flashAddress += 8;

    /* Store the buffer length */
    ret += HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flashAddress, length);
    flashAddress += 8;

    /* Store now the state_buffer, 8 bytes at a time */
    volatile uint64_t *pRecord = (volatile uint64_t* )state_buffer; 

    for (int i=0; i < length; i += 8, pRecord++, flashAddress += 8){
        ret += HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flashAddress, *pRecord);
    }

    ret += HAL_FLASH_Lock();
}



int loadConfig(configs_t *config)
{
  volatile uint32_t flashAddress = configStartAddress;

  if (MAGIC_NUMBER != *(volatile uint64_t*)flashAddress){
    /* first time (nothing saved yet) or error */
    return 0;
  }
  flashAddress += 8;

  uint8_t length = sizeof(configs_t);
  volatile uint64_t *ptr = (volatile uint64_t* )config;

  for (int i=0; i < length; i += 8, ptr++, flashAddress += 8){   
    *ptr = *(volatile uint64_t*)flashAddress;
  }

  UartLog("uThing Configuration loaded from Flash (%d bytes).", length);

  return length;
}

int saveConfig(configs_t *config)
{
  int ret = 0;
  static FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t PageError;

    UartLog("Storing uThing configuration in Flash...");

    /* Refresh IWDG: let's kick the watchdog,
     we don't want to be reset during a Flash write procedure!! */
    HAL_IWDG_Refresh(&watchdogHandle);

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR); 
    ret += HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);  
    
      /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Page = pageNumber;
    EraseInitStruct.Banks       = FLASH_BANK_1;
    EraseInitStruct.NbPages = 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK){
      UartLog("ERROR Storing uThing configuration in Flash!");
      Error_Handler(); //oops!
    }

    volatile uint32_t flashAddress = configStartAddress;
     
    /* Store the magic number */
    SET_BIT(FLASH->CR, FLASH_CR_PG);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);  
    ret += HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flashAddress, MAGIC_NUMBER);
    flashAddress += 8;

    /* Store now the state_buffer, 8 bytes at a time */
    volatile uint64_t *pRecord = (uint64_t* )config; 

    for (int i=0; i < sizeof(configs_t); i += 8, pRecord++, flashAddress += 8){
        ret += HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flashAddress, *pRecord);
    }

    ret += HAL_FLASH_Lock();
    return ret;
}

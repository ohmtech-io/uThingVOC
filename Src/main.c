#include "main.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"

#include "bme680.h"
#include "thConfig.h"
#include "thBsec.h"
#include "bsec_interface.h"
#include "bme680_selftest.h"
#include "bsec_serialized_configurations_iaq.h"
#include "flashSave.h"

#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)

#define TEMP_OFFSET 6.0f

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c2;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart1;
/*------------------------*/

/* IWDG handler declaration (independent, 40kHz LSI)*/
IWDG_HandleTypeDef   watchdogHandle;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);
static void WatchdogInit(IWDG_HandleTypeDef *watchdogHandle);

void output_ready(int64_t timestamp, float iaq, uint8_t iaq_accuracy, float temperature, 
                  float humidity, float pressure, float raw_temperature, float raw_humidity,
                  float gas, bsec_library_return_t bsec_status, float static_iaq, 
                  float co2_equivalent, float breath_voc_equivalent);
int gasSensorInit(struct bme680_dev *gas_sensor);
int gasSensorConfig(struct bme680_dev *gas_sensor);
uint32_t config_load(uint8_t *config_buffer, uint32_t n_buffer);
int64_t get_timestamp_us(void);
int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
void user_delay_ms(uint32_t period);


struct bme680_dev gas_sensor;
extern configs_t thConfig;

static char outputString[200];
static uint16_t secCount = 0;

uint8_t iaqAccuracy = 0;
bsec_library_return_t bsec_status = BSEC_E_CONFIG_EMPTY;

int main(void)
{
  return_values_init ret;

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Obtain serial number */
  initConfig(); 

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C2_Init();
  MX_USB_DEVICE_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();

  /* Self-test, it takes ~ 12 seconds */
  int8_t res = gasSensorInit(&gas_sensor);
  if (res == BME680_OK) {UartLog("BME680 initialized.");}
    else                {UartLog("Error initializing the sensor, %d", res);}

  res = bme680_self_test(&gas_sensor);
  if (res == BME680_OK) {
    UartLog("BME680: Self Test passed OK."); 
  }
  else {
      UartLog("Error!!! Self Test FAILED! %d", res);
      Error_Handler();
  }

  /**/

  WatchdogInit(&watchdogHandle);

  UartLog("Initializing BSEC and BME680...");
  ret = bsec_iot_init(BSEC_SAMPLE_RATE_LP, TEMP_OFFSET, user_i2c_write, user_i2c_read, user_delay_ms, state_load, config_load);

  if (ret.bme680_status)
  {
      /* Could not intialize BME680 */
      UartLog("Error while initializing BME680!!!");
      Error_Handler();
  } else {
     UartLog("Sensor and BSEC ready.");
     HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, GPIO_PIN_RESET);
  }
  
  if (ret.bsec_status)
  {
      /* Could not intialize BSEC library */
      UartLog("Error while initializing BSEC library!!!");
      Error_Handler();
  } else {
      /* Call to endless loop function which reads and processes data based on sensor settings */
      /* state is saved every 24 hours (24*3600 / 3) */
      bsec_iot_loop(user_delay_ms, get_timestamp_us, output_ready, state_save, 28800);
  }
 
  return -1; /*This should never be reached*/
}

/*--------------------------------------------*/
void output_ready(int64_t timestamp, float iaq, uint8_t iaq_accuracy, float temperature, 
                  float humidity, float pressure, float raw_temperature, float raw_humidity,
                  float gas, bsec_library_return_t _bsec_status, float static_iaq, float co2_equivalent, float breath_voc_equivalent)
{
      iaqAccuracy = iaq_accuracy;
      bsec_status = _bsec_status;

      /* the output will be finally printed by the timer handler... */
      switch (thConfig.format){
      case JSON:
        sprintf(outputString, "{\"temperature\": %.2f, \"pressure\": %.2f, \"humidity\": %.2f, \"gasResistance\": %6.0f, \"IAQ\": %.1f, \"iaqAccuracy\": %u, \"eqCO2\": %.2f, \"eqBreathVOC\": %.2f}\r\n", 
            temperature,
            pressure/100, 
            humidity, 
            gas,
            iaq,
            iaq_accuracy,
            co2_equivalent,
            breath_voc_equivalent);  
        break;
      case CSV:
        sprintf(outputString, "%.2f, %.2f, %.2f, %6.0f, %.1f, %u, %.1f, %.2f,\r\n",
            temperature,
            pressure/100, 
            humidity, 
            gas,
            iaq,
            iaq_accuracy,
            co2_equivalent,
            breath_voc_equivalent);  
        break;
      case HUMAN:
        sprintf(outputString, "Temperature: %.2f C, Pressure: %.2f hPa, Humidity: %.2f %%rH, Gas resistance: %6.0f ohms, IAQ: %.1f, IAQ Accuracy: %u, CO2equivalent: %.1f, Breath VOC equivalent: % .2f\r\n", 
            temperature,
            pressure/100, 
            humidity, 
            gas,
            iaq,
            iaq_accuracy,
            co2_equivalent,
            breath_voc_equivalent);  
        break;
      case BINARY:
        break;  
    }
}

/*--------------  BME680 initialization    ------------------------------*/
int gasSensorInit(struct bme680_dev *gas_sensor)
{
  gas_sensor->dev_id = BME680_I2C_ADDR_PRIMARY;
  gas_sensor->intf = BME680_I2C_INTF;
  gas_sensor->read = user_i2c_read;
  gas_sensor->write = user_i2c_write;
  gas_sensor->delay_ms = user_delay_ms;
  /* amb_temp can be set to 25 prior to configuring the gas sensor 
   * or by performing a few temperature readings without operating the gas sensor.
   */
  gas_sensor->amb_temp = 25;

  return bme680_init(gas_sensor);
}

int gasSensorConfig(struct bme680_dev *gas_sensor)
{
  uint8_t set_required_settings;
  int8_t res;
  /* Set the temperature, pressure and humidity settings */
  gas_sensor->tph_sett.os_hum = BME680_OS_2X;
  gas_sensor->tph_sett.os_pres = BME680_OS_4X;
  gas_sensor->tph_sett.os_temp = BME680_OS_8X;
  gas_sensor->tph_sett.filter = BME680_FILTER_SIZE_3;

  /* Set the remaining gas sensor settings and link the heating profile */
  gas_sensor->gas_sett.run_gas = BME680_ENABLE_GAS_MEAS;
  /* Create a ramp heat waveform in 3 steps */
  gas_sensor->gas_sett.heatr_temp = 320; /* degree Celsius */
  gas_sensor->gas_sett.heatr_dur = 150; /* milliseconds */

  /* Select the power mode */
  /* Must be set before writing the sensor configuration */
  gas_sensor->power_mode = BME680_FORCED_MODE; 

  /* Set the required sensor settings needed */
  set_required_settings = BME680_OST_SEL | BME680_OSP_SEL | BME680_OSH_SEL | BME680_FILTER_SEL 
      | BME680_GAS_SENSOR_SEL;

  /* Set the desired sensor configuration */
  res = bme680_set_sensor_settings(set_required_settings, gas_sensor);

  /* Set the power mode */
  res += bme680_set_sensor_mode(gas_sensor); 

  return res;
}

uint32_t config_load(uint8_t *config_buffer, uint32_t n_buffer)
{
    // ...
    // Load a library config from non-volatile memory, if available.
    //
    // Return zero if loading was unsuccessful or no config was available,
    // otherwise return length of loaded config string.
    // ...
    memcpy(config_buffer, bsec_config_iaq, sizeof(bsec_config_iaq));
    return sizeof(bsec_config_iaq);
}

int8_t user_i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
  int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */
  uint8_t addr = BME680_I2C_ADDR_PRIMARY << 1;

  rslt = HAL_I2C_Mem_Read(&hi2c2, addr, reg_addr, 1, reg_data, len, 1000);
  return rslt;
}

int8_t user_i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len)
{
  int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */
  uint8_t addr = BME680_I2C_ADDR_PRIMARY << 1;

  rslt = HAL_I2C_Mem_Write(&hi2c2, addr, reg_addr, 1, reg_data, len, 1000);

  return rslt;
}

void user_delay_ms(uint32_t period)
{
  HAL_Delay(period);
}

int64_t get_timestamp_us(void)
{
  return ((int64_t)HAL_GetTick()) * 1000;
}


/*-------------- Code autogenerated by the STMicro CubeMX ------------------------------*/
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x20303E5D;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /**Configure Analogue filter 
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /**Configure Digital filter 
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 4799;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 9999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_TIM_Base_Start_IT(&htim2);
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, BLUE_LED_Pin|RED_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA3_Pin PA4_Pin */
  GPIO_InitStruct.Pin = PA3_Pin|PA4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : BLUE_LED_Pin RED_LED_Pin */
  GPIO_InitStruct.Pin = BLUE_LED_Pin|RED_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB6_Pin PB7_Pin */
  GPIO_InitStruct.Pin = PB6_Pin|PB7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
 (IWDG independent watchdog, 40kHz LSI)
 **/
static void WatchdogInit(IWDG_HandleTypeDef *wdtHandle)
{
  wdtHandle->Instance = IWDG;
  /* 40kHz / 128 = 312 Hz */
  wdtHandle->Init.Prescaler = IWDG_PRESCALER_128; 
  wdtHandle->Init.Window    = IWDG_WINDOW_DISABLE;
  /* 3000 / 312Hz ~= 10sec. */
  wdtHandle->Init.Reload    = 3000;

  if (HAL_IWDG_Init(wdtHandle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
}

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}

/* ---------------------------------------------------------------------------
   ----------- IRQ Handlers: -------------------------------------------------
   ---------------------------------------------------------------------------
*/
/**
  * @brief  Period elapsed callback in non blocking mode
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* Blink blue LED until BSEC give us a valid IAQ value, ~5 minutes */
  if (iaqAccuracy == 0){  
    HAL_GPIO_TogglePin(GPIOB, BLUE_LED_Pin);  
  } 
  else 
  {
    HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, GPIO_PIN_RESET);
  }

  if (++secCount >= thConfig.samplingPeriod && bsec_status == BSEC_OK)
  {
    secCount = 0;
    uprintf(outputString);
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, 0);
  UartLog("ERROR HANDLER!!!!!!!!!");
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(char *file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */

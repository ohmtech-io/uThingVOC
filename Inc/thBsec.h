#pragma once

/* Use the following bme680 driver: https://github.com/BoschSensortec/BME680_driver/releases/tag/bme680_v3.5.1 */
#include "bme680.h"
#include "bsec_interface.h"
#include "bsec_datatypes.h"

/* function pointer to the system specific sleep function */
typedef void (*sleep_fct)(uint32_t t_ms);

/* function pointer to the system specific timestamp derivation function */
typedef int64_t (*get_timestamp_us_fct)();

/* function pointer to the function processing obtained BSEC outputs */
typedef void (*output_ready_fct)(int64_t timestamp, float iaq, uint8_t iaq_accuracy, float temperature, float humidity,
     float pressure, float raw_temperature, float raw_humidity, float gas, bsec_library_return_t bsec_status,
     float static_iaq, float co2_equivalent, float breath_voc_equivalent);

/* function pointer to the function loading a previous BSEC state from NVM */
typedef uint32_t (*state_load_fct)(uint8_t *state_buffer, uint32_t n_buffer);

/* function pointer to the function saving BSEC state to NVM */
typedef void (*state_save_fct)(const uint8_t *state_buffer, uint32_t length);

/* function pointer to the function loading the BSEC configuration string from NVM */
typedef uint32_t (*config_load_fct)(uint8_t *state_buffer, uint32_t n_buffer);
    
/* structure definitions */

/* Structure with the return value from bsec_iot_init() */
typedef struct{
	/*! Result of API execution status */
	int8_t bme680_status;
	/*! Result of BSEC library */
	bsec_library_return_t bsec_status;
}return_values_init;

/**********************************************************************************************************************/
/*!
 * @brief       Initialize the BME680 sensor and the BSEC library
 *
 * @param[in]   sample_rate         mode to be used (either BSEC_SAMPLE_RATE_ULP or BSEC_SAMPLE_RATE_LP)
 * @param[in]   temperature_offset  device-specific temperature offset (due to self-heating)
 * @param[in]   bus_write           pointer to the bus writing function
 * @param[in]   bus_read            pointer to the bus reading function
 * @param[in]   sleep               pointer to the system-specific sleep function
 * @param[in]   state_load          pointer to the system-specific state load function
 *
 * @return      zero if successful, negative otherwise
 */
return_values_init bsec_iot_init(float sample_rate, float temperature_offset, bme680_com_fptr_t bus_write, bme680_com_fptr_t bus_read, 
    sleep_fct sleep, state_load_fct state_load, config_load_fct config_load);

/*!
 * @brief       Runs the main (endless) loop that queries sensor settings, applies them, and processes the measured data
 *
 * @param[in]   sleep               pointer to the system-specific sleep function
 * @param[in]   get_timestamp_us    pointer to the system-specific timestamp derivation function
 * @param[in]   output_ready        pointer to the function processing obtained BSEC outputs
 * @param[in]   state_save          pointer to the system-specific state save function
 * @param[in]   save_intvl          interval at which BSEC state should be saved (in samples)
 *
 * @return      return_values_init	struct with the result of the API and the BSEC library
 */ 
void bsec_iot_loop(sleep_fct sleep, get_timestamp_us_fct get_timestamp_us, output_ready_fct output_ready,
    state_save_fct state_save, uint32_t save_intvl);

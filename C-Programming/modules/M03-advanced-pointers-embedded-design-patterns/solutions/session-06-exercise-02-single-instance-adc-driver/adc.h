#ifndef M03_ADC_H
#define M03_ADC_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Initializes the single ADC module instance.
 *
 * @param[in] sample_rate_hz Requested sample rate in hertz.
 * @return true on first successful initialization; otherwise false.
 */
bool ADC_Init(uint32_t sample_rate_hz);

/**
 * @brief Deinitializes the ADC module and clears its configuration.
 */
void ADC_DeInit(void);

/**
 * @brief Selects an ADC channel when the module is initialized.
 *
 * @param[in] channel Requested channel identifier.
 */
void ADC_SetChannel(uint32_t channel);

/**
 * @brief Reads the source-defined simulated ADC result.
 *
 * @return 300 when initialized; otherwise 0.
 */
uint32_t ADC_Read(void);

/**
 * @brief Reports whether the ADC module is initialized.
 *
 * @return true when initialized; otherwise false.
 */
bool ADC_IsInitialized(void);

/**
 * @brief Obtains the active channel, or 0 when not initialized.
 *
 * @return Active channel identifier or 0.
 */
uint32_t ADC_GetChannel(void);

/**
 * @brief Obtains the sample rate, or 0 when not initialized.
 *
 * @return Configured rate in hertz or 0.
 */
uint32_t ADC_GetSampleRate(void);

#endif

#include "adc.h"

/**
 * @brief Holds private persistent state for the single ADC module instance.
 */
typedef struct
{
    bool is_initialized;
    uint32_t active_channel;
    uint32_t sample_rate_hz;
} adc_context_t;

/** @brief Owns the ADC state; it has internal linkage in this translation unit. */
static adc_context_t s_adc_ctx = { false, 0U, 0U };

bool ADC_Init(uint32_t sample_rate_hz)
{
    if (s_adc_ctx.is_initialized)
    {
        return false;
    }

    s_adc_ctx.sample_rate_hz = sample_rate_hz;
    s_adc_ctx.active_channel = 0U;
    s_adc_ctx.is_initialized = true;
    return true;
}

void ADC_DeInit(void)
{
    s_adc_ctx.sample_rate_hz = 0U;
    s_adc_ctx.active_channel = 0U;
    s_adc_ctx.is_initialized = false;
}

void ADC_SetChannel(uint32_t channel)
{
    if (!s_adc_ctx.is_initialized)
    {
        return;
    }

    s_adc_ctx.active_channel = channel;
}

uint32_t ADC_Read(void)
{
    if (!s_adc_ctx.is_initialized)
    {
        return 0U;
    }

    return 300U;
}

bool ADC_IsInitialized(void)
{
    return s_adc_ctx.is_initialized;
}

uint32_t ADC_GetChannel(void)
{
    if (!s_adc_ctx.is_initialized)
    {
        return 0U;
    }

    return s_adc_ctx.active_channel;
}

uint32_t ADC_GetSampleRate(void)
{
    if (!s_adc_ctx.is_initialized)
    {
        return 0U;
    }

    return s_adc_ctx.sample_rate_hz;
}

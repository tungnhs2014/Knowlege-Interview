#include "adc.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * In a separate translation unit, this would fail because s_adc_ctx has
 * internal linkage in adc.c:
 *
 * s_adc_ctx.active_channel = 99U;
 *
 * Code in this implementation file can name that private object, so the
 * comment illustrates the external module boundary rather than a same-file
 * language restriction.
 */

int main(void)
{
    uint32_t result_mv;

    if (!ADC_Init(44100U))
    {
        return EXIT_FAILURE;
    }

    (void)puts("[ADC] Initialized at 44100 Hz on channel 0.");

    if (ADC_Init(22050U))
    {
        return EXIT_FAILURE;
    }

    (void)puts("[ADC] Error: Already initialized! Call ADC_DeInit() first.");

    if ((ADC_GetChannel() != 0U) || (ADC_GetSampleRate() != 44100U) ||
        !ADC_IsInitialized())
    {
        return EXIT_FAILURE;
    }

    (void)printf("Channel:     %u\n", (unsigned int)ADC_GetChannel());
    (void)printf("Sample rate: %u\n", (unsigned int)ADC_GetSampleRate());
    (void)puts("Init status: YES");

    ADC_SetChannel(2U);
    result_mv = ADC_Read();
    if ((ADC_GetChannel() != 2U) || (result_mv != 300U))
    {
        return EXIT_FAILURE;
    }

    (void)puts("[ADC] Channel set to 2.");
    (void)puts("[ADC] Read ch2 -> 300 mV");
    (void)printf("Result: %u mV\n", (unsigned int)result_mv);

    ADC_DeInit();
    if (ADC_IsInitialized())
    {
        return EXIT_FAILURE;
    }

    (void)puts("[ADC] De-initialized.");
    (void)puts("Is initialized? NO");

    if (!ADC_Init(22050U) || (ADC_GetSampleRate() != 22050U) ||
        (ADC_GetChannel() != 0U))
    {
        return EXIT_FAILURE;
    }

    ADC_DeInit();
    return EXIT_SUCCESS;
}

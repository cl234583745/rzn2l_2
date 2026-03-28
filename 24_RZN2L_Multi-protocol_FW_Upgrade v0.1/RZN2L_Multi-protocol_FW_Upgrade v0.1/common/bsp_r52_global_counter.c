/*
 * bsp_r52_global_counter.c
 *
 *  Created on: 2026年2月11日
 *      Author: Jerry.Chen
 */
#include <stdint.h>
#include "bsp_api.h"
#include "bsp_r52_global_counter.h"

/*
 * printf("ECAT FOE Total Time:%ldms!!!\n\n", (uint32_t)(endTime - beginTime)/BSP_GLOBAL_SYSTEM_COUNTER_CLOCK_HZ*1000);
 */
uint64_t getGlobalCounter(void)
{
    return (((uint64_t)R_GSC->CNTCVU << 32) | R_GSC->CNTCVL);
}

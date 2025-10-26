/**
 * @file housekeeping.h
 * @brief Housekeeping module for system monitoring
 * 
 * This module monitors system health, stack usage, watchdog, and
 * performs periodic maintenance tasks.
 */

#ifndef _HOUSEKEEPING_H_
#define _HOUSEKEEPING_H_

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

/*==================[macros and definitions]=================================*/

/* Housekeeping task period in milliseconds */
#define HOUSEKEEPING_PERIOD_MS    1000

/*==================[external functions]=====================================*/

/**
 * @brief Housekeeping Task
 * Monitors system health and performs maintenance
 * @param pvParameters Task parameters (unused)
 */
void vHousekeeping_Task(void *pvParameters);

/**
 * @brief Get free heap size
 * @return Free heap in bytes
 */
uint32_t Housekeeping_GetFreeHeap(void);

/**
 * @brief Get minimum free heap size (watermark)
 * @return Minimum free heap in bytes
 */
uint32_t Housekeeping_GetMinFreeHeap(void);

/**
 * @brief Feed watchdog
 */
void Housekeeping_FeedWatchdog(void);

/*==================[end of file]============================================*/

#endif /* _HOUSEKEEPING_H_ */


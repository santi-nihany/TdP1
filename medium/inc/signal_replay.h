/**
 * @file signal_replay.h
 * @brief Signal replay module for reproducing captured signals
 * 
 * This module handles loading signal files and replaying them with
 * precise timing using hardware timers and DMA.
 */

#ifndef _SIGNAL_REPLAY_H_
#define _SIGNAL_REPLAY_H_

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "signal_capture.h"

/*==================[macros and definitions]=================================*/

#define REPLAY_BUFFER_SIZE   2048

/*==================[types]==================================================*/

typedef enum {
    REPLAY_STATE_IDLE,
    REPLAY_STATE_LOADING,
    REPLAY_STATE_READY,
    REPLAY_STATE_PLAYING,
    REPLAY_STATE_ERROR
} ReplayState_t;

/*==================[external functions]=====================================*/

/**
 * @brief Replay Task
 * Handles signal replay with precise timing
 * @param pvParameters Task parameters (unused)
 */
void vReplay_Task(void *pvParameters);

/**
 * @brief Start replaying a signal
 * @param filename File to replay
 * @return pdPASS on success, pdFAIL otherwise
 */
BaseType_t Replay_Start(const char *filename);

/**
 * @brief Stop replay
 */
void Replay_Stop(void);

/**
 * @brief Get replay state
 * @return Current replay state
 */
ReplayState_t Replay_GetState(void);

/**
 * @brief Get replay progress
 * @return Progress percentage (0-100)
 */
uint8_t Replay_GetProgress(void);

/*==================[end of file]============================================*/

#endif /* _SIGNAL_REPLAY_H_ */


/**
 * @file isr_handlers.c
 * @brief Interrupt Service Routines for hardware signal capture
 * 
 * These ISRs handle hardware interrupts for:
 * - IR signal edges
 * - RF signal edges
 * - Button presses
 * - Timer capture events
 */

#include "signal_capture.h"
#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "queue.h"
#include "ui_controller.h"
#include "main.h"

/*==================[external data]==========================================*/

extern StreamBufferHandle_t xStreamBufferIR;
extern StreamBufferHandle_t xStreamBufferRF;
extern QueueHandle_t xUICommandQueue;

/*==================[external functions]=====================================*/

/**
 * @brief ISR for IR signal edge detection
 * This ISR is called when an edge is detected on the IR input pin
 * 
 * Flow:
 * 1. Read hardware timer capture value to get timestamp
 * 2. Calculate time delta since last edge
 * 3. Read signal level (HIGH/LOW)
 * 4. Send {delta, level} to StreamBuffer (non-blocking)
 * 5. Trigger SignalCaptureIR_Task if watermark reached
 */
void IR_ISR_Handler(void)
{
    /* Enter ISR context */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    /* TODO: Read timestamp from timer capture register */
    static uint32_t last_timestamp = 0;
    uint32_t current_timestamp = 0; /* TODO: Read from hardware */
    
    /* Calculate delta time */
    uint32_t delta_time = current_timestamp - last_timestamp;
    last_timestamp = current_timestamp;
    
    /* Read signal level */
    uint8_t signal_level = 0; /* TODO: Read from GPIO */
    
    /* Pack data: {delta_time(24bits), level(8bits)} as uint32_t */
    uint32_t sample_data = (delta_time & 0x00FFFFFF) | (signal_level << 24);
    
    /* Send to StreamBuffer (non-blocking, from ISR) */
    if (xStreamBufferIR != NULL) {
        xStreamBufferSendFromISR(
            xStreamBufferIR,
            &sample_data,
            sizeof(uint32_t),
            &xHigherPriorityTaskWoken
        );
    }
    
    /* Yield if higher priority task was woken */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief ISR for RF signal edge detection
 * Similar to IR_ISR_Handler but for RF signals
 */
void RF_ISR_Handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    /* TODO: Read timestamp from timer capture register */
    static uint32_t last_timestamp = 0;
    uint32_t current_timestamp = 0; /* TODO: Read from hardware */
    
    /* Calculate delta time */
    uint32_t delta_time = current_timestamp - last_timestamp;
    last_timestamp = current_timestamp;
    
    /* Read signal level */
    uint8_t signal_level = 0; /* TODO: Read from GPIO */
    
    /* Pack data: {delta_time(24bits), level(8bits)} as uint32_t */
    uint32_t sample_data = (delta_time & 0x00FFFFFF) | (signal_level << 24);
    
    /* Send to StreamBuffer (non-blocking, from ISR) */
    if (xStreamBufferRF != NULL) {
        xStreamBufferSendFromISR(
            xStreamBufferRF,
            &sample_data,
            sizeof(uint32_t),
            &xHigherPriorityTaskWoken
        );
    }
    
    /* Yield if higher priority task was woken */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * @brief ISR for button press/joystick input
 * Sends UI events to UI task via queue
 */
void Button_ISR_Handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    /* TODO: Read button state from GPIO */
    uint8_t button_state = 0; /* TODO: Read from hardware */
    
    /* Create UI command */
    UICommand_t command;
    command.event = UI_EVENT_NONE;
    
    /* Map button to UI event */
    switch (button_state) {
        case 0x01: /* UP */
            command.event = UI_EVENT_UP;
            break;
        case 0x02: /* DOWN */
            command.event = UI_EVENT_DOWN;
            break;
        case 0x04: /* ENTER/ACEPTAR */
            command.event = UI_EVENT_ACCEPT;
            break;
        case 0x08: /* BACK/ATRAS */
            command.event = UI_EVENT_BACK;
            break;
    }
    
    /* Send to UI queue (non-blocking, from ISR) */
    if (xUICommandQueue != NULL && command.event != UI_EVENT_NONE) {
        xQueueSendFromISR(
            xUICommandQueue,
            &command,
            &xHigherPriorityTaskWoken
        );
    }
    
    /* Yield if higher priority task was woken */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*==================[end of file]============================================*/


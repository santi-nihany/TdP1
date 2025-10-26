/**
 * @file signal_storage.c
 * @brief Signal storage implementation
 */

#include "signal_storage.h"
#include "main.h"
#include "FreeRTOS.h"

/*==================[internal data]==========================================*/

static SemaphoreHandle_t xStorageInUse = NULL;

/*==================[external data]==========================================*/

extern QueueHandle_t xStorageQueue;
extern SemaphoreHandle_t xStorageMutex;

/*==================[external functions]=====================================*/

void vStorage_Task(void *pvParameters)
{
    SignalPacket_t *packet = NULL;
    char filename[64];
    TickType_t last_wake_time = xTaskGetTickCount();
    
    printf("Storage Task started.\r\n");
    
    /* TODO: Initialize FatFS and mount SD card */
    
    xStorageInUse = xSemaphoreCreateMutex();
    
    for (;;) {
        /* Wait for packets from capture tasks */
        if (xQueueReceive(xStorageQueue, &packet, portMAX_DELAY) == pdPASS) {
            /* Take storage mutex */
            if (xSemaphoreTake(xStorageMutex, portMAX_DELAY) == pdPASS) {
                /* Generate filename with timestamp */
                snprintf(filename, sizeof(filename), 
                         "signal_%08X.med", 
                         (unsigned int)packet->timestamp_ms);
                
                /* TODO: Write to SD card via FatFS */
                printf("Saving signal to: %s\r\n", filename);
                
                /* TODO: Implement file write */
                
                /* Free packet memory */
                vPortFree(packet);
                
                /* Release mutex */
                xSemaphoreGive(xStorageMutex);
            }
        }
        
        /* Maintain periodic delay */
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(100));
    }
}

BaseType_t Storage_Init(void)
{
    /* TODO: Initialize FatFS, mount SD card */
    printf("Storage initializing...\r\n");
    return pdPASS;
}

BaseType_t Storage_SaveSignal(SignalPacket_t *packet, const char *filename)
{
    /* TODO: Implement file save */
    return pdFAIL;
}

BaseType_t Storage_LoadSignal(const char *filename, SignalPacket_t **packet)
{
    /* TODO: Implement file load */
    return pdFAIL;
}

BaseType_t Storage_DeleteSignal(const char *filename)
{
    /* TODO: Implement file delete */
    return pdFAIL;
}

uint32_t Storage_ListFiles(SignalFileInfo_t *file_list, uint32_t max_count)
{
    /* TODO: Implement file listing */
    return 0;
}

BaseType_t Storage_GetStats(uint32_t *free_space, uint32_t *total_space)
{
    /* TODO: Implement storage stats */
    return pdFAIL;
}


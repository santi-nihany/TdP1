#ifndef _TEST_STORAGE_H_
#define _TEST_STORAGE_H_

#include "FreeRTOS.h"
#include "task.h"

/* Test task that periodically creates a synthetic SignalPacket_t and sends it
 * to the storage queue via Storage_SaveSignal(). */
void vStorageTest_Task(void *pvParameters);

#endif /* _TEST_STORAGE_H_ */

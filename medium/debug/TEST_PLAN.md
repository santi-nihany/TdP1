# Plan de Testing - Firmware Médium

## 🎯 Objetivos de Testing

1. **Verificar que el RTOS funciona correctamente**
2. **Verificar que cada módulo funciona aisladamente**
3. **Verificar que la comunicación entre tareas funciona**
4. **Probar casos límite y errores**

## 📝 Estrategia de Testing

### Nivel 1: Testing del RTOS (SANITY CHECK)

**Objetivo**: Verificar que FreeRTOS arranca y las tareas básicas funcionan.

#### Test 1.1: Verificar Scheduler

```c
// Crear tareas que solo imprimen "Task X running"
// Verificar que se ejecutan en paralelo
```

#### Test 1.2: Verificar Prioridades

```c
// Crear 2 tareas con prioridades diferentes
// Tarea alta prioridad debe ejecutarse primero
```

#### Test 1.3: Verificar Bloqueo y Context Switching

```c
// Usar vTaskDelay y verificar que otras tareas se ejecutan
```

### Nivel 2: Testing de Módulos Aislados

#### Test 2.1: Testing de Signal Capture (Sin Hardware)

```c
// Mock StreamBuffer con datos sintéticos
// Verificar que SignalCapture_Task procesa correctamente
// Verificar creación de paquetes
```

#### Test 2.2: Testing de UI Controller

```c
// Inyectar eventos sintéticos
// Verificar transiciones de estados
// Verificar salida de display
```

#### Test 2.3: Testing de Housekeeping

```c
// Verificar que reporta free heap
// Verificar que no causa bloqueos
```

#### Test 2.4: Testing de Storage (Sin SD)

```c
// Mock FatFS
// Verificar formato de archivos
// Verificar manejo de errores
```

#### Test 2.5: Testing de Replay

```c
// Inyectar datos sintéticos
// Verificar formato de salida
```

### Nivel 3: Testing de Integración

#### Test 3.1: Captura → Almacenamiento

```c
// Simular captura → Verificar escritura en buffer sintético
```

#### Test 3.2: Almacenamiento → Replay

```c
// Leer archivo sintético → Verificar reproducción
```

#### Test 3.3: UI → Comandos

```c
// Simular botones → Verificar comandos a otras tareas
```

## 🧪 Módulos de Prueba

### Módulo 1: Test Básico de RTOS

**Archivo**: `src/test_rtos_basic.c`

```c
#include "FreeRTOS.h"
#include "task.h"
#include "sapi.h"

void vTestTask1(void *pvParameters) {
    while(1) {
        printf("Task 1: Prio=%d\r\n", uxTaskPriorityGet(NULL));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vTestTask2(void *pvParameters) {
    while(1) {
        printf("Task 2: Prio=%d\r\n", uxTaskPriorityGet(NULL));
        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}

int main(void) {
    boardConfig();
    uartConfig(UART_USB, 115200);

    printf("=== RTOS Basic Test ===\r\n");

    // Crear tareas con diferentes prioridades
    xTaskCreate(vTestTask1, "Test1", 512, NULL, 2, NULL);
    xTaskCreate(vTestTask2, "Test2", 512, NULL, 1, NULL);

    vTaskStartScheduler();

    while(1);
    return 0;
}
```

**Verificación**:

- ✅ Ver "Task 1" y "Task 2" alternándose
- ✅ Task 1 tiene mayor prioridad y se ejecuta más frecuente
- ✅ No hay bloqueos del sistema

### Módulo 2: Test de Queues y Comunicación

**Archivo**: `src/test_queue.c`

```c
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "sapi.h"

QueueHandle_t xTestQueue;

void vSenderTask(void *pvParameters) {
    uint32_t value = 0;
    while(1) {
        if (xQueueSend(xTestQueue, &value, 0) == pdPASS) {
            printf("Sent: %d\r\n", value++);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void vReceiverTask(void *pvParameters) {
    uint32_t received;
    while(1) {
        if (xQueueReceive(xTestQueue, &received, portMAX_DELAY) == pdPASS) {
            printf("Received: %d\r\n", received);
            gpioToggle(LEDB);
        }
    }
}

int main(void) {
    boardConfig();
    uartConfig(UART_USB, 115200);

    xTestQueue = xQueueCreate(5, sizeof(uint32_t));

    printf("=== Queue Test ===\r\n");

    xTaskCreate(vSenderTask, "Sender", 512, NULL, 1, NULL);
    xTaskCreate(vReceiverTask, "Receiver", 512, NULL, 2, NULL);

    vTaskStartScheduler();

    while(1);
    return 0;
}
```

**Verificación**:

- ✅ Ver mensajes "Sent" y "Received" sincronizados
- ✅ LED parpadea con cada recepción
- ✅ No hay pérdida de mensajes

### Módulo 3: Test de StreamBuffers

**Archivo**: `src/test_streambuffer.c`

```c
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "sapi.h"

StreamBufferHandle_t xTestStreamBuffer;

void vWriterTask(void *pvParameters) {
    uint32_t data = 0;
    while(1) {
        printf("Writing: %d\r\n", data);
        xStreamBufferSend(xTestStreamBuffer, &data, sizeof(uint32_t),
                         pdMS_TO_TICKS(100));
        data++;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vReaderTask(void *pvParameters) {
    uint32_t received;
    while(1) {
        size_t bytes = xStreamBufferReceive(xTestStreamBuffer, &received,
                                           sizeof(uint32_t), portMAX_DELAY);
        if (bytes == sizeof(uint32_t)) {
            printf("Read: %d\r\n", received);
        }
    }
}

int main(void) {
    boardConfig();
    uartConfig(UART_USB, 115200);

    xTestStreamBuffer = xStreamBufferCreate(128, sizeof(uint32_t));

    printf("=== StreamBuffer Test ===\r\n");

    xTaskCreate(vWriterTask, "Writer", 512, NULL, 1, NULL);
    xTaskCreate(vReaderTask, "Reader", 512, NULL, 1, NULL);

    vTaskStartScheduler();

    while(1);
    return 0;
}
```

**Verificación**:

- ✅ Datos transmitidos sin pérdidas
- ✅ Writer se bloquea si buffer lleno
- ✅ Reader se bloquea si buffer vacío

## 🔍 Verificación de Módulos Aislados

### Signal Capture Module

**Test Unitario**: Mock StreamBuffer y verificar procesamiento

```c
void test_signal_capture_module(void) {
    // Inyectar muestras sintéticas en StreamBuffer
    uint32_t test_samples[10] = {100, 200, 150, ...};

    // Enviar a StreamBuffer
    xStreamBufferSend(xStreamBufferIR, test_samples, sizeof(test_samples), 0);

    // SignalCapture_Task debe procesar y crear paquete
    // Verificar que paquete tiene formato correcto
    // Verificar que paquete llega a StorageQueue
}
```

### UI Controller Module

**Test Unitario**: Probar FSM

```c
void test_ui_fsm(void) {
    // Simular evento de botón
    UICommand_t cmd = {UI_EVENT_ACCEPT};
    UI_SendCommand(cmd);

    // Verificar transición de estado
    assert(UI_GetState() == UI_STATE_CAPTURE_IR);

    // Simular otro evento
    cmd.event = UI_EVENT_BACK;
    UI_SendCommand(cmd);

    // Verificar retorno a MENÚ
    assert(UI_GetState() == UI_STATE_MENU);
}
```

### Storage Module

**Test Unitario**: Mock FatFS

```c
void test_storage_format(void) {
    // Crear paquete sintético
    SignalPacket_t *packet = create_test_packet();

    // Guardar
    BaseType_t result = Storage_SaveSignal(packet, "test.med");

    // Verificar resultado
    assert(result == pdPASS);

    // Leer
    SignalPacket_t *loaded = NULL;
    result = Storage_LoadSignal("test.med", &loaded);

    // Verificar contenido
    assert(memcmp(packet->data, loaded->data, packet->sample_count) == 0);
}
```

## ⚠️ Casos Límite a Probar

1. **Buffer lleno**: ¿Qué pasa si StreamBuffer se llena?
2. **Queue llena**: ¿Cómo se maneja cola de storage llena?
3. **SD sin espacio**: ¿Cómo se maneja error de escritura?
4. **Interrupción durante captura**: ¿Se pierden datos?
5. **Timeout**: ¿Se detiene captura si tarda mucho?
6. **Stack overflow**: ¿Se detecta?
7. **Heap bajo**: ¿Cómo se maneja bajo consumo de memoria?

## 📊 Métricas de Éxito

- ✅ Todas las tareas se crean correctamente
- ✅ Prioridades se respetan
- ✅ No hay deadlocks
- ✅ No hay race conditions
- ✅ Comunicación entre tareas funciona
- ✅ ISRs no bloquean tareas
- ✅ Sistema estable por > 5 minutos
- ✅ Uso de memoria dentro de límites

## 🚀 Continuación del Diseño

### Módulos que Necesitan MEF

1. **UI Controller** ✅ Ya implementado (FSM)
2. **Signal Capture** ⚠️ Agregar estados: IDLE, PREPARING, CAPTURING, PROCESSING
3. **Storage** ⚠️ Agregar estados: IDLE, WRITING, READING, ERROR
4. **Replay** ✅ Ya implementado con estados

### Módulos que Necesitan Interrupciones

1. **IR Capture** ⚠️ Implementar Timer Capture ISR
2. **RF Capture** ⚠️ Implementar Timer Capture ISR
3. **Buttons** ⚠️ Implementar GPIO IRQ handlers
4. **Timer para Reproducción** ⚠️ Configurar Timer ISR para timing preciso

### Próximos Pasos

1. **Implementar Timer Capture ISR** para IR/RF
2. **Integrar FatFS** con ejemplo de SD_SPI
3. **Completar UI con LCD** driver
4. **Implementar control de botones** con GPIO IRQ
5. **Agregar detección de errores** robusta
6. **Testing en hardware real**

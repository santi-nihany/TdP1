# Resumen y Decisiones - Firmware Médium

## 🎯 Resumen Ejecutivo

Se ha realizado un análisis crítico del firmware implementado para el proyecto Médium y se identificaron los siguientes aspectos:

### ✅ Aspectos Positivos

1. Arquitectura clara y bien estructurada
2. Separación adecuada de capas
3. Uso correcto de FreeRTOS
4. Documentación completa
5. Estados de UI bien definidos

### ⚠️ Aspectos a Mejorar

1. Variables externas no definidas correctamente
2. ISRs son stubs sin implementación real
3. FatFS no está integrado
4. Falta timeout para capturas
5. No hay protección contra stack overflow
6. Manejo de errores insuficiente

## 🔧 Fixes Aplicables

### Fix 1: Definir Variables Externas Correctamente

**Problema**: Variables `extern` declaradas en headers pero no definidas.

**Solución**: Agregar módulo `src/shared_resources.c` con definiciones.

```c
// src/shared_resources.c
#include "shared_resources.h"

StreamBufferHandle_t xStreamBufferIR = NULL;
StreamBufferHandle_t xStreamBufferRF = NULL;
QueueHandle_t xStorageQueue = NULL;
QueueHandle_t xUICommandQueue = NULL;
SemaphoreHandle_t xStorageMutex = NULL;
```

### Fix 2: Agregar Timeout a Capturas

**Problema**: Capturas pueden durar indefinidamente.

**Solución**: Usar `xSemaphoreTake` con timeout.

```c
void vSignalCaptureIR_Task(void *pvParameters) {
    const TickType_t timeout = pdMS_TO_TICKS(MAX_CAPTURE_DURATION);

    if (xSemaphoreTake(xCaptureStartSemaphore, timeout) == pdPASS) {
        // Capturar
        while (capture_active && (xTaskGetTickCount() < start_time + timeout)) {
            // ...
        }
    }
}
```

### Fix 3: Implementar Watermarks en StreamBuffers

**Problema**: StreamBuffers no notifican cuando tienen datos suficientes.

**Solución**: Configurar watermark en creación.

```c
// En main.c
xStreamBufferIR = xStreamBufferCreate(STREAM_BUFFER_SIZE, sizeof(uint32_t));
// Configurar trigger level
xStreamBufferSetTriggerLevel(xStreamBufferIR, STREAM_BUFFER_WATERMARK);
```

### Fix 4: Mejorar Manejo de Errores en Storage

**Problema**: Si queue llena, datos se pierden.

**Solución**: Verificar antes de enviar y retry con timeout.

```c
BaseType_t result;
retry:
result = xQueueSend(xStorageQueue, &packet, pdMS_TO_TICKS(100));
if (result != pdPASS) {
    // Queue llena, esperar un poco
    vTaskDelay(pdMS_TO_TICKS(10));
    goto retry;
}
```

### Fix 5: Integrar FatFS

**Problema**: Storage_Task no puede escribir en SD.

**Solución**: Integrar ejemplo `sd_spi.c` existente.

```c
// En signal_storage.c
#include "ff.h"
#include "fssdc.h"

void vStorage_Task(void *pvParameters) {
    FATFS fs;
    FIL fil;

    // Inicializar SD
    FSSDC_InitSPI();
    f_mount(&fs, "SDC:", 0);

    // Ahora puede usar f_open, f_write, etc.
}
```

## 🧪 Estrategia de Testing

### Fase 1: Sanity Check de RTOS ✅ PRIORIDAD

**Módulo**: `test_rtos_sanity.c`

- Verificar que scheduler arranca
- Verificar que tareas se crean
- Verificar alternancia de tareas
- **Resultado esperado**: Sistema estable, tareas alternan

### Fase 2: Testing de Módulos Aislados ✅ PRIORIDAD

**Módulos a testear**:

1. SignalCapture (mock StreamBuffer)
2. UI Controller (mock events)
3. Housekeeping (verificar estadísticas)
4. Storage (mock FatFS)

**Resultado esperado**: Cada módulo funciona independientemente

### Fase 3: Testing de Integración ⚠️ MEDIA PRIORIDAD

**Flujos a testear**:

1. UI → Capture → Storage
2. Storage → Replay
3. Error handling completo

**Resultado esperado**: Flujos completos funcionan

### Fase 4: Testing en Hardware ⚠️ BAJA PRIORIDAD (después de fixes)

**Tests en hardware**:

- Conectar módulos IR/RF reales
- Probar captura con señales reales
- Probar escritura/lectura en SD real
- Medir latencias

## 🚀 Continuación del Diseño

### MEF Necesarias

1. **UI Controller** ✅ YA TIENE

   - Estados: MENU, CAPTURE_IR, CAPTURE_RF, FINISHED, FILES
   - Transiciones según eventos

2. **Signal Capture** ⚠️ AGREGAR

   - Estados: IDLE, PREPARING, CAPTURING, PROCESSING
   - Timer para timeout de captura

3. **Storage** ⚠️ AGREGAR

   - Estados: IDLE, MOUNTING, WRITING, READING, ERROR
   - Manejo de errores de SD

4. **Replay** ✅ YA TIENE
   - Estados: IDLE, LOADING, READY, PLAYING, ERROR

### Interrupciones Necesarias

1. **Timer Capture ISR (IR/RF)** 🔴 CRÍTICO

   ```c
   void TIMER_CAPTURE_IRQHandler(void) {
       // Leer timestamp
       // Calcular delta
       // Enviar a StreamBuffer
       // Notificar tarea con watermark
   }
   ```

2. **GPIO IRQ (Buttons)** 🟡 IMPORTANTE

   ```c
   void GPIO_IRQHandler(void) {
       // Leer estado de pin
       // Determinar qué botón
       // Enviar evento a UI queue
   }
   ```

3. **Timer ISR (Reproducción)** 🟡 IMPORTANTE
   ```c
   void REPLAY_TIMER_IRQHandler(void) {
       // Leer próximo valor del buffer
       // Configurar salida PWM/DIO
       // Actualizar contador
   }
   ```

### Hardware Pendiente

1. **Configurar SPI0 para SD Card** (seguir `sd_spi.c`)
2. **Configurar Timer Capture para IR** (CH1 del timer)
3. **Configurar Timer Capture para RF** (CH2 del timer)
4. **Configurar GPIO para botones** (con IRQ en edges)
5. **Configurar display LCD** (I2C/SPI según modelo)

## 📋 Plan de Implementación

### Etapa 1: Fixes Críticos (1 semana) ✅

- [x] Definir variables externas
- [ ] Agregar timeout a capturas
- [ ] Implementar watermarks
- [ ] Mejorar manejo de errores
- [ ] Crear módulos de test básicos

### Etapa 2: Integración FatFS (1 semana) ⚠️

- [ ] Integrar código de `sd_spi.c`
- [ ] Montar sistema de archivos
- [ ] Probar escritura/lectura
- [ ] Implementar formato MED

### Etapa 3: Interrupciones (1-2 semanas) ⚠️

- [ ] Implementar Timer Capture ISRs
- [ ] Implementar GPIO IRQ handlers
- [ ] Configurar Timer para replay
- [ ] Probar timing de captura

### Etapa 4: Hardware Integration (2-3 semanas) ⚠️

- [ ] Conectar módulos reales
- [ ] Ajustar parámetros de timing
- [ ] Implementar driver de LCD
- [ ] Tests end-to-end

### Etapa 5: Polish (1 semana) ⚠️

- [ ] Optimizar rendimiento
- [ ] Agregar validaciones
- [ ] Completar documentación
- [ ] Preparar demo

## 🎓 Lecciones del Análisis

1. **Start Simple**: Primero hacer que compile y corra, luego optimizar
2. **Test Early**: Crear tests desde el inicio, no al final
3. **Mock Hardware**: Usar mocks para probar sin hardware
4. **Incremental**: Implementar una cosa a la vez
5. **Document**: Documentar decisiones y cambios

## 📊 Estado Actual

| Componente           | Estado              | Prioridad Próximo Paso |
| -------------------- | ------------------- | ---------------------- |
| Arquitectura         | ✅ Completa         | -                      |
| Task Creation        | ✅ Completo         | -                      |
| Signal Capture Logic | ⚠️ Completo (stub)  | ISR Implementation     |
| Storage Logic        | ⚠️ Completo (stub)  | FatFS Integration      |
| UI FSM               | ✅ Completo         | LCD Integration        |
| Replay Logic         | ⚠️ Completo (stub)  | Timer Implementation   |
| ISR Handlers         | ❌ Stubs            | CRITICAL               |
| FatFS Integration    | ❌ Falta            | HIGH                   |
| Testing              | ⚠️ Plan Documentado | Implementation         |

## ✅ Conclusión

El firmware tiene una **base sólida** pero necesita completar:

1. **ISR implementations** (crítico para funcionamiento)
2. **FatFS integration** (crítico para almacenamiento)
3. **Testing modules** (importante para verificar diseño)
4. **Hardware integration** (final para funcionar con hardware real)

El diseño arquitectural es correcto y está bien estructurado. Los próximos pasos son implementación de bajo nivel y testing.

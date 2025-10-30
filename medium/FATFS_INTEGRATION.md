# Integración de FatFS - Médium Firmware

## 📋 Resumen

Se ha integrado FatFS en el módulo `signal_storage.c` basándonos en los ejemplos disponibles en el firmware de CIAA:

- `examples/c/sapi/spi/sd_card/fatfs_write_file`
- `examples/c/sapi/spi/sd_card/fatfs_log_time_stamp`
- `examples/c/sapi/spi/sd_card/fatfs_list_files`
- `examples/c/sapi/fatfs_sdcard_usbmsd`

## ✅ Cambios Realizados

### 1. Archivo `src/main.c`

**Agregados**:

- Inclusión de `ff.h` para FatFS
- Función `diskTickHook()` para timer de disco
- Configuración de SPI para SD card
- Configuración de tick hook (10ms) para FatFS

**Código agregado**:

```c
/* FatFS includes for disk timer */
#include "ff.h"

/* SPI configuration for SD card */
spiConfig(SPI0);

/* Configure tick hook for disk timer (required by FatFS) */
tickConfig(10);  // 10ms tick resolution
tickCallbackSet(diskTickHook, NULL);

void diskTickHook(void *ptr)
{
    disk_timerproc();  // Disk timer process (FatFS internal function)
}
```

### 2. Archivo `src/signal_storage.c`

**Funcionalidades implementadas**:

#### ✅ Montaje de SD Card

```c
static BaseType_t MountSD(void)
```

- Usa `FSSDC_InitSPI()` para inicializar hardware
- `f_mount()` para montar filesystem
- Crea directorio `SDC:/signals` automáticamente
- Manejo de errores con reintentos

#### ✅ Escritura de Archivos

```c
void vStorage_Task(void *pvParameters)
```

- Recibe paquetes desde `xStorageQueue`
- Genera nombres únicos: `signals/signal_IR_000001.sig`
- Escribe header con metadata:
  - Magic: "MED1"
  - Versión: "VER1"
  - Timestamp: `TS=<timestamp_ms>`
  - Modo: `MODE=<IR|RF>`
  - Samples: `SAMPLES=<count>`
- Escribe datos de señal
- Usa mutex para exclusión (único escritor)

#### ✅ Listado de Archivos

```c
uint32_t Storage_ListFiles(SignalFileInfo_t *file_list, uint32_t max_count)
```

- Abre directorio `SDC:/signals`
- Filtra solo archivos `.sig`
- Retorna información de cada archivo

#### ✅ Estadísticas de SD

```c
BaseType_t Storage_GetStats(uint32_t *free_space, uint32_t *total_space)
```

- Usa `f_getfree()` para obtener espacio disponible
- Calcula espacio total y libre en bytes

#### ✅ Eliminación de Archivos

```c
BaseType_t Storage_DeleteSignal(const char *filename)
```

- Usa `f_unlink()` para borrar archivos
- Maneja errores correctamente

### 3. Formato de Archivo

#### Header (versión simple)

```
MED1;VER1;TS=12345678;MODE=0;SAMPLES=100

```

#### Datos

- Array de `uint32_t` con los samples capturados
- Cada sample es: `{delta_time(24bits), level(8bits)}`

#### Estructura

```
[Header (64 bytes)] [Samples (N * 4 bytes)]
```

### 4. Configuración `config.mk`

Ya incluye:

```mk
USE_FATFS=y
```

Esto habilita la compilación de FatFS automáticamente.

## 🎯 Características Clave

### Exclusión Mutua

- Solo `Storage_Task` puede escribir en SD
- Usa semáforo para evitar corrupción
- Otros tasks solo pueden leer

### Manejo de Errores

- Reintenta montaje si falla
- Verifica que SD esté montado antes de operaciones
- LEDs indican éxito (LEDG) o error (LEDR)
- Logs por UART para debugging

### Directorio Organizado

- Todos los archivos van a `SDC:/signals/`
- Nombres únicos con contador incremental
- Extensión `.med` para identificación

## 📝 Ejemplo de Uso

### Escritura

```c
// Desde SignalCapture_Task:
packet = pvPortMalloc(sizeof(SignalPacket_t) + data_size);
packet->mode = SIGNAL_MODE_IR;
packet->timestamp_ms = xTaskGetTickCount();
packet->sample_count = num_samples;
memcpy(packet->data, samples, data_size);

// Enviar a queue
xQueueSend(xStorageQueue, &packet, portMAX_DELAY);
```

### Lectura

```c
// Desde UI o Replay_Task:
SignalFileInfo_t files[10];
uint32_t count = Storage_ListFiles(files, 10);

for (uint32_t i = 0; i < count; i++) {
    printf("%s (%d bytes)\r\n", files[i].filename, files[i].file_size);
}
```

## 🔍 Debugging

### Ver Archivos en PC

1. Conecta SD a PC
2. Abre directorio `/signals/`
3. Archivos `.med` contienen las señales

### UART Debug

```bash
# Monitorear por UART
cat /dev/ttyUSB0

# Verás:
[Storage] Storage Task started
[Storage] Initializing SD card...
[Storage] SD card mounted successfully
[Storage] Saving signal to: signals/signal_IR_000001.sig
[Storage] Wrote 256 bytes to signals/signal_IR_000001.sig
```

## ⚠️ Notas Importantes

1. **Timer Hook Obligatorio**: FatFS requiere `diskTickHook()` cada 10ms
2. **Montaje Automático**: Sistema intenta montar al iniciar
3. **Directorio**: Crea `signals/` automáticamente si no existe
4. **Mutex**: Obligatorio para evitar corrupción de datos
5. **Formato ASCII**: Header es texto para fácil debugging

## 🚀 Próximos Pasos

1. **Implementar lectura completa** (parsing de header)
2. **Agregar CRC** para validación de datos
3. **Optimizar escritura** (buffering de chunks)
4. **Implementar compresión** (opcional)
5. **Testing con SD real**

## 📊 Integración Completa

| Componente         | Estado          | Notas          |
| ------------------ | --------------- | -------------- |
| Montaje SD         | ✅ Implementado | Con reintentos |
| Escritura archivos | ✅ Implementado | Con header     |
| Listado archivos   | ✅ Implementado | Filtra .sig    |
| Estadísticas SD    | ✅ Implementado | Free/Total     |
| Eliminación        | ✅ Implementado | f_unlink       |
| Exclusión mutua    | ✅ Implementado | Semáforo       |
| Manejo errores     | ✅ Implementado | LEDs + UART    |
| Timer hook         | ✅ Implementado | 10ms tick      |
| Directorio         | ✅ Auto-creado  | signals/       |

## 🎓 Referencias

- `examples/c/sapi/spi/sd_card/fatfs_write_file/src/sd_spi.c`
- `examples/c/sapi/spi/sd_card/fatfs_list_files/src/sd_spi.c`
- `examples/c/sapi/spi/sd_card/Ejemplos SD.txt`
- FatFS documentation: http://elm-chan.org/fsw/ff/00index_e.html

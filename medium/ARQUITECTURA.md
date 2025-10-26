# Arquitectura de Firmware - Proyecto Médium

## Descripción General

El sistema implementa una **arquitectura híbrida Event-Driven / Time-Driven** sobre un **RTOS preemptivo** (FreeRTOS v10).

## Pseudocódigo de Alto Nivel

```pseudocódigo
INICIALIZAR_SISTEMA():
  1. Configurar hardware (timers, GPIOs, SD, UART)
  2. Inicializar FreeRTOS (colas, buffers, semáforos)
  3. Crear tareas principales
  4. Configurar interrupciones (IR, RF, botones)
  5. Arrancar scheduler → RTOS empieza a planificar tareas

ISR_IR_Handler():
  - Lee timestamp exacto del flanco de señal
  - Calcula delta desde el anterior
  - Envía {delta, nivel} a StreamBuffer (no bloqueante)
  - Notifica tarea si watermark alcanzado

ISR_RF_Handler():
  - Similar a IR pero para señales RF

ISR_Botones():
  - Lee estado de botones
  - Envía evento a UI_Task mediante Queue

SignalCaptureIR_Task():
  - Consume del StreamBuffer
  - Agrupa en paquetes con metadatos
  - Envía paquete a StorageQueue

SignalCaptureRF_Task():
  - Similar a IR pero para RF

Storage_Task():
  - Única tarea que escribe en microSD
  - Recibe paquetes desde StorageQueue
  - Escribe datos en bloques
  - Finaliza archivos con metadatos (CRC, timestamp)

Replay_Task():
  - Carga archivo de señal desde SD
  - Prepara buffer para reproducir
  - Controla emisión precisa con timers/DMA

UI_Task():
  - Atiende eventos de botones
  - Gestiona menú en LCD
  - Envía comandos a otras tareas

Housekeeping_Task():
  - Supervisa watchdog
  - Monitorea memoria y pila
```

## Diagrama de Flujo

### Flujo de Captura

```
Usuario presiona START
    ↓
UI_Task → SignalCapture_Start()
    ↓
ISR activa captura HW → StreamBuffer
    ↓
SignalCapture_Task → Agrupa muestras → StorageQueue
    ↓
Storage_Task → Escribe en microSD
    ↓
Usuario presiona STOP
    ↓
UI_Task → SignalCapture_Stop() → Menú Finalizado
```

### Flujo de Reproducción

```
Usuario selecciona "Reproducir" → Selecciona archivo
    ↓
UI_Task → Replay_Start(filename)
    ↓
Storage_Task (lector) → Lee archivo → Replay_Buffer
    ↓
Replay_Task → Reproduce con Timer/DMA
    ↓
Usuario presiona STOP → Replay_Stop()
```

## Capas de Software

### Capa de Aplicación

- **SignalCapture Module**: Captura IR/RF, empaqueta datos
- **Storage Module**: Gestión de archivos en microSD
- **Replay Module**: Reproducción de señales
- **UI Module**: Interfaz de usuario
- **Housekeeping Module**: Monitoreo del sistema

### Capa de RTOS

- **FreeRTOS v10**: Scheduler, tasks, queues, stream buffers, semaphores
- **Task Priorities**: 0-5
- **Memory Management**: heap_4.c

### Capa de Drivers

- **Hardware Timers**: Timer Capture para señales
- **GPIO**: Botones, LEDs, display
- **SPI/SDIO**: Comunicación con microSD
- **FatFS**: Sistema de archivos
- **UART**: Debugging

### Bibliotecas Externas

- **sAPI**: Hardware Abstraction Layer (CIAA)
- **LPCOpen**: Drivers específicos NXP LPC4337
- **CMSIS Core**: Interfaz estándar ARM Cortex-M
- **FatFS**: Sistema de archivos para SD

## Interfaz de Usuario - Estados

### Estados del Sistema

1. **MENÚ** (Estado inicial)

   - Opciones: Capturar IR, Capturar RF, Archivos
   - Transición por botón ENTER

2. **CAPTURAR SEÑAL IR**

   - Sub-estados: Listo, Capturando
   - START: Inicia captura
   - STOP: Finaliza captura → FINALIZADO

3. **CAPTURAR SEÑAL RF**

   - Sub-estados: Config frecuencia, Listo, Capturando
   - START: Inicia captura
   - STOP: Finaliza captura → FINALIZADO

4. **FINALIZADO** (después de captura)

   - Acciones: Guardar, Descartar
   - Guardar → ARCHIVOS
   - Descartar → MENÚ

5. **ARCHIVOS**
   - Opciones: Reproducir, Borrar archivo
   - Reproducir → Selección → Reproduciendo
   - Borrar → Confirmación → Archivo eliminado

### Máquina de Estados

```
MENÚ
├── (ENTER) → Capturar IR
│   ├── START → Capturando
│   ├── STOP → Finalizado
│   └── (ATRAS) → MENÚ
│
├── (ENTER) → Capturar RF
│   ├── Seleccionar Frecuencia
│   ├── START → Capturando
│   ├── STOP → Finalizado
│   └── (ATRAS) → MENÚ
│
└── (ENTER) → Archivos
    ├── Reproducir → Selección → Reproduciendo
    ├── Borrar archivo → Confirmación
    └── (ATRAS) → MENÚ

Finalizado
├── Guardar → Archivo guardado → MENÚ
├── Descartar → MENÚ
└── (ATRAS) → MENÚ
```

## Prioridades de Tareas

| Prioridad | Tarea           | Funcionalidad                    |
| --------- | --------------- | -------------------------------- |
| 5         | ISR/DMA         | Captura hardware en tiempo real  |
| 4         | SignalCaptureIR | Procesa muestras IR              |
| 4         | SignalCaptureRF | Procesa muestras RF              |
| 3         | Storage         | Escritura/lectura SD (exclusiva) |
| 2         | Replay          | Reproducción con timing preciso  |
| 1         | UI              | Interfaz de usuario              |
| 1         | Housekeeping    | Monitoreo del sistema            |
| 0         | Idle            | Tarea idle del sistema           |

## Gestión de Memoria

- **Stack por tarea**: 256-1024 bytes
- **Heap FreeRTOS**: 16KB
- **Buffer de captura**: 2KB StreamBuffer
- **Buffer de replay**: 2KB
- **Cola de storage**: 10 elementos

## Timing y Latencias

### Captura

- **ISR latency**: < 10μs (target)
- **Timer resolution**: 1μs
- **StreamBuffer**: Non-blocking desde ISR

### Reproducción

- **Reproducción precisa**: Timer compare + DMA
- **Latencia de transmisión**: < 100μs
- **Prevención de underflow**: Buffer triple

### Almacenamiento

- **Escritura en bloques**: Múltiplo de 512B
- **Latencia de escritura**: < 100ms
- **Acceso exclusivo**: Mutex de storage

## Características de la Arquitectura

### Escalabilidad

- Módulos independientes
- Separación de capas
- RTOS permite agregar tareas

### Robustez

- Manejo de errores por capa
- Recuperación automática
- Validación de datos

### Eficiencia

- Priorización de tareas
- Uso optimizado de memoria
- Minimización de latencias

### Mantenibilidad

- Código estructurado y documentado
- Interfaces claras entre módulos
- Estándares de la industria

## Formato de Archivo

### Header (64 bytes)

- Magic: "MED1"
- Versión: 1
- Timestamp UTC (epoch)
- Modo: IR o RF
- Clock base (ticks/μs)
- Polaridad inicial
- Encoding: "durations+levels"
- Flags: compresión = 0
- Length_bytes: longitud total
- CRC32 header

### Payload

- Secuencia de {delta_ticks, level_bit}
- Marcadores de frame (opcional)

### Footer (opcional)

- CRC32 del payload

## Consideraciones de Diseño

1. **Solo Storage_Task escribe en SD** (mutex exclusivo)
2. **ISRs son brief y non-blocking**
3. **Watermarks para StreamBuffer** evitan overflow
4. **Packaging en SignalCapture_Task** reduce I/O al storage
5. **Reproducción con timer** garantiza timing preciso

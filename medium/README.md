# Proyecto Médium - Firmware

## Descripción

Proyecto Médium: Dispositivo para captar y transmitir señales IR y RF

Este firmware implementa la arquitectura descrita en el documento del proyecto:

- Captura de señales IR y RF con máxima fidelidad temporal
- Almacenamiento en microSD
- Reproducción precisa de señales capturadas
- Interfaz de usuario según diagrama de flujo

## Arquitectura

### Tipo de Arquitectura

**Event-Driven / Time-Driven híbrida sobre RTOS preemptivo (FreeRTOS)**

### Componentes y Prioridades (0..5, 5 = más alta)

- **5** - ISR/DMA (hardware) - Captura de señales en tiempo real
- **4** - SignalCapture_Task (IR y RF) - Procesa StreamBuffers y envía a Storage Queue
- **3** - Storage_Task - Escritura exclusiva a microSD
- **2** - Replay_Task - Reproducción de señales con timing preciso
- **1** - UI_Task / Housekeeping_Task - Interfaz de usuario y mantenimiento del sistema
- **0** - Idle

### Flujo de Datos

**Captura:**

```
ISR → StreamBuffer → SignalCapture_Task → StorageQueue → Storage_Task → microSD
```

**Reproducción:**

```
Storage_Task → Replay_Buffer → Replay_Task (Timer/DMA) → Salida HW
```

## Estructura de Código

```
examples/c/medium_device/
├── config.mk                 # Configuración de compilación
├── src/
│   ├── main.c               # Punto de entrada principal
│   ├── signal_capture.c     # Captura de señales IR/RF
│   ├── signal_storage.c     # Almacenamiento en microSD
│   ├── signal_replay.c      # Reproducción de señales
│   ├── ui_controller.c      # Control de interfaz de usuario
│   ├── housekeeping.c       # Monitoreo del sistema
│   └── isr_handlers.c       # Handlers de interrupciones
└── inc/
    ├── signal_capture.h
    ├── signal_storage.h
    ├── signal_replay.h
    ├── ui_controller.h
    └── housekeeping.h
```

## Interfaz de Usuario (FSM)

Estados principales según diagrama de flujo:

1. **Menú Principal**

   - Capturar señal IR
   - Capturar señal RF
   - Archivos

2. **Capturar señal IR**

   - Iniciar captura
   - Parar captura
   - Guardar/Descartar (en Finalizado)

3. **Capturar señal RF**

   - Seleccionar frecuencia
   - Iniciar captura
   - Parar captura
   - Guardar/Descartar (en Finalizado)

4. **Archivos**
   - Reproducir
   - Borrar archivo

## Compilación

```bash
# Desde el directorio raíz del firmware
cd examples/c/medium_device
make clean
make all
make program
```

## Dependencias

- FreeRTOS v10
- sAPI (HAL de CIAA)
- FatFS (sistema de archivos)
- LPCOpen (drivers NXP)

## Estado de Implementación

### ✅ Implementado

- Arquitectura básica de tareas
- Estructura de módulos
- Estados de UI según diagrama
- StreamBuffers y Queues
- Pseudocódigo de flujo

### ⚠️ Pendiente

- Configuración de hardware (IR/RF modules)
- Implementación de FatFS
- Implementación de Timer Capture
- Configuración de interrupciones HW
- Implementación de LCD display
- Empaquetado y guardado real de datos

## Notas de Desarrollo

Este código es un esqueleto funcional que implementa la arquitectura propuesta.
Se requiere completar las implementaciones marcadas con `TODO` para que el sistema
sea completamente funcional con el hardware real.

## Autor

Equipo Médium - Grupo 5

- Majoros, Lorenzo
- Nihany, Santiago
- Triviño Loyola, Gastón Eduardo
- Seery, Juan Martín

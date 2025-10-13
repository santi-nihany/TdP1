#include "sapi.h"

#define SAMPLE_PERIOD_US   20        // Resolución del muestreo (20 us = 50 kHz)
#define TIMEOUT_US         40000     // 40 ms sin cambios = fin de trama
#define MAX_PULSES         200       // Cantidad máxima de pulsos a almacenar

typedef struct {
   uint8_t level;       // 0 o 1
   uint32_t duration;   // en microsegundos
} IRPulse_t;

static IRPulse_t pulseBuffer[MAX_PULSES];
static uint16_t pulseCount = 0;

static uint32_t pulseTime = 0;
static uint8_t lastLevel = 1;
static uint32_t idleTime = 0;

static bool tramaCompleta = false;

// --- Prototipo de funciones ---
void emitirTrama(void);

// ISR periódica (50 kHz)
void sampleISR(void *ptr) {
   uint8_t level = gpioRead(GPIO7);

   pulseTime += SAMPLE_PERIOD_US;
   idleTime  += SAMPLE_PERIOD_US;

   if(level != lastLevel) {
      if(pulseCount < MAX_PULSES) {
         pulseBuffer[pulseCount].level = lastLevel;
         pulseBuffer[pulseCount].duration = pulseTime;
         pulseCount++;
      }

      lastLevel = level;
      pulseTime = 0;
      idleTime  = 0;
   }

   // Fin de trama (sin cambios por 40 ms)
   if(idleTime >= TIMEOUT_US && pulseCount > 0) {

      if(pulseCount < MAX_PULSES) {
         pulseBuffer[pulseCount].level = lastLevel;
         pulseBuffer[pulseCount].duration = pulseTime;
         pulseCount++;
      }

      tramaCompleta = true; // marcar para procesar en main()

      // Reiniciar para próxima captura
      pulseTime = 0;
      idleTime  = 0;
      lastLevel = 1;
   }
}

int main(void) {
   boardConfig();
   uartConfig(UART_USB, 115200);  // Serial USB

   gpioConfig(GPIO6, GPIO_OUTPUT); // Emisor IR
   gpioConfig(GPIO7, GPIO_INPUT);  // Receptor IR

   tickConfig(SAMPLE_PERIOD_US);
   tickCallbackSet(sampleISR, NULL);

   printf("Sistema listo. Esperando señal IR...\r\n");

   while(true) {

      // Si se completó una trama
      if(tramaCompleta) {
         tramaCompleta = false;
         printf("Señal IR recibida (%d pulsos)\r\n", pulseCount);
      }

      // Si el usuario envía una 'E', emitir la señal almacenada
      if(uartRxReady(UART_USB)) {
         char c = uartRxRead(UART_USB);
         if(c == 'E' || c == 'e') {
            printf("Reproduciendo señal IR...\r\n");
            emitirTrama();
         }
      }

      __WFI(); // Esperar interrupciones
   }
}

// --- Reproducir la señal capturada ---
void emitirTrama(void) {
   for(int i = 0; i < pulseCount; i++) {
      gpioWrite(GPIO6, pulseBuffer[i].level);
      delayMicroseconds(pulseBuffer[i].duration);
   }

   // Apagar emisor al terminar
   gpioWrite(GPIO6, 0);
   printf("Emisión finalizada\r\n");
}

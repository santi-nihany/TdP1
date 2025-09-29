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

static uint32_t pulseTime = 0;       // Duración del pulso actual
static uint8_t lastLevel = 1;        // Estado inicial (reposo = 1 lógico)
static uint32_t idleTime = 0;        // Tiempo sin cambios

// Interrupción del TICK cada SAMPLE_PERIOD_US
void sampleISR(void *ptr) {
   uint8_t level = gpioRead(GPIO7);

   pulseTime += SAMPLE_PERIOD_US;
   idleTime  += SAMPLE_PERIOD_US;

   if(level != lastLevel) {
      // Cambio detectado → guardar el pulso anterior
      if(pulseCount < MAX_PULSES) {
         pulseBuffer[pulseCount].level = lastLevel;
         pulseBuffer[pulseCount].duration = pulseTime;
         pulseCount++;
      }

      // Reiniciar contadores
      lastLevel = level;
      pulseTime = 0;
      idleTime  = 0;
   }

   // Detectar fin de trama (timeout)
   if(idleTime >= TIMEOUT_US && pulseCount > 0) {
      // Guardar último pulso
      if(pulseCount < MAX_PULSES) {
         pulseBuffer[pulseCount].level = lastLevel;
         pulseBuffer[pulseCount].duration = pulseTime;
         pulseCount++;
      }

      // Señal de trama completa: imprimir resultado
      printf("Trama capturada (%d pulsos):\r\n", pulseCount);
      for(int i=0; i<pulseCount; i++) {
         printf("[%d] Nivel=%d, Dur=%lu us\r\n",
                i, pulseBuffer[i].level, pulseBuffer[i].duration);
      }

      // Resetear para próxima trama
      pulseCount = 0;
      pulseTime = 0;
      idleTime  = 0;
      lastLevel = 1;
   }
}

int main(void) {
   boardConfig();

   // Configurar entrada (receptor IR en GPIO7)
   gpioConfig(GPIO7, GPIO_INPUT);

   // Configurar Ticker de alta frecuencia
   tickConfig(SAMPLE_PERIOD_US);      // Configura base de tiempo
   tickCallbackSet(sampleISR, NULL);  // ISR periódica

   while(true) {
      // El procesamiento ocurre en la ISR
      __WFI(); // Esperar interrupción
   }
}

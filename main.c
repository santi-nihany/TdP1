#include "sapi.h"
#include "chip.h"

// Configuración para interrupciones GPIO
#define TIMEOUT_US 40000 // 40 ms sin cambios = fin de trama
#define MAX_PULSES 200 // Cantidad máxima de pulsos a almacenar
#define IR_GPIO_PIN GPIO7
#define IR_EMITTER_PIN GPIO6

// Canal de interrupción GPIO (PININT)
#define IR_GPIO_IRQ_CHANNEL 0

// Definiciones para tipos de flanco (tomadas de sapi_ultrasonic_hcsr04.c)
#define RAISING_EDGE 0    /* Flanco ascendente (0→1) */
#define FALLING_EDGE 1    /* Flanco descendente (1→0) */
#define BOTH_EDGES 2      /* Ambos flancos */

typedef struct {
uint8_t level; // 0 o 1
uint32_t duration; // en microsegundos
} IRPulse_t;

static IRPulse_t pulseBuffer[MAX_PULSES];
static uint16_t pulseCount = 0;

// Variables para medición de tiempo
static uint32_t pulseStartTime = 0;
static uint8_t lastLevel = 1;
static uint32_t lastPulseTime = 0;
static bool_t tramaCompleta = false;

// Timer para timeout
static volatile bool_t timeoutReached = false;


// --- Prototipos de funciones ---
void emitirTrama(void);
void initGPIOIrqs(void);
void enableGPIOIrq(uint8_t irqChannel, uint8_t port, uint8_t pin, uint8_t edge);
void disableGPIOIrq(uint8_t irqChannel);
void clearInterrupt(uint8_t irqChannel);
void initTimerForTimeout(void);
void startTimeoutTimer(void);
void stopTimeoutTimer(void);
uint32_t getCurrentTimeUs(void);

// --- Funciones de configuración de interrupciones GPIO ---
void initGPIOIrqs(void) {
   Chip_PININT_Init(LPC_GPIO_PIN_INT);
}

void enableGPIOIrq(uint8_t irqChannel, uint8_t port, uint8_t pin, uint8_t edge) {
   // Seleccionar canal de interrupción para el pin GPIO
   Chip_SCU_GPIOIntPinSel(irqChannel, port, pin);
   
   // Limpiar estado de interrupción
   Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(irqChannel));
   
   // Configurar modo de flanco
   Chip_PININT_SetPinModeEdge(LPC_GPIO_PIN_INT, PININTCH(irqChannel));

   if (edge == RAISING_EDGE) {
      Chip_PININT_EnableIntHigh(LPC_GPIO_PIN_INT, PININTCH(irqChannel));
   } else if (edge == FALLING_EDGE) {
      Chip_PININT_EnableIntLow(LPC_GPIO_PIN_INT, PININTCH(irqChannel));
   } else {
      // Ambos flancos
      Chip_PININT_EnableIntHigh(LPC_GPIO_PIN_INT, PININTCH(irqChannel));
      Chip_PININT_EnableIntLow(LPC_GPIO_PIN_INT, PININTCH(irqChannel));
   }

   // Limpiar interrupciones pendientes y habilitar
   NVIC_ClearPendingIRQ(PIN_INT0_IRQn + irqChannel);
   NVIC_EnableIRQ(PIN_INT0_IRQn + irqChannel);
}

void disableGPIOIrq(uint8_t irqChannel) {
   NVIC_ClearPendingIRQ(PIN_INT0_IRQn + irqChannel);
   NVIC_DisableIRQ(PIN_INT0_IRQn + irqChannel);
}

void clearInterrupt(uint8_t irqChannel) {
   Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(irqChannel));
}

// --- Funciones de timer para medición de tiempo y timeout ---
void initTimerForTimeout(void) {
   // Inicializar TIMER1 para medición de tiempo
   Chip_TIMER_Init(LPC_TIMER1);
   Chip_TIMER_PrescaleSet(LPC_TIMER1, (Chip_Clock_GetRate(CLK_MX_TIMER1) / 1000000) - 1);
   
   // Configurar TIMER2 para delays
   Chip_TIMER_Init(LPC_TIMER2);
   Chip_TIMER_PrescaleSet(LPC_TIMER2, (Chip_Clock_GetRate(CLK_MX_TIMER2) / 1000000) - 1);
}

uint32_t getCurrentTimeUs(void) {
   return Chip_TIMER_ReadCount(LPC_TIMER1);
}

void startTimeoutTimer(void) {
   // Configurar match para timeout
   Chip_TIMER_SetMatch(LPC_TIMER1, 1, TIMEOUT_US);
   Chip_TIMER_MatchEnableInt(LPC_TIMER1, 1);
   Chip_TIMER_Reset(LPC_TIMER1);
   Chip_TIMER_Enable(LPC_TIMER1);
   timeoutReached = false;
}

void stopTimeoutTimer(void) {
   Chip_TIMER_Disable(LPC_TIMER1);
   Chip_TIMER_MatchDisableInt(LPC_TIMER1, 1);
}

// Delay en microsegundos usando TIMER2
void delay_us(uint32_t us) {
   if (us == 0) return;
   Chip_TIMER_Reset(LPC_TIMER2);
   Chip_TIMER_Enable(LPC_TIMER2);
   while (Chip_TIMER_ReadCount(LPC_TIMER2) < us) {
   }
   Chip_TIMER_Disable(LPC_TIMER2);
}

// --- ISR para interrupciones GPIO ---
void PIN_INT0_IRQHandler(void) {
   uint32_t currentTime = getCurrentTimeUs();
   uint8_t currentLevel = gpioRead(IR_GPIO_PIN);
   
   // Si hay un cambio de nivel
   if (currentLevel != lastLevel) {
      // Guardar el pulso anterior si hay espacio
      if (pulseCount < MAX_PULSES) {
         pulseBuffer[pulseCount].level = lastLevel;
         pulseBuffer[pulseCount].duration = currentTime - pulseStartTime;
         pulseCount++;
      }
      
      // Actualizar variables para el próximo pulso
      lastLevel = currentLevel;
      pulseStartTime = currentTime;
      
      // Reiniciar timer de timeout
      startTimeoutTimer();
   }
   
   // Limpiar flag de interrupción
   clearInterrupt(IR_GPIO_IRQ_CHANNEL);
}

// --- ISR para timeout del timer ---
void TIMER1_IRQHandler(void) {
   if (Chip_TIMER_MatchPending(LPC_TIMER1, 1)) {
      Chip_TIMER_ClearMatch(LPC_TIMER1, 1);
      
      // Timeout alcanzado - fin de trama
      if (pulseCount > 0) {
         // Guardar último pulso
         if (pulseCount < MAX_PULSES) {
            pulseBuffer[pulseCount].level = lastLevel;
            pulseBuffer[pulseCount].duration = getCurrentTimeUs() - pulseStartTime;
            pulseCount++;
         }
         
         tramaCompleta = true;
         timeoutReached = true;
      }
      
      stopTimeoutTimer();
   }
}

int main(void) {
   boardConfig();
   uartConfig(UART_USB, 115200); // Serial USB

   gpioConfig(IR_EMITTER_PIN, GPIO_OUTPUT); // Emisor IR
   gpioConfig(IR_GPIO_PIN, GPIO_INPUT); // Receptor IR

   // Inicializar sistemas de interrupción
   initGPIOIrqs();
   initTimerForTimeout();
   
   // Configurar interrupción GPIO para ambos flancos
   enableGPIOIrq(IR_GPIO_IRQ_CHANNEL, 3, 7, BOTH_EDGES); // GPIO7 = puerto 3, pin 7
   
   // Habilitar interrupciones del timer
   NVIC_EnableIRQ(TIMER1_IRQn);

   printf("Sistema listo. Esperando señal IR...\r\n");

   while(true) {
      // Si se completó una trama
      if(tramaCompleta) {
         tramaCompleta = false;
         printf("Señal IR recibida (%d pulsos)\r\n", pulseCount);
         printf("Datos de la señal IR...\r\n");
         for(int i = 0; i < pulseCount; i++) {
            printf("|%d - %d us|)", pulseBuffer[i].level,pulseBuffer[i].duration);
         }
         printf("...\r\n");
         printf("Reproduciendo señal IR...\r\n");
         emitirTrama();
         
         // Resetear para próxima captura
         pulseCount = 0;
         lastLevel = 1;
         timeoutReached = false;
      }
   }
}

// --- Reproducir la señal capturada ---
void emitirTrama(void) {
   for(int i = 0; i < pulseCount; i++) {
      gpioWrite(IR_EMITTER_PIN, pulseBuffer[i].level);
      delay_us(pulseBuffer[i].duration);
   }
   
   // Limpiar buffer
   for (int i = 0; i < MAX_PULSES; i++){
      pulseBuffer[i].level = 0;
      pulseBuffer[i].duration = 0;
   }

   // Apagar emisor al terminar
   gpioWrite(IR_EMITTER_PIN, 0);
   printf("Emisión finalizada\r\n");
}
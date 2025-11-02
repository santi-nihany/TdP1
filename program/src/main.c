//===----------------------------------------------------------------------===//
///
/// \file
/// Inicio del programa. Contiene llamados a inicialización y bucle principal.
///
//===----------------------------------------------------------------------===//

#include "main.h"

int main(void) {
  // Inicializar placa
  boardConfig();

  uint8_t leds[] = {LEDB, LED1, LED2, LED3};
  uint8_t i = 1;
  while (1) {
    gpioWrite(leds[i], OFF);
    i = (i + 1) % sizeof(leds);
    gpioWrite(leds[i], ON);
    delay(100);
  }

  return 0;
}
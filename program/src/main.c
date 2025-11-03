//===----------------------------------------------------------------------===//
///
/// \file
/// Inicio del programa. Contiene llamados a inicialización y bucle principal.
///
//===----------------------------------------------------------------------===//

#include "main.h"
#include "sh1106.h"
#include "sprites.h"

int main(void) {
  // Inicializar placa, puertos y protocolos
  boardConfig();
  i2cInit(I2C0, 100000);

  // Inicializar componentes
  sh1106_init();

  sh1106_place(Sprite_0001, 0, 0);
  sh1106_place(pixil_frame_0_2_, 8, 1);
  sh1106_update();

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
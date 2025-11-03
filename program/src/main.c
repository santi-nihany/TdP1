//===----------------------------------------------------------------------===//
///
/// \file
/// Inicio del programa. Contiene llamados a inicialización y bucle principal.
///
//===----------------------------------------------------------------------===//

#include "main.h"
#include "display.h"
#include "sprites.h"

int main(void) {
  // Inicializar placa, puertos y protocolos
  boardConfig();
  i2cInit(I2C0, 100000);

  // Inicializar componentes
  displayInit();

  displayPlace(Sprite_0001, 0, 0);
  displayUpdate();

  bool_t tec1, tec2;
  while (1) {
    if (tec1 != !gpioRead(TEC1)) {
      tec1 = !gpioRead(TEC1);
      gpioWrite(LED1, tec1);
      if (tec1) {
        displayDrawRectangle(64, 32, Sprite_0002.width, Sprite_0002.height,
                             DISPLAY_BLACK, true);
        displayDrawRectangle(104, 32, Sprite_0003.width, Sprite_0003.height,
                             DISPLAY_BLACK, true);
        displayPlace(Sprite_0002, 19, 32);
        displayPlace(Sprite_0003, 59, 32);
        displayUpdate();
      }
    }
    if (tec2 != !gpioRead(TEC2)) {
      tec2 = !gpioRead(TEC2);
      gpioWrite(LED2, tec2);
      if (tec2) {
        displayDrawRectangle(19, 32, Sprite_0002.width, Sprite_0002.height,
                             DISPLAY_BLACK, true);
        displayDrawRectangle(59, 32, Sprite_0003.width, Sprite_0003.height,
                             DISPLAY_BLACK, true);
        displayPlace(Sprite_0002, 64, 32);
        displayPlace(Sprite_0003, 104, 32);
        displayUpdate();
      }
    }

    delay(50);
  }

  return 0;
}
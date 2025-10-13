#include <IRremote.hpp>   // Librería IRremote moderna (v3.x o superior)

// ==================== CONFIGURACIÓN ====================

// Pines
#define PIN_IR_SEND    3    // Pin de salida IR LED
#define PIN_IR_RECV    2    // Pin de entrada IR receptor
#define PIN_BUTTON     4    // Pulsador
#define PIN_LED        13   // LED indicador

// Parámetros NEC (pueden cambiarse fácilmente)
#define NEC_ADDRESS    0x10AF   // Dirección arbitraria
#define NEC_COMMAND    0xA25D   // Comando arbitrario

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  IrSender.begin(PIN_IR_SEND, ENABLE_LED_FEEDBACK); // Inicia emisor
  IrReceiver.begin(PIN_IR_RECV, ENABLE_LED_FEEDBACK); // Inicia receptor

  Serial.println("Sistema IR listo.");
  Serial.println("Presione el botón para emitir una señal NEC...");
}

// ==================== LOOP ====================
void loop() {

  // --- Cuando se presiona el botón ---
  if (digitalRead(PIN_BUTTON) == LOW) {
    Serial.println("Transmitiendo señal NEC...");
    IrSender.sendNEC(NEC_ADDRESS, NEC_COMMAND, 0);  // Enviar una sola vez
    delay(1000); // Pequeño retardo para evitar rebote
  }

  // --- Escucha de señales IR ---
  if (IrReceiver.decode()) {
    // Extraer datos recibidos
    uint16_t recvAddress = IrReceiver.decodedIRData.address;
    uint16_t recvCommand = IrReceiver.decodedIRData.command;
    decode_type_t proto = IrReceiver.decodedIRData.protocol;

    Serial.print("Recibido -> Protocolo: ");
    Serial.print(getProtocolString(proto));
    Serial.print(", Addr: 0x");
    Serial.print(recvAddress, HEX);
    Serial.print(", Cmd: 0x");
    Serial.println(recvCommand, HEX);

    // Comparar con la señal esperada
    if (proto == NEC && recvAddress == NEC_ADDRESS && recvCommand == NEC_COMMAND) {
      digitalWrite(PIN_LED, HIGH);
      Serial.println("Coincidencia detectada ");
    } else {
      digitalWrite(PIN_LED, LOW);
      Serial.println("No coincide ");
    }

    IrReceiver.resume(); // Preparar para la próxima recepción
  }
}

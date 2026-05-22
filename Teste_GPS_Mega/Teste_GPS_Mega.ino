/*
  Teste da comunicacao Mega <- ESP32.

  Use depois de gravar ESP32_GPS_Bridge.ino no ESP32.

  Ligacoes:
  GND ESP32      -> GND Mega
  VIN ESP32      -> 5V Mega
  TX GPIO 1 ESP32 -> RX1 pino 19 Mega
  TX1 pino 18 Mega -> RX GPIO 3 ESP32 via divisor de tensao
*/

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  Serial.println("Teste GPS/ESP32 iniciado");
  Serial.println("Aguardando linhas vindas do ESP32 em Serial1...");
}

void loop() {
  while (Serial1.available()) {
    Serial.write(Serial1.read());
  }
}

/*
  Ponte GPS - ESP32

  Bibliotecas para instalar na IDE Arduino:
  - TinyGPSPlus

  Ligações:
  GPS NEO-6M TX -> GPIO 16 do ESP32
  GPS NEO-6M RX -> GPIO 17 do ESP32
  ESP32 TX GPIO 1 -> RX1 pino 19 do Mega
  Mega TX1 pino 18 -> ESP32 RX GPIO 3 via divisor de tensao

  Observacao:
  Os pinos GPIO 1 e GPIO 3 tambem sao usados pela USB serial do ESP32.
  Para gravar o ESP32 com mais facilidade, desconecte temporariamente os fios
  entre ESP32 e Mega.
*/

#include <TinyGPSPlus.h>

const int GPS_RX_PIN = 16;
const int GPS_TX_PIN = 17;
const unsigned long INTERVALO_ENVIO_MS = 1000;

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

unsigned long ultimoEnvio = 0;

void setup() {
  Serial.begin(9600);  // Envia dados para o Mega pelos pinos GPIO 1 e GPIO 3.
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
}

void loop() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  unsigned long agora = millis();
  if (agora - ultimoEnvio >= INTERVALO_ENVIO_MS) {
    ultimoEnvio = agora;
    enviarGpsParaMega();
  }
}

void enviarGpsParaMega() {
  bool valido = gps.location.isValid() && gps.location.age() < 5000;

  Serial.print("GPS,");
  Serial.print(valido ? 1 : 0);
  Serial.print(",");

  if (valido) {
    Serial.print(gps.location.lat(), 6);
    Serial.print(",");
    Serial.println(gps.location.lng(), 6);
  } else {
    Serial.println("NA,NA");
  }
}

/*
  Teste do GPS NEO-6M ligado direto no Arduino Mega.

  Biblioteca para instalar:
  - TinyGPSPlus

  Ligacoes recomendadas:
  VCC GPS -> 3.3V ou 5V, conforme o seu modulo aceitar
  GND GPS -> GND Mega
  TX GPS  -> RX1 pino 19 Mega

  O fio RX do GPS pode ficar desligado. Para ler localizacao, o Mega so
  precisa receber o TX do GPS.
*/

#include <TinyGPSPlus.h>

TinyGPSPlus gps;
unsigned long ultimoRelatorio = 0;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  Serial.println("Teste GPS direto iniciado");
  Serial.println("GPS TX -> Mega RX1 pino 19");
  Serial.println("Aguarde alguns minutos em local aberto para fixar satelites.");
}

void loop() {
  while (Serial1.available()) {
    gps.encode(Serial1.read());
  }

  if (millis() - ultimoRelatorio >= 1000) {
    ultimoRelatorio = millis();

    Serial.print("Caracteres recebidos: ");
    Serial.print(gps.charsProcessed());
    Serial.print(" | Sats: ");
    Serial.print(gps.satellites.isValid() ? gps.satellites.value() : 0);
    Serial.print(" | GPS: ");

    if (gps.location.isValid()) {
      Serial.print(gps.location.lat(), 6);
      Serial.print(",");
      Serial.println(gps.location.lng(), 6);
    } else {
      Serial.println("sem posicao ainda");
    }
  }
}

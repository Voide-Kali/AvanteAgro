/*
  Diagnostico I2C para Arduino Mega

  Use este sketch para verificar se RTC DS3231, LCD I2C e BME280 aparecem
  no barramento I2C.

  No Mega:
  SDA -> pino 20
  SCL -> pino 21
*/

#include <Wire.h>

void setup() {
  Serial.begin(9600);
  Wire.begin();

  Serial.println("Scanner I2C iniciado");
  Serial.println("Procurando dispositivos...");
}

void loop() {
  byte encontrados = 0;

  for (byte endereco = 1; endereco < 127; endereco++) {
    Wire.beginTransmission(endereco);
    byte erro = Wire.endTransmission();

    if (erro == 0) {
      Serial.print("Dispositivo encontrado em 0x");
      if (endereco < 16) {
        Serial.print("0");
      }
      Serial.println(endereco, HEX);
      encontrados++;
    }
  }

  if (encontrados == 0) {
    Serial.println("Nenhum dispositivo I2C encontrado.");
  } else {
    Serial.print("Total encontrado: ");
    Serial.println(encontrados);
  }

  Serial.println();
  delay(3000);
}

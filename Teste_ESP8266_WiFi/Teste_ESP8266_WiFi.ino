/*
  Teste simples de Wi-Fi para ESP8266.

  Antes de integrar com o Mega, carregue este sketch no ESP8266 e veja
  se ele conecta ao seu Wi-Fi.
*/

#include <ESP8266WiFi.h>

const char *WIFI_SSID = "FPINA";
const char *WIFI_PASSWORD = "perseveranca";

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Teste Wi-Fi ESP8266");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Conectando");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Conectado. IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
}

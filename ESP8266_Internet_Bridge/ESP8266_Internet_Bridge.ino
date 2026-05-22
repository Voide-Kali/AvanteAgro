/*
  Ponte Wi-Fi com ESP8266 para receber dados do Arduino Mega e enviar
  para a internet.

  Placa na IDE Arduino:
  - NodeMCU 1.0 (ESP-12E Module), se for NodeMCU
  - Generic ESP8266 Module, se for ESP8266MOD/ESP-12 sem placa NodeMCU

  Bibliotecas:
  - ESP8266WiFi ja vem com o pacote ESP8266
  - ESP8266HTTPClient ja vem com o pacote ESP8266

  Ligacao com o Mega usando SoftwareSerial:
  Mega TX2 pino 17 -> RX do ESP8266 via divisor de tensao para 3.3V
  Mega GND         -> GND ESP8266

  NodeMCU/Wemos:
  RX do ESP8266 neste codigo = D5

  ESP8266MOD/ESP-12:
  RX do ESP8266 neste codigo = GPIO14
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>

const char *WIFI_SSID = "FPINA";
const char *WIFI_PASSWORD = "perseveranca";

// Troque depois pela URL do seu servidor, planilha, API ou webhook.
const char *SERVER_URL = "http://example.com/receber-dados";

const int PIN_RX_MEGA = 14;  // D5 no NodeMCU, GPIO14 no ESP8266MOD.
const int PIN_TX_MEGA = 13;  // D7 no NodeMCU, GPIO13. Opcional.

SoftwareSerial megaSerial(PIN_RX_MEGA, PIN_TX_MEGA);

void setup() {
  Serial.begin(115200);
  megaSerial.begin(9600);

  Serial.println();
  Serial.println("ESP8266 ponte internet iniciado");
  conectarWifi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    conectarWifi();
  }

  if (megaSerial.available()) {
    String linha = megaSerial.readStringUntil('\n');
    linha.trim();

    if (linha.startsWith("DATA,")) {
      linha.remove(0, 5);
      Serial.print("Recebido do Mega: ");
      Serial.println(linha);
      enviarParaServidor(linha);
    }
  }
}

void conectarWifi() {
  Serial.print("Conectando no Wi-Fi ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 20000) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi OK. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi ERRO. Vou tentar de novo depois.");
  }
}

void enviarParaServidor(const String &linha) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Sem Wi-Fi, nao enviou.");
    return;
  }

  WiFiClient client;
  HTTPClient http;

  if (!http.begin(client, SERVER_URL)) {
    Serial.println("Erro ao iniciar HTTP.");
    return;
  }

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  String corpo = "linha=" + urlEncode(linha);
  int codigo = http.POST(corpo);

  Serial.print("HTTP codigo: ");
  Serial.println(codigo);

  http.end();
}

String urlEncode(const String &texto) {
  String out = "";
  const char *hex = "0123456789ABCDEF";

  for (unsigned int i = 0; i < texto.length(); i++) {
    char c = texto.charAt(i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += c;
    } else if (c == ' ') {
      out += '+';
    } else {
      out += '%';
      out += hex[(c >> 4) & 0x0F];
      out += hex[c & 0x0F];
    }
  }

  return out;
}

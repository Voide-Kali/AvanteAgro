/*
  Teste isolado do modulo SD no Arduino Mega.

  Ligacoes:
  VCC  -> 5V
  GND  -> GND
  MISO -> 50
  MOSI -> 51
  SCK  -> 52
  CS   -> 48

  Use cartao em FAT32. Cartoes de 4GB ou 8GB costumam dar menos problema.
*/

#include <SPI.h>
#include <SD.h>

const byte PIN_SD_CS = 48;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
  }

  pinMode(53, OUTPUT);
  pinMode(PIN_SD_CS, OUTPUT);
  digitalWrite(PIN_SD_CS, HIGH);

  Serial.println("Teste SD iniciado");
  Serial.println("Inicializando cartao...");

  if (!SD.begin(PIN_SD_CS)) {
    Serial.println("ERRO: SD.begin falhou.");
    Serial.println("Confira CS=48, MISO=50, MOSI=51, SCK=52, GND e cartao FAT32.");
    return;
  }

  Serial.println("SD: OK");

  File arquivo = SD.open("TESTE.TXT", FILE_WRITE);
  if (!arquivo) {
    Serial.println("ERRO: nao abriu TESTE.TXT para escrita.");
    return;
  }

  arquivo.println("Teste de escrita OK");
  arquivo.close();
  Serial.println("Escrita em TESTE.TXT: OK");

  arquivo = SD.open("TESTE.TXT", FILE_READ);
  if (!arquivo) {
    Serial.println("ERRO: nao abriu TESTE.TXT para leitura.");
    return;
  }

  Serial.println("Conteudo do arquivo:");
  while (arquivo.available()) {
    Serial.write(arquivo.read());
  }
  arquivo.close();
  Serial.println();
  Serial.println("Teste SD finalizado.");
}

void loop() {
}

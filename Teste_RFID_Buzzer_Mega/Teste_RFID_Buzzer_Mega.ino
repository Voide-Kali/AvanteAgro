/*
  Teste isolado RFID RC522 + buzzer no Arduino Mega.

  Ligacoes RC522:
  VCC      -> 3.3V
  GND      -> GND
  RST      -> pino 5
  SDA / SS -> pino 53
  MOSI     -> pino 51
  MISO     -> pino 50
  SCK      -> pino 52

  Ligacoes buzzer:
  + ou S -> pino 7
  -      -> GND
*/

#include <SPI.h>
#include <MFRC522.h>

const byte PIN_RFID_RST = 5;
const byte PIN_RFID_SS = 53;
const byte PIN_BUZZER = 7;
const bool BUZZER_ATIVO = true;  // Se for buzzer passivo, troque para false.

MFRC522 rfid(PIN_RFID_SS, PIN_RFID_RST);

void setup() {
  Serial.begin(9600);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  SPI.begin();
  rfid.PCD_Init();

  Serial.println("Teste RFID + buzzer iniciado");
  Serial.println("Aproxime uma tag/cartao do RC522...");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      uid += "0";
    }
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  Serial.print("RFID lido: ");
  Serial.println(uid);
  apitar();

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(700);
}

void apitar() {
  if (BUZZER_ATIVO) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(350);
    digitalWrite(PIN_BUZZER, LOW);
  } else {
    tone(PIN_BUZZER, 1800, 350);
    delay(380);
    noTone(PIN_BUZZER);
  }
}

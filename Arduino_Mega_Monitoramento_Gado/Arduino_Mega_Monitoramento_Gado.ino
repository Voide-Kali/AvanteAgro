/*
  Monitoramento de Gado - Arduino Mega 2560
  Versao 2.0 - Reescrito do zero

  Ligacoes:
  LCD/RTC/BME280 I2C : SDA 20, SCL 21
  RFID RC522         : SS 53, RST 5, MOSI 51, MISO 50, SCK 52
  SD card            : CS 48, MOSI 51, MISO 50, SCK 52
  PIR                : pino 6
  Buzzer             : pino 7
  HC-SR04            : Trig 9, Echo 10
  GPS NEO-6M         : TX do GPS -> RX1 (pino 19 do Mega)
  Botao troca tela   : pino 8 -> GND (pull-up interno)
*/

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <MFRC522.h>
#include <Adafruit_BME280.h>
#include <TinyGPSPlus.h>

// ─── Pinos ───────────────────────────────────────────────
#define PIN_PIR          6
#define PIN_BUZZER       7
#define PIN_BOTAO        8
#define PIN_TRIG         9
#define PIN_ECHO         10
#define PIN_RFID_RST     5
#define PIN_RFID_SS      53
#define PIN_SD_CS        48

// ─── Configuracoes ────────────────────────────────────────
#define TOTAL_TELAS      3
#define INTERVALO_LEITURA    5000   // le sensores a cada 5s
#define INTERVALO_LCD        300    // atualiza LCD a cada 300ms (tempo real)
#define DEBOUNCE_BOTAO       50     // debounce do botao em ms
#define PIR_ESTABILIZACAO    60000  // tempo aquecimento PIR
#define PIR_SEGURA_MOV       3000   // mantem movimento por 3s apos deteccao
#define PIR_AMOSTRAS         10
#define PIR_MIN_ALTAS        7
#define BUZZER_ATIVO         true   // false se buzzer for passivo

// ─── Objetos ─────────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;
MFRC522 rfid(PIN_RFID_SS, PIN_RFID_RST);
Adafruit_BME280 bme;
TinyGPSPlus gps;

// ─── Status dos modulos ───────────────────────────────────
bool rtcOk   = false;
bool bmeOk   = false;
bool sdOk    = false;
bool rfidOk  = false;
byte rfidVer = 0;

// ─── Dados dos sensores ───────────────────────────────────
float tempAr      = NAN;
float umidade     = NAN;
float pressao     = NAN;
float distancia   = NAN;
bool  movimento   = false;
bool  movAnterior = false;
bool  pirRaw      = false;
bool  pirAquecendo = true;
unsigned long ultimoMovimento = 0;

String gpsLat  = "NA";
String gpsLng  = "NA";
String gpsOk   = "0";
String gpsSat  = "0";
unsigned long gpsChars = 0;

// ─── Controle de tempo ────────────────────────────────────
unsigned long tLeitura    = 0;
unsigned long tLcd        = 0;
unsigned long tSd         = 0;
unsigned long tRfid       = 0;
unsigned long buzzerAte   = 0;

// ─── LCD e botao ─────────────────────────────────────────
byte telaAtual    = 0;
bool botaoAntes   = HIGH;
unsigned long tBotao = 0;

// ═════════════════════════════════════════════════════════
void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  pinMode(PIN_PIR,     INPUT);
  pinMode(PIN_BUZZER,  OUTPUT);
  pinMode(PIN_TRIG,    OUTPUT);
  pinMode(PIN_ECHO,    INPUT);
  pinMode(PIN_BOTAO,   INPUT_PULLUP);
  pinMode(PIN_RFID_SS, OUTPUT);
  pinMode(PIN_SD_CS,   OUTPUT);

  digitalWrite(PIN_RFID_SS, HIGH);
  digitalWrite(PIN_SD_CS,   HIGH);
  digitalWrite(PIN_BUZZER,  LOW);

  // LCD - splash
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Monitoramento");
  lcd.setCursor(0, 1); lcd.print("Gado v2.0");

  Wire.begin();
  SPI.begin();

  // RTC
  rtcOk = rtc.begin();
  if (rtcOk && rtc.lostPower())
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // BME280
  bmeOk = bme.begin(0x76);
  if (!bmeOk) bmeOk = bme.begin(0x77);

  // SD
  iniciarSd();

  // RFID
  iniciarRfid();

  Serial.println(F("=== Sistema iniciado ==="));
  Serial.print(F("RTC: "));    Serial.println(rtcOk  ? F("OK") : F("ERRO"));
  Serial.print(F("BME280: ")); Serial.println(bmeOk  ? F("OK") : F("ERRO"));
  Serial.print(F("SD: "));     Serial.println(sdOk   ? F("OK") : F("ERRO"));
  Serial.print(F("RFID: "));   Serial.println(rfidOk ? F("OK") : F("ERRO"));

  delay(1500);
  lcd.clear();
  lerSensores();
  mostrarTela();
}

// ═════════════════════════════════════════════════════════
void loop() {
  unsigned long agora = millis();

  lerGps();
  verificarRfid();
  lerPir();
  atualizarBuzzer();
  lerBotao();
  reconectarSd();
  reconectarRfid();

  // Leitura periodica dos sensores
  if (agora - tLeitura >= INTERVALO_LEITURA) {
    tLeitura = agora;
    lerSensores();
    gravarSd("LEITURA", "");
    imprimirSerial();
  }

  // Atualiza LCD em tempo real (sem trocar de tela)
  if (agora - tLcd >= INTERVALO_LCD) {
    tLcd = agora;
    mostrarTela();
  }
}

// ─── Botao ────────────────────────────────────────────────
void lerBotao() {
  bool estado = digitalRead(PIN_BOTAO);
  unsigned long agora = millis();

  if (estado == LOW && botaoAntes == HIGH && (agora - tBotao) > DEBOUNCE_BOTAO) {
    tBotao = agora;
    telaAtual = (telaAtual + 1) % TOTAL_TELAS;
    mostrarTela();
    beep(1000, 60);
  }
  botaoAntes = estado;
}

// ─── Telas do LCD (atualiza em tempo real) ────────────────
void mostrarTela() {
  lcd.clear();

  if (telaAtual == 0) {
    // Tela 1: Temperatura, Umidade, Pressao
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(isnan(tempAr) ? "NA" : String(tempAr, 1) + "C");
    lcd.print(" U:");
    lcd.print(isnan(umidade) ? "NA" : String((int)umidade) + "%");

    lcd.setCursor(0, 1);
    lcd.print("P:");
    lcd.print(isnan(pressao) ? "NA" : String((int)pressao) + "hPa");

  } else if (telaAtual == 1) {
    // Tela 2: Movimento, Distancia, SD
    lcd.setCursor(0, 0);
    lcd.print("Mov:");
    if (pirAquecendo) {
      lcd.print("AQUEC");
    } else {
      lcd.print(movimento ? "SIM " : "NAO ");
    }
    lcd.print("SD:");
    lcd.print(sdOk ? "OK" : "NA");

    lcd.setCursor(0, 1);
    lcd.print("Dist:");
    lcd.print(isnan(distancia) ? "NA  " : String((int)distancia) + "cm");

  } else {
    // Tela 3: GPS e Hora
    lcd.setCursor(0, 0);
    lcd.print("GPS:");
    lcd.print(gpsOk == "1" ? "OK" : "NA");
    lcd.print(" Sat:");
    lcd.print(gpsSat);

    lcd.setCursor(0, 1);
    if (rtcOk) {
      DateTime now = rtc.now();
      char buf[9];
      snprintf(buf, sizeof(buf), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
      lcd.print(buf);
    } else {
      lcd.print("RTC NA");
    }
  }
}

// ─── Sensores ─────────────────────────────────────────────
void lerSensores() {
  if (bmeOk) {
    tempAr  = bme.readTemperature();
    umidade = bme.readHumidity();
    pressao = bme.readPressure() / 100.0F;
  }
  distancia = medirDistancia();
}

void lerPir() {
  if (millis() < PIR_ESTABILIZACAO) {
    pirAquecendo = true;
    pirRaw = digitalRead(PIN_PIR) == HIGH;
    movimento = false;
    return;
  }

  pirAquecendo = false;
  byte altas = 0;
  for (byte i = 0; i < PIR_AMOSTRAS; i++) {
    if (digitalRead(PIN_PIR) == HIGH) altas++;
    delay(5);
  }

  pirRaw = altas >= PIR_MIN_ALTAS;
  if (pirRaw) ultimoMovimento = millis();

  movAnterior = movimento;
  movimento = (millis() - ultimoMovimento) <= PIR_SEGURA_MOV;

  if (movimento && !movAnterior) {
    beep(1200, 150);
  }
}

float medirDistancia() {
  float a = medirUmaVez(); delay(20);
  float b = medirUmaVez(); delay(20);
  float c = medirUmaVez();
  return mediana(a, b, c);
}

float medirUmaVez() {
  digitalWrite(PIN_TRIG, LOW);  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  unsigned long d = pulseIn(PIN_ECHO, HIGH, 30000UL);
  return d == 0 ? NAN : d / 58.0;
}

float mediana(float a, float b, float c) {
  bool va = !isnan(a), vb = !isnan(b), vc = !isnan(c);
  if (va && vb && vc) {
    if ((a >= b && a <= c) || (a >= c && a <= b)) return a;
    if ((b >= a && b <= c) || (b >= c && b <= a)) return b;
    return c;
  }
  if (va && vb) return (a + b) / 2.0;
  if (va && vc) return (a + c) / 2.0;
  if (vb && vc) return (b + c) / 2.0;
  if (va) return a; if (vb) return b; if (vc) return c;
  return NAN;
}

// ─── GPS ─────────────────────────────────────────────────
void lerGps() {
  while (Serial1.available()) gps.encode(Serial1.read());

  bool valido = gps.location.isValid() && gps.location.age() < 5000;
  gpsOk = valido ? "1" : "0";
  gpsLat = valido ? String(gps.location.lat(), 6) : "NA";
  gpsLng = valido ? String(gps.location.lng(), 6) : "NA";
  gpsSat = gps.satellites.isValid() ? String(gps.satellites.value()) : "0";
  gpsChars = gps.charsProcessed();
}

// ─── RFID ─────────────────────────────────────────────────
void verificarRfid() {
  if (!rfidOk) return;
  spiNenhum();

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    spiNenhum(); return;
  }

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  Serial.print(F("RFID: ")); Serial.println(uid);
  beep(1800, 350);
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  spiNenhum();
  gravarSd("RFID", uid);

  // Mostra UID no LCD por 2 segundos sem travar o sistema
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("RFID detectado");
  lcd.setCursor(0, 1); lcd.print(uid.substring(0, 16));
  delay(2000);
  mostrarTela();
}

// ─── Buzzer ───────────────────────────────────────────────
void beep(unsigned int freq, unsigned long ms) {
  buzzerAte = millis() + ms;
  if (BUZZER_ATIVO) digitalWrite(PIN_BUZZER, HIGH);
  else              tone(PIN_BUZZER, freq);
}

void atualizarBuzzer() {
  if (buzzerAte > 0 && millis() >= buzzerAte) {
    buzzerAte = 0;
    if (BUZZER_ATIVO) digitalWrite(PIN_BUZZER, LOW);
    else              noTone(PIN_BUZZER);
  }
}

// ─── SD ───────────────────────────────────────────────────
void iniciarSd() {
  sdOk = false;
  for (byte t = 0; t < 3; t++) {
    spiNenhum(); delay(100);
    if (SD.begin(PIN_SD_CS)) {
      sdOk = true;
      cabecalhoSd();
      spiNenhum();
      return;
    }
    delay(300);
  }
  spiNenhum();
}

void cabecalhoSd() {
  spiNenhum();
  digitalWrite(PIN_RFID_SS, HIGH);
  if (!SD.exists("DADOS.CSV")) {
    File f = SD.open("DADOS.CSV", FILE_WRITE);
    if (f) {
      f.println("data,hora,temp_c,umidade_pct,pressao_hpa,distancia_cm,movimento,gps_ok,lat,lng,satelites,evento,rfid_uid");
      f.close();
    }
  }
  digitalWrite(PIN_SD_CS, HIGH);
  spiNenhum();
}

void gravarSd(const String &evento, const String &uid) {
  String data = "NA", hora = "NA";
  if (rtcOk) {
    DateTime now = rtc.now();
    char bd[11], bh[9];
    snprintf(bd, sizeof(bd), "%04d-%02d-%02d", now.year(), now.month(), now.day());
    snprintf(bh, sizeof(bh), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    data = bd; hora = bh;
  }

  String linha = data + "," + hora + ",";
  linha += (isnan(tempAr)    ? "NA" : String(tempAr, 1))    + ",";
  linha += (isnan(umidade)   ? "NA" : String(umidade, 1))   + ",";
  linha += (isnan(pressao)   ? "NA" : String(pressao, 1))   + ",";
  linha += (isnan(distancia) ? "NA" : String(distancia, 1)) + ",";
  linha += String(movimento ? 1 : 0) + ",";
  linha += gpsOk + "," + gpsLat + "," + gpsLng + "," + gpsSat + ",";
  linha += evento + "," + uid;

  Serial.println(linha);

  if (!sdOk) return;
  spiNenhum();
  digitalWrite(PIN_RFID_SS, HIGH);
  File f = SD.open("DADOS.CSV", FILE_WRITE);
  if (f) { f.println(linha); f.close(); }
  else Serial.println(F("Erro SD"));
  digitalWrite(PIN_SD_CS, HIGH);
  spiNenhum();
}

void reconectarSd() {
  if (sdOk || millis() - tSd < 10000) return;
  tSd = millis();
  iniciarSd();
  Serial.print(F("SD: ")); Serial.println(sdOk ? F("OK") : F("ERRO"));
}

// ─── RFID init ────────────────────────────────────────────
void iniciarRfid() {
  spiNenhum();
  rfid.PCD_Init();
  delay(50);
  rfidVer = rfid.PCD_ReadRegister(MFRC522::VersionReg);
  rfidOk  = rfidVer != 0x00 && rfidVer != 0xFF;
  spiNenhum();
}

void reconectarRfid() {
  if (rfidOk || millis() - tRfid < 10000) return;
  tRfid = millis();
  iniciarRfid();
  Serial.print(F("RFID: ")); Serial.println(rfidOk ? F("OK") : F("ERRO"));
}

void spiNenhum() {
  digitalWrite(PIN_RFID_SS, HIGH);
  digitalWrite(PIN_SD_CS,   HIGH);
}

// ─── Monitor serial ───────────────────────────────────────
void imprimirSerial() {
  Serial.print(F("Temp: "));     Serial.print(isnan(tempAr)  ? "NA" : String(tempAr, 1));
  Serial.print(F("C | Umid: ")); Serial.print(isnan(umidade) ? "NA" : String(umidade, 1));
  Serial.print(F("% | Press: "));Serial.print(isnan(pressao) ? "NA" : String(pressao, 1));
  Serial.print(F("hPa | Dist: "));Serial.print(isnan(distancia) ? "NA" : String(distancia, 1));
  Serial.print(F("cm | Mov: "));
  if (pirAquecendo) Serial.print(F("AQUECENDO"));
  else Serial.print(movimento ? F("SIM") : F("NAO"));
  Serial.print(F(" | GPS: ")); Serial.print(gpsOk == "1" ? F("OK") : F("NA"));
  Serial.print(F(" | Sat: ")); Serial.println(gpsSat);
}

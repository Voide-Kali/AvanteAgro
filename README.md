# AvanteAgro

Sistema embarcado de monitoramento de gado desenvolvido para Arduino Mega 2560.

Autor: void  
Plataforma: Arduino Mega 2560  
Linguagem: C++ (Arduino)  
Licença: MIT  

---

## Sobre o Projeto

O AvanteAgro é um sistema de monitoramento em tempo real para ambientes rurais, desenvolvido para auxiliar no controle e segurança de rebanhos bovinos. O sistema coleta dados ambientais e de presença, registra eventos com data e hora, identifica animais via RFID e exibe as informações em um display LCD com navegação por botão físico.

---

## Funcionalidades

- Monitoramento de temperatura, umidade e pressão atmosférica via BME280
- Medição de distância via sensor ultrassônico HC-SR04
- Detecção de movimento com sensor PIR com confirmação por múltiplas amostras
- Rastreamento via GPS NEO-6M com coordenadas e número de satélites
- Registro de data e hora com RTC DS3231
- Identificação de animais via RFID RC522
- Gravação de logs em cartão SD no formato CSV
- Display LCD 16x2 I2C com atualização em tempo real
- Navegação entre telas por botão físico
- Reconexão automática de SD e RFID em caso de falha

---

## Hardware Necessário

- Arduino Mega 2560
- Sensor BME280 (temperatura, umidade, pressão)
- Sensor ultrassônico HC-SR04
- Sensor de movimento PIR
- Módulo GPS NEO-6M
- Módulo RTC DS3231
- Leitor RFID RC522
- Módulo cartão SD
- Display LCD 16x2 com módulo I2C
- Buzzer ativo
- Botão tactile switch
- Protoboard e jumpers

---

## Ligações

| Componente       | Pino Arduino         |
|------------------|----------------------|
| LCD / RTC / BME280 I2C | SDA 20, SCL 21 |
| RFID RC522       | SS 53, RST 5, MOSI 51, MISO 50, SCK 52 |
| Módulo SD        | CS 48, MOSI 51, MISO 50, SCK 52 |
| Sensor PIR       | Pino 6               |
| Buzzer           | Pino 7               |
| Botão tela       | Pino 8 -> GND        |
| HC-SR04 Trig     | Pino 9               |
| HC-SR04 Echo     | Pino 10              |
| GPS NEO-6M TX    | RX1 (Pino 19)        |

O botão usa pull-up interno do Arduino. Não é necessário resistor externo.

---

## Bibliotecas Necessárias

Instale via arduino-cli ou pelo gerenciador de bibliotecas do Arduino IDE 2:

- LiquidCrystal I2C
- RTClib
- MFRC522
- Adafruit BME280 Library
- Adafruit Unified Sensor
- TinyGPSPlus
- SD (incluída na IDE)

Para instalar todas de uma vez via terminal:

```bash
arduino-cli lib install "LiquidCrystal I2C"
arduino-cli lib install "RTClib"
arduino-cli lib install "MFRC522"
arduino-cli lib install "Adafruit BME280 Library"
arduino-cli lib install "Adafruit Unified Sensor"
arduino-cli lib install "TinyGPSPlus"
```

---

## Como Usar

1. Faça o clone do repositório
2. Instale as bibliotecas listadas acima
3. Abra o arquivo `Arduino_Mega_Monitoramento_Gado.ino` no Arduino IDE 2
4. Selecione a placa: Arduino Mega or Mega 2560
5. Selecione a porta correta em Ferramentas -> Porta
6. Carregue o sketch

---

## Telas do LCD

O display possui 3 telas navegáveis pelo botão físico. Cada tela atualiza os dados automaticamente a cada 300ms.

Tela 1 - Clima: temperatura, umidade e pressão atmosférica  
Tela 2 - Campo: detecção de movimento, distância medida e status do SD  
Tela 3 - Localização: status do GPS, satélites conectados e hora atual  

---

## Formato do Log CSV

Os dados são gravados no arquivo `DADOS.CSV` no cartão SD com o seguinte formato:

```
data,hora,temp_c,umidade_pct,pressao_hpa,distancia_cm,movimento,gps_ok,lat,lng,satelites,evento,rfid_uid
2026-05-22,08:30:00,24.5,68.0,1013.2,45.3,0,1,-15.123456,-47.654321,6,LEITURA,
2026-05-22,08:31:12,24.6,67.8,1013.1,45.1,1,1,-15.123456,-47.654321,6,LEITURA,
2026-05-22,08:32:05,24.6,67.9,1013.1,45.2,0,1,-15.123456,-47.654321,6,RFID,A1B2C3D4
```

---

## Estrutura do Projeto

```
AvanteAgro/
└── Arduino_Mega_Monitoramento_Gado/
    └── Arduino_Mega_Monitoramento_Gado.ino
```

---

## Licença

Este projeto está licenciado sob a licença MIT. Consulte o arquivo LICENSE para mais detalhes.

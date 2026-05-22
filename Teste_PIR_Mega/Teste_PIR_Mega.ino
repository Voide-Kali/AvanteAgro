/*
  Teste isolado do sensor PIR no Arduino Mega.

  Ligacoes:
  VCC -> 5V
  OUT -> pino 6
  GND -> GND
*/

const byte PIN_PIR = 6;
int ultimoEstado = -1;

void setup() {
  Serial.begin(9600);
  pinMode(PIN_PIR, INPUT);

  Serial.println("Teste PIR iniciado");
  Serial.println("Aguarde 60 segundos para estabilizar depois de ligar.");
}

void loop() {
  int estado = digitalRead(PIN_PIR);

  if (estado != ultimoEstado) {
    ultimoEstado = estado;
    Serial.print("PIR: ");
    Serial.println(estado == HIGH ? "MOVIMENTO" : "SEM MOVIMENTO");
  }

  delay(100);
}

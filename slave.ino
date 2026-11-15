#include <Arduino.h>
#include <SoftwareSerial.h>

/*================ PINOS ================*/
#define PIN_ANALOG   35
#define RELAY_OPEN   19
#define RELAY_CLOSE  21
#define FIM_ABRE     22
#define FIM_FECHA    23
#define SENSOR_FLOW  25
#define PH_PIN       32
#define BOIA_PIN     12

/*================ CONSTANTES ================*/
const int   SLAVE_ID = 0; // <--- ID do dispositivo definido como constante para facilitar alterações futuras
const float FLOW_CAL = 4.5;
const float PH_CAL   = 11.5;
const int   FLOW_INT = 1000;

/*================ LORA ================*/
SoftwareSerial loraSerial(4, 5);

/*================ VARIÁVEIS ================*/
volatile byte pulseCount = 0;

float flowRate = 0;
unsigned long totalMilliLitres = 0;
unsigned long lastFlow = 0;
unsigned long lastLoRa = 0;

int buffer_arr[10], avgval, temp;
float ph_act = 0;

bool gateOpen = false;

/*================ PROTÓTOTIPOS ================*/
void readSensors();
void updateFlow();
void printTelemetry();
void sendLoRa();
void receiveLoRa();

void abrirComporta();
void fecharComporta();
void pararMotor();
void processMessage(String msg);
void IRAM_ATTR pulseCounter();

/*================ SETUP ================*/
void setup() {

  Serial.begin(115200);
  loraSerial.begin(9600);

  pinMode(RELAY_OPEN, OUTPUT);
  pinMode(RELAY_CLOSE, OUTPUT);
  pinMode(FIM_ABRE, INPUT_PULLUP);
  pinMode(FIM_FECHA, INPUT_PULLUP);
  pinMode(SENSOR_FLOW, INPUT_PULLUP);
  pinMode(BOIA_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(SENSOR_FLOW),
                  pulseCounter,
                  FALLING);

  digitalWrite(RELAY_OPEN, LOW);
  digitalWrite(RELAY_CLOSE, LOW);

  Serial.print("\n=== ESCRAVO ");
  Serial.print(SLAVE_ID);
  Serial.println(" INICIALIZADO ===");
}

/*================ LOOP SLAVE ================*/
void loop() {
  readSensors();
  sendLoRa();
  receiveLoRa();
}

/*======================================================
  LEITURA DE SENSORES
======================================================*/
void readSensors() {

  int waterLevel = (digitalRead(BOIA_PIN) == LOW);

  avgval = 0;

  for (int i = 0; i < 10; i++) {
    buffer_arr[i] = analogRead(PH_PIN);
    delay(10);
  }

  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (buffer_arr[i] > buffer_arr[j]) {
        temp = buffer_arr[i];
        buffer_arr[i] = buffer_arr[j];
        buffer_arr[j] = temp;
      }
    }
  }

  for (int i = 2; i < 8; i++)
    avgval += buffer_arr[i];

  float volts = (float)avgval / 6.0 * (3.3 / 4095.0);
  ph_act = -5.70 * volts + PH_CAL;
  updateFlow();
  printTelemetry();
}

/*======================================================
  FLUXO
======================================================*/
void updateFlow() {

  unsigned long now = millis();

  if (now - lastFlow >= FLOW_INT) {

    // Proteção de seção crítica para ler e zerar a variável volátil de interrupção
    noInterrupts();
    float pulse = pulseCount;
    pulseCount = 0;
    interrupts();

    flowRate = ((1000.0 / (now - lastFlow))
                * pulse) / FLOW_CAL;

    lastFlow = now;

    totalMilliLitres += (flowRate / 60.0) * 1000.0;
  }
}

/*======================================================
  TELEMETRIA SERIAL
======================================================*/
void printTelemetry() {

  Serial.println("=== DADOS ===");
  Serial.print("ID do Slave: "); Serial.println(SLAVE_ID);
  Serial.print("pH: "); Serial.println(ph_act, 2);
  Serial.print("Fluxo: "); Serial.println(flowRate, 2);
  Serial.print("Total mL: "); Serial.println(totalMilliLitres);
  Serial.println("=============\n");
}

/*======================================================
  ENVIO LORA
======================================================*/
void sendLoRa() {

  if (millis() - lastLoRa < 4000)
    return;

  lastLoRa = millis();

  // JSON modificado incluindo a chave "id" no início
  String msg =
    String("{\"id\":") + String(SLAVE_ID) +
    ",\"p\":" + String(ph_act, 2) +
    ",\"f\":" + String(flowRate, 1) +
    ",\"l\":" + String(digitalRead(BOIA_PIN) == LOW) +
    ",\"g\":" + String(gateOpen ? 1 : 0) +
    "}";

  loraSerial.println(msg);

  Serial.print("LoRa: ");
  Serial.println(msg);
}

/*======================================================
  RECEBIMENTO LORA
======================================================*/
void receiveLoRa() {

  if (!loraSerial.available())
    return;

  String msg = loraSerial.readStringUntil('\n');
  msg.trim();

  processMessage(msg);
}

/*======================================================
  CONTROLE
======================================================*/
void processMessage(String msg) {

  Serial.println(msg);

  if (msg == "OPEN_GATE" && !gateOpen)
    abrirComporta();

  else if (msg == "CLOSE_GATE" && gateOpen)
    fecharComporta();
}

/*======================================================
  MOTOR
======================================================*/
void abrirComporta() {

  digitalWrite(RELAY_OPEN, HIGH);
  digitalWrite(RELAY_CLOSE, LOW);

  unsigned long t = millis();

  while (millis() - t < 5000) {
    if (digitalRead(FIM_ABRE) == LOW) break;
    delay(10);
  }

  pararMotor();
  gateOpen = true;
}

/*------------------------------------------------------*/

void fecharComporta() {

  digitalWrite(RELAY_OPEN, LOW);
  digitalWrite(RELAY_CLOSE, HIGH);

  unsigned long t = millis();

  while (millis() - t < 5000) {
    if (digitalRead(FIM_FECHA) == LOW) break;
    delay(10);
  }

  pararMotor();
  gateOpen = false;
}

/*------------------------------------------------------*/

void pararMotor() {
  digitalWrite(RELAY_OPEN, LOW);
  digitalWrite(RELAY_CLOSE, LOW);
}

/*======================================================
  INTERRUPÇÃO
======================================================*/
void IRAM_ATTR pulseCounter() {
  pulseCount++;
}

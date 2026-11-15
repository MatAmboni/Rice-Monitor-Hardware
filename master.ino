/*****************************************************************
 * SISTEMA DE MONITORAMENTO E CONTROLE DE QUALIDADE DA ÁGUA
 * Gateway ESP32 + LoRa + Firebase
 *
 * Funções:
 * - Recebe telemetria do nó remoto via LoRa
 * - Identifica dinamicamente o ID do Slave
 * - Atualiza dados no Firebase no nó correspondente
 * - Recebe comandos da nuvem para o Slave correto
 * - Controla abertura e fechamento da comporta
 *****************************************************************/

#include <WiFi.h>
#include <SoftwareSerial.h>
#include <Firebase_ESP_Client.h>
#include "secrets.h"

/*===============================================================
  CONFIGURAÇÕES WIFI
================================================================*/
/*===============================================================
  OBJETOS GLOBAIS
================================================================*/
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

SoftwareSerial loraSerial(4, 5);

/*===============================================================
  VARIÁVEIS DE CONTROLE
================================================================*/
bool signupOK = false;
unsigned long lastSync = 0;

/*===============================================================
  DADOS RECEBIDOS DO NÓ REMOTO
================================================================*/
int slaveId = 0;      
float ph = 0.0;
float flowRate = 0.0;

bool waterLevel   = false;
bool humidity     = false;
bool realGateState = false;

bool commandSent = false;

/*===============================================================
  PROTÓTIPOS
================================================================*/
void receiveTelemetry();
void printTelemetry();
void updateFirebase();
void checkCloudCommands();
String getValue(String data, String key);

/*===============================================================
  SETUP
================================================================*/
void setup() {

  Serial.begin(9600);
  loraSerial.begin(9600);

  Serial.println("\n=============================================");
  Serial.println("     SISTEMA DE CONTROLE HÍDRICO (MASTER)    ");
  Serial.println("=============================================");

  /*---------------- Conexão WiFi ----------------*/
  Serial.print("Conectando ao WiFi [" WIFI_SSID "]... ");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.waitForConnectResult() != WL_CONNECTED);

  Serial.println("OK");

  /*---------------- Firebase ----------------*/
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  Serial.print("Autenticando no Firebase... ");

  if (Firebase.signUp(&config, &auth, "", "")) {
    signupOK = true;
    Serial.println("PRONTO");
  }
  else {
    Serial.println("ERRO!");
    Serial.println(config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("=============================================");
  Serial.println("         MONITORAMENTO ATIVO (LORA)          ");
  Serial.println("=============================================\n");
}

/*===============================================================
  LOOP PRINCIPAL MESTRE
================================================================*/
void loop() {

  receiveTelemetry();

  if (Firebase.ready() &&
      signupOK &&
      millis() - lastSync >= 5000) {

    lastSync = millis();

    updateFirebase();
    checkCloudCommands();
  }
}

/*===============================================================
  RECEBIMENTO DE TELEMETRIA
================================================================*/
void receiveTelemetry() {

  if (!loraSerial.available())
    return;

  String input = loraSerial.readStringUntil('\n');
  input.trim();

  if (!(input.startsWith("{") && input.endsWith("}"))) {

    while (loraSerial.available())
      loraSerial.read();

    return;
  }

  slaveId       = getValue(input, "id").toInt(); 
  
  ph            = getValue(input, "p").toFloat();
  flowRate      = getValue(input, "f").toFloat();
  waterLevel    = getValue(input, "l").toInt();
  humidity      = getValue(input, "h").toInt();
  realGateState = getValue(input, "g").toInt();

  printTelemetry(); 
}

/*===============================================================
  EXIBE TELEMETRIA NO MONITOR SERIAL
================================================================*/
void printTelemetry() {

  Serial.print("[TELEMETRIA - SLAVE ");
  Serial.print(slaveId);
  Serial.print("] ");

  Serial.print("pH: ");
  Serial.print(ph, 2);

  Serial.print(" | Vazao: ");
  Serial.print(flowRate, 1);
  Serial.print(" L/min");

  Serial.print(" | Boia: ");
  Serial.print(waterLevel ? "CHEIO" : "VAZIO");

  Serial.print(" | Umid: ");
  Serial.print(humidity ? "ALTA" : "NORMAL");

  Serial.print(" | Comporta: ");
  Serial.println(realGateState ? "ABERTA" : "FECHADA");
}

/*===============================================================
  ENVIO DADOS FIREBASE
================================================================*/
void updateFirebase() {
  String path = "markers/marker_" + String(slaveId) + "/";
  Firebase.RTDB.setFloat(&fbdo, (path + "ph").c_str(), ph);
  Firebase.RTDB.setFloat(&fbdo, (path + "waterFlow").c_str(), flowRate);
  Firebase.RTDB.setBool(&fbdo, (path + "waterLevel").c_str(), waterLevel);
  Firebase.RTDB.setBool(&fbdo, (path + "humidity").c_str(), humidity);
}

/*===============================================================
  VERIFICA COMANDOS FIREBASE
================================================================*/
void checkCloudCommands() {

  bool desiredGateState = false;
  
  String path = "markers/marker_" + String(slaveId) + "/isGateOpen";

  if (!Firebase.RTDB.getBool(&fbdo, path.c_str()))
    return;

  desiredGateState = fbdo.boolData();

  if (desiredGateState == realGateState) {
    commandSent = false;
    return;
  }

  Serial.println("---------------------------------------------");
  Serial.print("[NUVEM] Alteracao no Slave ");
  Serial.print(slaveId);
  Serial.print("! Desejado: ");
  Serial.print(desiredGateState ? "ABERTA" : "FECHADA");

  Serial.print(" | Atual: ");
  Serial.println(realGateState ? "ABERTA" : "FECHADA");

  if (desiredGateState) {

    loraSerial.println("OPEN_GATE");
    Serial.println("[LORA] ---> Transmitindo: OPEN_GATE");

  } else {

    loraSerial.println("CLOSE_GATE");
    Serial.println("[LORA] ---> Transmitindo: CLOSE_GATE");
  }

  Serial.println("---------------------------------------------");

  commandSent = true;
}

/*===============================================================
  EXTRAÇÃO DE VALORES JSON SIMPLIFICADO
================================================================*/
String getValue(String data, String key) {

  int keyIndex = data.indexOf("\"" + key + "\":");

  if (keyIndex == -1)
    return "0";

  int start = data.indexOf(":", keyIndex) + 1;
  int end   = data.indexOf(",", start);

  if (end == -1)
    end = data.indexOf("}", start);

  return data.substring(start, end);
}

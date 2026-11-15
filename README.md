# Sistema de gerenciamento hídrico para lavouras de arroz

Sistema embarcado desenvolvido como Trabalho de Conclusão de Curso para monitorar variáveis hidráulicas em uma lavoura de arroz e controlar remotamente uma comporta. A solução utiliza microcontroladores ESP32, comunicação LoRa e Firebase Realtime Database para integrar o nó instalado no campo ao nó conectado à rede.

> **Status:** protótipo acadêmico em evolução. Os diretórios de testes são mantidos para rastreabilidade dos experimentos; o firmware recomendado para começar uma implantação é [`espSlave/espSlave.ino`](espSlave/espSlave.ino), em conjunto com [`lora_e32/v2/espWiFi/espWiFi.ino`](lora_e32/v2/espWiFi/espWiFi.ino).

## Visão geral

```text
┌──────────────────────┐       LoRa/E32        ┌──────────────────────┐
│ Nó de campo          │ ────────────────────▶ │ Nó gateway            │
│ ESP32 + sensores     │ ◀──────────────────── │ ESP32 + Wi-Fi         │
│ fluxo / nível / pH   │                       │ Firebase RTDB         │
│ acionamento da       │                       │ dashboard/controle    │
│ comporta             │                       └──────────────────────┘
└──────────────────────┘
```

### Funcionalidades

- Medição de vazão com sensor YF-S201/YF201.
- Leitura de nível de água por boia.
- Leitura e calibração de pH.
- Telemetria estruturada em mensagens JSON.
- Comunicação serial com módulos LoRa E32.
- Publicação de dados e recebimento de comandos via Firebase.
- Acionamento de comporta por relés e fins de curso.
- Experimentos independentes para handshake, frequência, pacotes e malha Wi-Fi.

## Estrutura do repositório

| Caminho | Finalidade |
| --- | --- |
| [`espSlave/`](espSlave/) | Nó de campo principal: sensores e telemetria LoRa. |
| [`lora_e32/v2/espWiFi/`](lora_e32/v2/espWiFi/) | Gateway Wi-Fi: sincroniza telemetria/controle com Firebase. |
| [`lora_e32/v2/espEscravo/`](lora_e32/v2/espEscravo/) | Variante experimental do nó escravo. |
| [`tcc.ino`](tcc.ino) | Protótipo integrado de sensores e comporta. |
| [`calibra_sensor_ph/`](calibra_sensor_ph/) | Leitura auxiliar para calibração do sensor de pH. |
| [`sensor_fluxo_YF201/`](sensor_fluxo_YF201/) | Teste isolado do sensor de vazão. |
| [`sensor_umidade/`](sensor_umidade/) | Teste isolado de entrada analógica. |
| [`projetoubiquos/`](projetoubiquos/) | Ensaios anteriores e provas de conceito. |

## Hardware de referência

- ESP32.
- Módulo LoRa E32 (UART em `RX=4` e `TX=5` nos exemplos).
- Sensor de vazão YF-S201/YF201.
- Boia de nível com entrada `INPUT_PULLUP`.
- Sensor de pH analógico.
- Dois relés, motor da comporta e fins de curso.

Os pinos podem variar entre protótipos. Confirme o esquema elétrico antes de energizar o circuito; não use os exemplos como substituto para validação elétrica e dimensionamento dos relés.

## Como começar

### 1. Preparar o ambiente

1. Instale a [Arduino IDE](https://www.arduino.cc/en/software).
2. Adicione o suporte para **ESP32 by Espressif Systems**.
3. Instale as bibliotecas usadas pelo sketch escolhido:
   - `SoftwareSerial` (ou a implementação compatível com sua placa).
   - `Firebase ESP Client` para os gateways.
   - Dependências do Firebase (`TokenHelper` e `RTDBHelper`) quando o sketch as referenciar.
   - `painlessMesh` e `ArduinoJson` somente para os experimentos em `projetoubiquos/espmesh/`.
4. Selecione a placa ESP32, a porta serial e o monitor serial em `115200` ou `9600`, conforme indicado no sketch.

### 2. Configurar credenciais

Credenciais não devem ser versionadas. Os sketches que usam Wi-Fi/Firebase contêm apenas valores fictícios (`YOUR_*`) para permitir a compilação sem expor segredos. Substitua esses valores somente na sua cópia local antes do upload:

- SSID e senha da rede Wi-Fi.
- API key do projeto Firebase.
- URL do Realtime Database.

Use um projeto Firebase de desenvolvimento, regras de acesso restritas e credenciais rotacionadas. Se alguma credencial deste repositório tiver sido utilizada fora de um ambiente local, revogue-a e gere outra.

### 3. Fazer o upload

1. Abra o arquivo `.ino` desejado na Arduino IDE.
2. Compile antes de conectar o hardware de potência.
3. Grave primeiro o firmware do gateway e valide a conexão Wi-Fi/Firebase pelo monitor serial.
4. Grave o firmware do nó de campo e valide os sensores sem acionar o motor.
5. Teste os comandos `OPEN_GATE` e `CLOSE_GATE` com a comporta desacoplada e os fins de curso funcionando.

## Protocolo de telemetria

O nó de campo envia mensagens delimitadas por nova linha. A implementação principal usa uma estrutura semelhante a:

```json
{
  "flowRate": 2.4,
  "waterLevel": 1
}
```

O protótipo integrado também utiliza `ph`, `totalMilliLitres`, `analogVoltage` e `gateState`. O gateway e o nó de campo precisam usar o mesmo formato e unidades.

## Segurança e operação

- Nunca publique senhas, API keys ou tokens no Git.
- Não acione motores sem fins de curso e proteção elétrica validados.
- Faça calibração do pH e do sensor de vazão antes de interpretar os dados.
- Trate os valores recebidos via LoRa como entrada não confiável.
- Use regras de segurança do Firebase adequadas ao ambiente; o firmware é um protótipo e não substitui autenticação/autorização de produção.
- Interrompa o ensaio se houver aquecimento, ruído elétrico ou comportamento inesperado no motor.

## Desenvolvimento

Este repositório não possui uma suíte automatizada de testes porque os sketches dependem de hardware físico. Antes de enviar uma alteração:

1. Compile o sketch afetado na Arduino IDE.
2. Verifique o monitor serial em condições normais e de desconexão.
3. Teste o caminho de falha de Wi-Fi/LoRa e os fins de curso.
4. Documente alterações de pinos, bibliotecas e protocolo no pull request.

## Autoria

Projeto acadêmico de **Matheus Costa Amboni**, desenvolvido na Universidade Federal de Santa Catarina (UFSC).

Consulte o trabalho acadêmico associado para a fundamentação, metodologia e resultados experimentais.

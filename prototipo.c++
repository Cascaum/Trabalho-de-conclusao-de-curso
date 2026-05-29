#include <Arduino.h>
#include <wiring_private.h>
#include <HardwareSerial.h>
#include <Serial.h>
#include <DFRobotDFPlayerMini.h>

// ======================================
// UART COM SEMÁFORO
// D2 RX
// D3 TX
// ======================================

arduino::UART semaforoSerial(
  digitalPinToPinName(2),
  digitalPinToPinName(3),
  NC,
  NC
);

// ======================================
// DFPLAYER
// RX -> TX1
// TX -> RX0
// ======================================

DFRobotDFPlayerMini player;

// ======================================

const byte PINO_BOTAO = 4;

char corAtual = '?';
int tempoAtual = 0;

String linhaRecebida = "";

bool pedidoPedestreEnviado =
  false;

unsigned long ultimoCliqueMs =
  0;

const unsigned long
INTERVALO_MINIMO_CLIQUE_MS =
  4000;

// timeout comunicação
unsigned long ultimoPacoteMs =
  0;

const unsigned long
TIMEOUT_MS = 5000;

// ======================================
// ÁUDIOS
//
// 001 -> Não atravesse
// 002 -> Não atravesse.
//        Faltam 10 segundos
// 003 -> Não atravesse.
//        Faltam 05 segundos
// 004 -> Sistema fora do ar
// 005 -> Pode atravessar
// ======================================

void tocarAudio(int numero)
{
  player.stop();

  delay(120);

  player.play(numero);

  delay(80);
}

// ======================================

void tocarEstadoAtual()
{
  // timeout
  if (millis() - ultimoPacoteMs >
      TIMEOUT_MS)
  {
    tocarAudio(4);
    return;
  }

  // ==================================
  // VERMELHO DOS CARROS
  // PEDESTRE PODE ATRAVESSAR
  // ==================================

  if (corAtual == 'R')
  {
    // faltam 5s ou menos
    if (tempoAtual <= 5)
    {
      tocarAudio(1);
      return;
    }

    tocarAudio(5);
    return;
  }

  // ==================================
  // AMARELO
  // ==================================

  if (corAtual == 'Y')
  {
    tocarAudio(1);
    return;
  }

  // ==================================
  // VERDE DOS CARROS
  // ==================================

  if (corAtual == 'G')
  {
    // 10 segundos
    if (tempoAtual == 10)
    {
      tocarAudio(2);
      return;
    }

    // 5 segundos
    if (tempoAtual == 5)
    {
      tocarAudio(3);
      return;
    }

    // 9-6 segundos
    if (tempoAtual >= 6 &&
        tempoAtual <= 9)
    {
      tocarAudio(1);
      return;
    }

    // 4-1 segundos
    if (tempoAtual >= 1 &&
        tempoAtual <= 4)
    {
      tocarAudio(1);
      return;
    }

    // 30-11 segundos
    tocarAudio(2);
  }
}

// ======================================

void enviarPedidoPedestre()
{
  semaforoSerial.write('P');
  semaforoSerial.write('\n');

  pedidoPedestreEnviado =
    true;
}

// ======================================

void processarLinha(
  const String& linha)
{
  int separador =
    linha.indexOf(',');

  if (separador <= 0)
    return;

  char novaCor =
    linha.charAt(0);

  int novoTempo =
    linha.substring(
      separador + 1).toInt();

  if (novaCor != 'G' &&
      novaCor != 'Y' &&
      novaCor != 'R')
  {
    return;
  }

  corAtual = novaCor;
  tempoAtual = novoTempo;

  ultimoPacoteMs = millis();

  if (corAtual == 'R')
  {
    pedidoPedestreEnviado =
      false;
  }
}

// ======================================

void lerSemaforo()
{
  while (semaforoSerial.available())
  {
    char c =
      (char)semaforoSerial.read();

    if (c == '\r')
      continue;

    if (c == '\n')
    {
      if (linhaRecebida.length() > 0)
      {
        processarLinha(
          linhaRecebida);

        linhaRecebida = "";
      }
    }
    else
    {
      linhaRecebida += c;
    }
  }
}

// ======================================

void tratarBotao()
{
  static bool ultimoEstadoBotao =
    HIGH;

  bool estadoAtualBotao =
    digitalRead(PINO_BOTAO);

  digitalWrite(
    LED_BUILTIN,
    estadoAtualBotao == LOW
      ? HIGH : LOW
  );

  if (ultimoEstadoBotao == HIGH &&
      estadoAtualBotao == LOW)
  {
    unsigned long agora =
      millis();

    if (agora - ultimoCliqueMs >=
        INTERVALO_MINIMO_CLIQUE_MS)
    {
      ultimoCliqueMs = agora;

      tocarEstadoAtual();

      if ((corAtual == 'G' ||
           corAtual == 'Y') &&
          !pedidoPedestreEnviado)
      {
        enviarPedidoPedestre();
      }
    }
  }

  ultimoEstadoBotao =
    estadoAtualBotao;
}

// ======================================

void setup()
{
  pinMode(
    PINO_BOTAO,
    INPUT_PULLUP);

  pinMode(
    LED_BUILTIN,
    OUTPUT);

  digitalWrite(
    LED_BUILTIN,
    LOW);

  semaforoSerial.begin(9600);

  Serial1.begin(9600);

  delay(1000);

  player.begin(Serial1);

  player.volume(25);
}

// ======================================

void loop()
{
  lerSemaforo();

  tratarBotao();
}

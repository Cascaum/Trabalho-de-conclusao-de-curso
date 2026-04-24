#include <Arduino.h>
#include <wiring_private.h>
#include <HardwareSerial.h>
#include <Serial.h>
#include <DFRobotDFPlayerMini.h>

arduino::UART semaforoSerial(digitalPinToPinName(2), digitalPinToPinName(3), NC, NC);

DFRobotDFPlayerMini player;

const byte PINO_BOTAO = 4;

char corAtual = '?';
int tempoAtual = 0;

unsigned long ultimoCliqueMs = 0;
const unsigned long INTERVALO_MINIMO_CLIQUE_MS = 4000;

bool pedidoPedestreEnviado = false;

String linhaRecebida = "";

int arquivoContagem(int segundos)
{
  if (segundos < 1 || segundos > 10)
    return 0;

  return 13 - segundos;
}

// Arquivos no cartão SD do DFPlayer:
// 0001.mp3 -> "Pode atravessar."
// 0002.mp3 -> "Não inicie a travessia. Aguarde o próximo ciclo."
// 0003.mp3 -> "Restam 10 segundos."
// 0004.mp3 -> "Restam 9 segundos."
// 0005.mp3 -> "Restam 8 segundos."
// 0006.mp3 -> "Restam 7 segundos."
// 0007.mp3 -> "Restam 6 segundos."
// 0008.mp3 -> "Restam 5 segundos."
// 0009.mp3 -> "Restam 4 segundos."
// 0010.mp3 -> "Restam 3 segundos."
// 0011.mp3 -> "Restam 2 segundos."
// 0012.mp3 -> "Resta 1 segundo."
// 0013.mp3 -> "Aguarde para atravessar."

void tocarEstadoAtual()
{
  if (corAtual == 'R')
  {
    if (tempoAtual > 10)
    {
      player.play(1);
    }
    else if (tempoAtual >= 8)
    {
      int arquivo = arquivoContagem(tempoAtual);

      if (arquivo > 0)
        player.play(arquivo);
      else
        player.play(1);
    }
    else
    {
      player.play(2);
    }
  }
  else
  {
    player.play(13);
  }
}

void enviarPedidoPedestre()
{
  semaforoSerial.write('P');
  semaforoSerial.write('\n');
  pedidoPedestreEnviado = true;
}

void processarLinha(const String& linha)
{
  int separador = linha.indexOf(',');

  if (separador <= 0)
    return;

  char novaCor = linha.charAt(0);
  int novoTempo = linha.substring(separador + 1).toInt();

  if (novaCor != 'G' && novaCor != 'Y' && novaCor != 'R')
    return;

  corAtual = novaCor;
  tempoAtual = novoTempo;

  if (corAtual == 'R')
  {
    pedidoPedestreEnviado = false;
  }
}

void lerSemaforo()
{
  while (semaforoSerial.available() > 0)
  {
    char c = (char)semaforoSerial.read();

    if (c == '\r')
      continue;

    if (c == '\n')
    {
      if (linhaRecebida.length() > 0)
      {
        processarLinha(linhaRecebida);
        linhaRecebida = "";
      }
    }
    else
    {
      linhaRecebida += c;
    }
  }
}

void tratarBotao()
{
  static bool ultimoEstadoBotao = HIGH;

  bool estadoAtualBotao = digitalRead(PINO_BOTAO);

  // Acende o LED da placa enquanto o botão estiver pressionado
  digitalWrite(LED_BUILTIN, estadoAtualBotao == LOW ? HIGH : LOW);

  // Detecta o momento em que o botão foi pressionado
  if (ultimoEstadoBotao == HIGH && estadoAtualBotao == LOW)
  {
    unsigned long agora = millis();

    if (agora - ultimoCliqueMs >= INTERVALO_MINIMO_CLIQUE_MS)
    {
      ultimoCliqueMs = agora;

      tocarEstadoAtual();

      if ((corAtual == 'G' || corAtual == 'Y') && !pedidoPedestreEnviado)
      {
        enviarPedidoPedestre();
      }
    }
  }

  ultimoEstadoBotao = estadoAtualBotao;
}

void setup()
{
  pinMode(PINO_BOTAO, INPUT_PULLUP);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  semaforoSerial.begin(9600);

  Serial1.begin(9600);
  delay(800);

  player.begin(Serial1);
  player.volume(25);
}

void loop()
{
  lerSemaforo();
  tratarBotao();
}
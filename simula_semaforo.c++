// ======================================
// CONTROLADOR DO SEMÁFORO
// ======================================

enum EstadoSemaforo
{
  ESTADO_VERDE,
  ESTADO_AMARELO,
  ESTADO_VERMELHO
};

const byte PINO_VERDE    = 2;
const byte PINO_AMARELO  = 3;
const byte PINO_VERMELHO = 4;

// ======================================
// TEMPOS
// ======================================

const int TEMPO_VERDE_PADRAO   = 30;
const int TEMPO_AMARELO        = 5;
const int TEMPO_VERMELHO       = 30;

const int TEMPO_VERDE_REDUZIDO = 10;

const unsigned long
INTERVALO_ENVIO_MS = 250;

// ======================================

EstadoSemaforo estadoAtual =
  ESTADO_VERDE;

unsigned long inicioEstadoMs  = 0;
unsigned long duracaoEstadoMs = 0;
unsigned long ultimoEnvioMs   = 0;

bool pedidoPedestrePendente = false;

// ======================================

char estadoParaChar(
  EstadoSemaforo estado)
{
  switch (estado)
  {
    case ESTADO_VERDE:
      return 'G';

    case ESTADO_AMARELO:
      return 'Y';

    case ESTADO_VERMELHO:
      return 'R';
  }

  return '?';
}

// ======================================

void aplicarSaidas()
{
  digitalWrite(
    PINO_VERDE,
    estadoAtual == ESTADO_VERDE
      ? HIGH : LOW
  );

  digitalWrite(
    PINO_AMARELO,
    estadoAtual == ESTADO_AMARELO
      ? HIGH : LOW
  );

  digitalWrite(
    PINO_VERMELHO,
    estadoAtual == ESTADO_VERMELHO
      ? HIGH : LOW
  );
}

// ======================================

int tempoRestanteSegundos()
{
  unsigned long agora =
    millis();

  unsigned long decorrido =
    agora - inicioEstadoMs;

  if (decorrido >= duracaoEstadoMs)
    return 0;

  unsigned long restanteMs =
    duracaoEstadoMs - decorrido;

  return
    (int)((restanteMs + 999UL)
    / 1000UL);
}

// ======================================

void enviarEstadoAtual()
{
  Serial.print(
    estadoParaChar(estadoAtual));

  Serial.print(',');

  Serial.println(
    tempoRestanteSegundos());
}

// ======================================

void iniciarEstado(
  EstadoSemaforo novoEstado)
{
  estadoAtual = novoEstado;

  inicioEstadoMs = millis();

  switch (estadoAtual)
  {
    case ESTADO_VERDE:

      duracaoEstadoMs =
        (unsigned long)
        TEMPO_VERDE_PADRAO *
        1000UL;

      break;

    case ESTADO_AMARELO:

      duracaoEstadoMs =
        (unsigned long)
        TEMPO_AMARELO *
        1000UL;

      break;

    case ESTADO_VERMELHO:

      duracaoEstadoMs =
        (unsigned long)
        TEMPO_VERMELHO *
        1000UL;

      pedidoPedestrePendente =
        false;

      break;
  }

  aplicarSaidas();

  enviarEstadoAtual();
}

// ======================================

void processarComandosRecebidos()
{
  while (Serial.available() > 0)
  {
    char comando =
      Serial.read();

    if (comando == 'P')
    {
      if (estadoAtual ==
          ESTADO_VERDE)
      {
        pedidoPedestrePendente =
          true;
      }
    }
  }
}

// ======================================
// REDUZ VERDE 30 -> 15
// SOMENTE ENTRE 30 E 16
// ======================================

void ajustarVerdePorPedido()
{
  if (!pedidoPedestrePendente)
    return;

  if (estadoAtual !=
      ESTADO_VERDE)
  {
    return;
  }

  int restante =
    tempoRestanteSegundos();

  // apenas entre 30 e 16
  if (restante >= 16)
  {
    unsigned long agora =
      millis();

    unsigned long decorrido =
      agora - inicioEstadoMs;

    duracaoEstadoMs =
      decorrido +
      ((unsigned long)
      TEMPO_VERDE_REDUZIDO
      * 1000UL);

    pedidoPedestrePendente =
      false;
  }
}

// ======================================

void atualizarEstado()
{
  unsigned long agora =
    millis();

  unsigned long decorrido =
    agora - inicioEstadoMs;

  if (decorrido <
      duracaoEstadoMs)
  {
    return;
  }

  switch (estadoAtual)
  {
    case ESTADO_VERDE:

      iniciarEstado(
        ESTADO_AMARELO);

      break;

    case ESTADO_AMARELO:

      iniciarEstado(
        ESTADO_VERMELHO);

      break;

    case ESTADO_VERMELHO:

      iniciarEstado(
        ESTADO_VERDE);

      break;
  }
}

// ======================================

void enviarPeriodicamente()
{
  unsigned long agora =
    millis();

  if (agora - ultimoEnvioMs >=
      INTERVALO_ENVIO_MS)
  {
    ultimoEnvioMs = agora;

    enviarEstadoAtual();
  }
}

// ======================================

void setup()
{
  pinMode(PINO_VERDE, OUTPUT);
  pinMode(PINO_AMARELO, OUTPUT);
  pinMode(PINO_VERMELHO, OUTPUT);

  Serial.begin(9600);

  iniciarEstado(
    ESTADO_VERDE);
}

// ======================================

void loop()
{
  processarComandosRecebidos();

  ajustarVerdePorPedido();

  atualizarEstado();

  enviarPeriodicamente();
}

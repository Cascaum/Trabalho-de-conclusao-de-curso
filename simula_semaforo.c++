// Controlador do semáforo
// LEDs:
// verde    -> D2
// amarelo  -> D3
// vermelho -> D4
//
// Comunicação com o Nano 33:
// usa a UART padrão (pinos D0 RX / D1 TX)
//
// Protocolo enviado:
// G,15
// Y,3
// R,12
//
// Comando recebido:
// P  -> pedido de travessia

enum EstadoSemaforo
{
  ESTADO_VERDE,
  ESTADO_AMARELO,
  ESTADO_VERMELHO
};

const byte PINO_VERDE = 2;
const byte PINO_AMARELO = 3;
const byte PINO_VERMELHO = 4;

const int TEMPO_VERDE_PADRAO = 15;
const int TEMPO_AMARELO = 3;
const int TEMPO_VERMELHO = 15;

// Após um pedido de pedestre, o verde dos carros é encurtado,
// mas ainda garante alguns segundos antes de mudar para amarelo.
const int TEMPO_MINIMO_VERDE_APOS_PEDIDO = 5;

// Envia o estado algumas vezes por segundo.
const unsigned long INTERVALO_ENVIO_MS = 250;

EstadoSemaforo estadoAtual = ESTADO_VERDE;

unsigned long inicioEstadoMs = 0;
unsigned long duracaoEstadoMs = 0;
unsigned long ultimoEnvioMs = 0;

bool pedidoPedestrePendente = false;

char estadoParaChar(EstadoSemaforo estado)
{
  switch (estado)
  {
    case ESTADO_VERDE: return 'G';
    case ESTADO_AMARELO: return 'Y';
    case ESTADO_VERMELHO: return 'R';
    default: return '?';
  }
}

void aplicarSaidas(EstadoSemaforo estado)
{
  digitalWrite(PINO_VERDE, estado == ESTADO_VERDE ? HIGH : LOW);
  digitalWrite(PINO_AMARELO, estado == ESTADO_AMARELO ? HIGH : LOW);
  digitalWrite(PINO_VERMELHO, estado == ESTADO_VERMELHO ? HIGH : LOW);
}

void iniciarEstado(EstadoSemaforo novoEstado)
{
  estadoAtual = novoEstado;
  inicioEstadoMs = millis();

  switch (estadoAtual)
  {
    case ESTADO_VERDE:
      duracaoEstadoMs = (unsigned long)TEMPO_VERDE_PADRAO * 1000UL;
      break;

    case ESTADO_AMARELO:
      duracaoEstadoMs = (unsigned long)TEMPO_AMARELO * 1000UL;
      break;

    case ESTADO_VERMELHO:
      duracaoEstadoMs = (unsigned long)TEMPO_VERMELHO * 1000UL;
      pedidoPedestrePendente = false; // pedido atendido ao entrar no vermelho
      break;
  }

  aplicarSaidas(estadoAtual);
  enviarEstadoAtual();
}

int tempoRestanteSegundos()
{
  unsigned long agora = millis();
  unsigned long decorrido = agora - inicioEstadoMs;

  if (decorrido >= duracaoEstadoMs)
    return 0;

  unsigned long restanteMs = duracaoEstadoMs - decorrido;

  // Arredonda para cima para manter contagem humana.
  return (int)((restanteMs + 999UL) / 1000UL);
}

void enviarEstadoAtual()
{
  Serial.print(estadoParaChar(estadoAtual));
  Serial.print(',');
  Serial.println(tempoRestanteSegundos());
}

void processarComandosRecebidos()
{
  while (Serial.available() > 0)
  {
    char comando = Serial.read();

    if (comando == 'P')
    {
      // Só faz sentido guardar pedido se ainda não está
      // no vermelho dos carros.
      if (estadoAtual != ESTADO_VERMELHO)
      {
        pedidoPedestrePendente = true;
      }
    }
  }
}

void ajustarVerdePorPedido()
{
  if (estadoAtual != ESTADO_VERDE || !pedidoPedestrePendente)
    return;

  unsigned long agora = millis();
  unsigned long decorrido = agora - inicioEstadoMs;
  unsigned long restanteAtualMs = (decorrido >= duracaoEstadoMs) ? 0 : (duracaoEstadoMs - decorrido);

  unsigned long restanteMinimoMs = (unsigned long)TEMPO_MINIMO_VERDE_APOS_PEDIDO * 1000UL;

  // Se ainda faltam mais segundos do que o mínimo desejado,
  // encurta o verde para sobrar somente esse mínimo.
  if (restanteAtualMs > restanteMinimoMs)
  {
    duracaoEstadoMs = decorrido + restanteMinimoMs;
  }
}

void atualizarEstado()
{
  unsigned long agora = millis();
  unsigned long decorrido = agora - inicioEstadoMs;

  if (decorrido < duracaoEstadoMs)
    return;

  switch (estadoAtual)
  {
    case ESTADO_VERDE:
      iniciarEstado(ESTADO_AMARELO);
      break;

    case ESTADO_AMARELO:
      iniciarEstado(ESTADO_VERMELHO);
      break;

    case ESTADO_VERMELHO:
      iniciarEstado(ESTADO_VERDE);
      break;
  }
}

void enviarPeriodicamente()
{
  unsigned long agora = millis();

  if (agora - ultimoEnvioMs >= INTERVALO_ENVIO_MS)
  {
    ultimoEnvioMs = agora;
    enviarEstadoAtual();
  }
}

void setup()
{
  pinMode(PINO_VERDE, OUTPUT);
  pinMode(PINO_AMARELO, OUTPUT);
  pinMode(PINO_VERMELHO, OUTPUT);

  Serial.begin(9600);

  iniciarEstado(ESTADO_VERDE);
}

void loop()
{
  processarComandosRecebidos();
  ajustarVerdePorPedido();
  atualizarEstado();
  enviarPeriodicamente();
}
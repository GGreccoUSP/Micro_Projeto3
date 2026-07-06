/* ==========================================================================
 * SEL0433 - Aplicação de Microprocessadores
 * Projeto 3 - Parte 1: Controle PWM de LED RGB (biblioteca LEDC)
 * ==========================================================================
 *
 * DESCRIÇÃO
 *   Controla um LED RGB de catodo comum usando 3 canais PWM independentes
 *   (LEDC) da ESP32, um por cor. O duty cycle de cada canal varia
 *   continuamente entre 0% e 100%, em loop, usando incrementos DIFERENTES
 *   e independentes para cada cor (requisito do enunciado):
 *
 *       Verde     -> passo base:     5%   (5, 10, 15 ... 100, volta a 0)
 *       Azul      -> dobro do verde: 10%
 *       Vermelho  -> triplo do verde:15%
 *
 *   Os incrementos são #define no topo do arquivo, então basta alterar
 *   STEP_GREEN para reconfigurar as três taxas de uma só vez (mecanismo
 *   flexível pedido no enunciado).
 *
 *   A cada atualização, o programa envia pela UART (115200 baud) os
 *   incrementos configurados e o duty cycle atual de cada canal.
 *
 * HARDWARE (ligações sugeridas)
 *   LED RGB catodo comum:
 *     Terminal R  -> resistor 220 ohm -> GPIO 25
 *     Terminal G  -> resistor 220 ohm -> GPIO 26
 *     Terminal B  -> resistor 220 ohm -> GPIO 27
 *     Catodo comum (COM) -> GND
 *
 * DECISÕES DE ENGENHARIA
 *   - Resolução de 8 bits (0-255) atende ao mínimo exigido (8 bits) e é
 *     suficiente para perceber a variação de brilho a olho nu, minimizando
 *     uso de memória/tempo de cálculo frente a resoluções maiores (ex. 12
 *     ou 16 bits), o que também favorece o consumo energético do MCU ao
 *     manter os registradores do LEDC menores e o tempo de CPU por
 *     atualização reduzido.
 *   - Frequência de 5 kHz evita flicker perceptível e fica bem acima da
 *     faixa audível de eventuais acoplamentos mecânicos, sem exigir clock
 *     do LEDC excessivamente alto.
 *   - Canais LEDC (PWM por hardware) foram usados em vez de PWM via
 *     software (delay/bit-bang) pois a ESP32 gera o sinal de forma
 *     autônoma em hardware, liberando a CPU para outras tarefas (ex.
 *     comunicação serial, Bluetooth) - importante para escalabilidade do
 *     projeto (Parte 1 desafio com BLE).
 * ==========================================================================
 */

#include <Arduino.h>

// ---------------------------------------------------------------------
// Configurações gerais
// ---------------------------------------------------------------------
#define BAUD_RATE        115200

// Pinos GPIO usados para cada cor (ajuste conforme sua montagem)
#define PIN_LED_RED      25
#define PIN_LED_GREEN    26
#define PIN_LED_BLUE     27

// Canais PWM (LEDC) da ESP32 - existem 16 canais (0 a 15) disponíveis
#define CH_RED           0
#define CH_GREEN         1
#define CH_BLUE          2

// Parâmetros do PWM exigidos pelo enunciado
#define PWM_FREQ_HZ      5000   // 5 kHz
#define PWM_RESOLUTION   8      // 8 bits -> duty de 0 a 255

// Incrementos independentes por cor, em pontos percentuais (%)
// Altere apenas STEP_GREEN para escalar as três taxas proporcionalmente.
#define STEP_GREEN       5
#define STEP_BLUE        (STEP_GREEN * 2)   // dobro do verde
#define STEP_RED         (STEP_GREEN * 3)   // triplo do verde

// Intervalo entre atualizações do duty cycle (ms)
#define UPDATE_PERIOD_MS 200

// ---------------------------------------------------------------------
// Variáveis de estado (duty cycle atual de cada canal, em %)
// ---------------------------------------------------------------------
int dutyPercentRed   = 0;
int dutyPercentGreen = 0;
int dutyPercentBlue  = 0;

// ---------------------------------------------------------------------
// Converte um percentual (0-100) para o valor de contagem do LEDC,
// de acordo com a resolução configurada (ex.: 8 bits -> 0-255)
// ---------------------------------------------------------------------
uint32_t percentToDuty(int percent) {
  uint32_t maxDuty = (1UL << PWM_RESOLUTION) - 1; // 255 para 8 bits
  percent = constrain(percent, 0, 100);
  return (uint32_t)(((uint32_t)percent * maxDuty) / 100UL);
}

// ---------------------------------------------------------------------
// Aplica o valor atual de duty cycle (em %) de cada cor nos respectivos
// canais LEDC
// ---------------------------------------------------------------------
void aplicarDutyCycles() {
  ledcWrite(CH_RED,   percentToDuty(dutyPercentRed));
  ledcWrite(CH_GREEN, percentToDuty(dutyPercentGreen));
  ledcWrite(CH_BLUE,  percentToDuty(dutyPercentBlue));
}

// ---------------------------------------------------------------------
// Avança o duty cycle de cada cor pelo seu próprio incremento,
// reiniciando em 0% ao ultrapassar 100% (loop contínuo)
// ---------------------------------------------------------------------
void avancarDutyCycles() {
  dutyPercentRed   += STEP_RED;
  dutyPercentGreen += STEP_GREEN;
  dutyPercentBlue  += STEP_BLUE;

  if (dutyPercentRed   > 100) dutyPercentRed   = 0;
  if (dutyPercentGreen > 100) dutyPercentGreen = 0;
  if (dutyPercentBlue  > 100) dutyPercentBlue  = 0;
}

// ---------------------------------------------------------------------
// Envia pela UART o incremento e o duty cycle atual de cada canal
// ---------------------------------------------------------------------
void logSerial() {
  Serial.printf(
    "R: inc=+%d%%  duty=%3d%% (%3lu/255) | "
    "G: inc=+%d%%  duty=%3d%% (%3lu/255) | "
    "B: inc=+%d%%  duty=%3d%% (%3lu/255)\n",
    STEP_RED,   dutyPercentRed,   (unsigned long)percentToDuty(dutyPercentRed),
    STEP_GREEN, dutyPercentGreen, (unsigned long)percentToDuty(dutyPercentGreen),
    STEP_BLUE,  dutyPercentBlue,  (unsigned long)percentToDuty(dutyPercentBlue)
  );
}

void setup() {
  Serial.begin(BAUD_RATE);
  delay(300);
  Serial.println();
  Serial.println(F("================================================="));
  Serial.println(F(" SEL0433 - Projeto 3 - Parte 1: PWM RGB (LEDC)"));
  Serial.println(F("================================================="));
  Serial.printf("Frequencia PWM : %d Hz\n", PWM_FREQ_HZ);
  Serial.printf("Resolucao      : %d bits (0-%d)\n", PWM_RESOLUTION, (1 << PWM_RESOLUTION) - 1);
  Serial.printf("Incrementos    : R=+%d%% | G=+%d%% | B=+%d%%\n", STEP_RED, STEP_GREEN, STEP_BLUE);
  Serial.println(F("-------------------------------------------------"));

  // Configura cada canal LEDC (frequencia + resolucao) e associa ao pino
  ledcSetup(CH_RED,   PWM_FREQ_HZ, PWM_RESOLUTION);
  ledcSetup(CH_GREEN, PWM_FREQ_HZ, PWM_RESOLUTION);
  ledcSetup(CH_BLUE,  PWM_FREQ_HZ, PWM_RESOLUTION);

  ledcAttachPin(PIN_LED_RED,   CH_RED);
  ledcAttachPin(PIN_LED_GREEN, CH_GREEN);
  ledcAttachPin(PIN_LED_BLUE,  CH_BLUE);

  // Observação: em versões mais novas do core arduino-esp32 (>= 3.x),
  // ledcSetup/ledcAttachPin foram substituídas por uma única chamada:
  //   ledcAttach(pin, freq, resolution);
  // Caso o Wokwi/Arduino IDE reclame que ledcSetup não existe, troque as
  // 6 linhas acima por:
  //   ledcAttach(PIN_LED_RED,   PWM_FREQ_HZ, PWM_RESOLUTION);
  //   ledcAttach(PIN_LED_GREEN, PWM_FREQ_HZ, PWM_RESOLUTION);
  //   ledcAttach(PIN_LED_BLUE,  PWM_FREQ_HZ, PWM_RESOLUTION);
  // e troque ledcWrite(CH_x, duty) por ledcWrite(PIN_LED_x, duty) no
  // restante do código.

  aplicarDutyCycles();
}

void loop() {
  avancarDutyCycles();
  aplicarDutyCycles();
  logSerial();
  delay(UPDATE_PERIOD_MS);
}

// ==========================================================================
// SEL0433 - Aplicação de Microprocessadores
// Projeto 3 - Parte 1: Controle PWM de LED RGB (biblioteca LEDC)
// ==========================================================================


#include <Arduino.h>

#define BAUD_RATE        115200

// Pinos GPIO usados para cada cor
#define PIN_LED_RED      25
#define PIN_LED_GREEN    26
#define PIN_LED_BLUE     27

// Canais PWM (LEDC) da ESP32
#define CH_RED           0
#define CH_GREEN         1
#define CH_BLUE          2

// Parâmetros do PWM exigidos
#define PWM_FREQ_HZ      5000   // 5 kHz
#define PWM_RESOLUTION   8      // 8 bits -> duty de 0 a 255

// Incrementos independentes por cor, em pontos percentuais (%)
#define STEP_GREEN       5
#define STEP_BLUE        (STEP_GREEN * 2)   // dobro do verde
#define STEP_RED         (STEP_GREEN * 3)   // triplo do verde

// Intervalo entre atualizações do duty cycle (ms)
#define UPDATE_PERIOD_MS 200

   
// Variáveis de estado (duty cycle atual de cada canal, em %)
int dutyPercentRed   = 0;
int dutyPercentGreen = 0;
int dutyPercentBlue  = 0;

// Converte um percentual (0-100) para o valor de contagem do LEDC
uint32_t percentToDuty(int percent) {
  uint32_t maxDuty = (1UL << PWM_RESOLUTION) - 1; // 255 para 8 bits
  percent = constrain(percent, 0, 100);
  return (uint32_t)(((uint32_t)percent * maxDuty) / 100UL);
}

// Aplica o valor atual de duty cycle (em %) de cada cor nos respectivos
// canais LEDC
void aplicarDutyCycles() {
  ledcWrite(CH_RED,   percentToDuty(dutyPercentRed));
  ledcWrite(CH_GREEN, percentToDuty(dutyPercentGreen));
  ledcWrite(CH_BLUE,  percentToDuty(dutyPercentBlue));
}

// Avança o duty cycle de cada cor pelo seu próprio incremento,
// reiniciando em 0% ao ultrapassar 100% (loop contínuo)
void avancarDutyCycles() {
  dutyPercentRed   += STEP_RED;
  dutyPercentGreen += STEP_GREEN;
  dutyPercentBlue  += STEP_BLUE;

  if (dutyPercentRed   > 100) dutyPercentRed   = 0;
  if (dutyPercentGreen > 100) dutyPercentGreen = 0;
  if (dutyPercentBlue  > 100) dutyPercentBlue  = 0;
}

// Envia pela UART o incremento e o duty cycle atual de cada canal
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

  aplicarDutyCycles();
}

void loop() {
  avancarDutyCycles();
  aplicarDutyCycles();
  logSerial();
  delay(UPDATE_PERIOD_MS);
}

/* ==========================================================================
 * SEL0433 - Aplicação de Microprocessadores
 * Projeto 3 - Parte 2 / Exercício 1
 * Controle PWM de servomotor via potenciômetro (biblioteca ESP32Servo)
 * ==========================================================================
 *
 * DESCRIÇÃO
 *   O sinal analógico do potenciômetro é lido pelo ADC da ESP32 e convertido
 *   em duty cycle (0-100%) / ângulo (0-180°), posicionando o servomotor de
 *   forma manual e contínua conforme o usuário gira o potenciômetro.
 *
 * HARDWARE
 *   Potenciômetro:
 *     Pino VCC -> 3V3
 *     Pino GND -> GND
 *     Pino SIG (cursor) -> GPIO 34 (ADC1_CH6, somente entrada)
 *   Servomotor:
 *     Fio de sinal (laranja/amarelo) -> GPIO 18
 *     V+  -> 5V
 *     GND -> GND
 *
 * DECISÕES DE ENGENHARIA
 *   - GPIO 34 foi escolhido por ser um pino ADC1 "input only", adequado
 *     para leitura analógica sem interferir em periféricos de saída.
 *   - A biblioteca "ESP32Servo" foi usada (em vez de gerar o PWM na mão)
 *     porque já encapsula a geração de pulso de 50 Hz com largura entre
 *     500-2400 us esperada pela maioria dos servos, e usa internamente o
 *     mesmo hardware LEDC, evitando bloquear a CPU com delays.
 *   - allocateTimer(0) reserva um timer do LEDC dedicado ao servo, evitando
 *     conflito de frequência com os canais PWM usados na Parte 1 (5 kHz),
 *     já que servos exigem 50 Hz.
 * ==========================================================================
 */

#include <ESP32Servo.h>

// ---------- Configurações de hardware ----------
#define PIN_SERVO        18
#define PIN_POTENCIOMETRO 34
#define BAUD_RATE        115200

// ---------- Parâmetros do servo ----------
#define SERVO_PULSO_MIN_US  500   // pulso minimo (0 graus)
#define SERVO_PULSO_MAX_US  2400  // pulso maximo (180 graus)
#define SERVO_ANGULO_MIN    0
#define SERVO_ANGULO_MAX    180

// ---------- Parâmetros do ADC ----------
#define ADC_RESOLUCAO_BITS  12
#define ADC_VALOR_MAX       4095  // 2^12 - 1

#define UPDATE_PERIOD_MS    100

Servo meuServo;
int ultimoAngulo = -1; // forca a primeira impressao/escrita

void setup() {
  Serial.begin(BAUD_RATE);
  delay(300);
  Serial.println();
  Serial.println(F("======================================================"));
  Serial.println(F(" Projeto 3 - Parte 2 / Exercicio 1: Servo + Potenciometro"));
  Serial.println(F("======================================================"));

  // Configuracao recomendada pela biblioteca ESP32Servo: aloca um timer
  // do LEDC dedicado exclusivamente ao(s) servo(s) do projeto.
  ESP32PWM::allocateTimer(0);

  meuServo.setPeriodHertz(50); // servos analogicos padrao trabalham a 50 Hz
  meuServo.attach(PIN_SERVO, SERVO_PULSO_MIN_US, SERVO_PULSO_MAX_US);

  analogReadResolution(ADC_RESOLUCAO_BITS); // garante 12 bits de resolucao no ADC

  Serial.printf("Resolucao ADC: %d bits (0-%d)\n", ADC_RESOLUCAO_BITS, ADC_VALOR_MAX);
  Serial.println(F("Gire o potenciometro para mover o servo..."));
  Serial.println(F("------------------------------------------------------"));
}

void loop() {
  int leituraADC = analogRead(PIN_POTENCIOMETRO);

  // Converte a leitura bruta do ADC em duty cycle percentual (0-100%)
  float dutyPercent = (leituraADC * 100.0f) / ADC_VALOR_MAX;

  // Converte a leitura em angulo do servo (0-180 graus)
  int angulo = map(leituraADC, 0, ADC_VALOR_MAX, SERVO_ANGULO_MIN, SERVO_ANGULO_MAX);
  angulo = constrain(angulo, SERVO_ANGULO_MIN, SERVO_ANGULO_MAX);

  meuServo.write(angulo);

  Serial.printf("ADC: %4d/%d | Duty: %6.2f%% | Angulo do servo: %3d graus\n",
                leituraADC, ADC_VALOR_MAX, dutyPercent, angulo);

  delay(UPDATE_PERIOD_MS);
}

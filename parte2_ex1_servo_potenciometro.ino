// ==========================================================================
// SEL0433 - Aplicação de Microprocessadores
// Projeto 3 - Parte 2 / Exercício 1
// Controle PWM de servomotor via potenciômetro (biblioteca ESP32Servo)
// ==========================================================================


#include <ESP32Servo.h>

// Configurações de hardware
#define PIN_SERVO        18
#define PIN_POTENCIOMETRO 34
#define BAUD_RATE        115200

// Parâmetros do servo
#define SERVO_PULSO_MIN_US  500   // pulso minimo (0 graus)
#define SERVO_PULSO_MAX_US  2400  // pulso maximo (180 graus)
#define SERVO_ANGULO_MIN    0
#define SERVO_ANGULO_MAX    180

// Parâmetros do ADC
#define ADC_RESOLUCAO_BITS  12
#define ADC_VALOR_MAX       4095  // 2^12 - 1

#define UPDATE_PERIOD_MS    100

Servo meuServo;

void setup() {
  Serial.begin(BAUD_RATE);
  delay(300);
  Serial.println();

  meuServo.setPeriodHertz(50);
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

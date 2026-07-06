/* ==========================================================================
 * SEL0433 - Aplicação de Microprocessadores
 * Projeto 3 - Parte 2 / Exercício 2
 * Aplicação proposta: Controlador de velocidade e sentido de rotação de um
 * motor DC usando a biblioteca nativa MCPWM (Motor Control PWM) da ESP32.
 * ==========================================================================
 *
 * DESCRIÇÃO GERAL
 *   O grupo escolheu como atuador um MOTOR DC acionado por ponte H (ex.:
 *   L298N, TB6612 ou DRV8833). A biblioteca MCPWM gera dois sinais PWM
 *   (MCPWM0A e MCPWM0B) que vão para as entradas IN1/IN2 da ponte H:
 *     - Girar em um sentido    -> PWM em IN1, IN2 em nível baixo
 *     - Girar no sentido oposto-> PWM em IN2, IN1 em nível baixo
 *     - O duty cycle aplicado controla a VELOCIDADE do motor.
 *
 * RECURSOS ADICIONAIS INCORPORADOS (além do PWM, conforme exigido)
 *   1) ADC (potenciômetro)      -> define a velocidade (duty cycle 0-100%)
 *   2) Botão + interrupção      -> alterna o SENTIDO de rotação (debounce
 *                                   por software dentro da ISR)
 *   3) Buzzer                   -> alerta sonoro ao trocar de sentido e ao
 *                                   atingir velocidade >= 95% (proteção)
 *   4) Display OLED (I2C)       -> mostra velocidade, sentido e status
 *   5) Comunicação serial       -> monitoramento em tempo real (115200 baud)
 *
 * HARDWARE (ligações sugeridas)
 *   Ponte H (ex. L298N) / motor DC:
 *     IN1 -> GPIO 32 (MCPWM0A)
 *     IN2 -> GPIO 33 (MCPWM0B)
 *     (alimentação do motor e da ponte H conforme datasheet do driver)
 *   Potenciômetro:
 *     VCC -> 3V3 | GND -> GND | SIG -> GPIO 34
 *   Botão (troca de sentido):
 *     Um terminal -> GPIO 27 | outro terminal -> GND (usa INPUT_PULLUP)
 *   Buzzer:
 *     Terminal + -> GPIO 25 | Terminal - -> GND
 *   Display OLED SSD1306 128x64 (I2C):
 *     VCC -> 3V3 | GND -> GND | SDA -> GPIO 21 | SCL -> GPIO 22
 *
 * OBSERVAÇÃO SOBRE A SIMULAÇÃO NO WOKWI
 *   O catálogo padrão (gratuito) do Wokwi não possui um chip de ponte H
 *   pronto. Para validar a lógica de PWM/direção na simulação, o
 *   diagram.json incluso substitui a ponte H por DOIS LEDs indicadores
 *   (um para IN1/sentido horário, outro para IN2/sentido anti-horário):
 *   como o brilho de cada LED acompanha o duty cycle do respectivo canal
 *   MCPWM, dá para visualizar perfeitamente velocidade e sentido. Em uma
 *   montagem física real, IN1/IN2 vão para os pinos de entrada de uma
 *   ponte H de verdade, que aciona o motor DC.
 *
 * DECISÕES DE ENGENHARIA
 *   - MCPWM (em vez de LEDC) foi escolhido por já oferecer, nativamente,
 *     suporte a geração de dois sinais complementares por temporizador e
 *     funções prontas para zerar um canal (mcpwm_set_signal_low), o que
 *     simplifica a implementação de controle de sentido em pontes H.
 *   - A troca de sentido é tratada fora da ISR (uso de uma flag volátil)
 *     para manter a rotina de interrupção curta e evitar problemas de
 *     reentrância com Serial/display, que não devem ser chamados de
 *     dentro de uma ISR.
 *   - Debounce por tempo (millis()) evita múltiplas trocas de sentido
 *     por um único acionamento do botão.
 * ==========================================================================
 */

#include <Arduino.h>
#include "driver/mcpwm.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------- Pinos ----------------
#define PIN_MOTOR_IN1   32   // MCPWM0A
#define PIN_MOTOR_IN2   33   // MCPWM0B
#define PIN_POTENCIOMETRO 34
#define PIN_BOTAO       27
#define PIN_BUZZER      25

// ---------------- Display OLED ----------------
#define TELA_LARGURA    128
#define TELA_ALTURA     64
#define OLED_RESET      -1
#define OLED_ENDERECO   0x3C
Adafruit_SSD1306 display(TELA_LARGURA, TELA_ALTURA, &Wire, OLED_RESET);

// ---------------- Configuração do MCPWM ----------------
#define MCPWM_FREQ_HZ   1000   // 1 kHz, adequado para acionamento de motor DC

// ---------------- Parâmetros gerais ----------------
#define ADC_VALOR_MAX       4095
#define LIMIAR_ALERTA_PCT   95.0f
#define DEBOUNCE_MS         200
#define UPDATE_PERIOD_MS    150

// ---------------- Variáveis globais ----------------
volatile bool sentidoHorario = true;
volatile bool flagBotaoPressionado = false;
volatile unsigned long ultimoInstanteBotao = 0;

float velocidadeAtualPct = 0.0f;

// ---------------------------------------------------------------------
// ISR do botão (interrupção externa) - mantida curta, apenas sinaliza
// ---------------------------------------------------------------------
void IRAM_ATTR isrBotaoSentido() {
  unsigned long agora = millis();
  if (agora - ultimoInstanteBotao > DEBOUNCE_MS) {
    flagBotaoPressionado = true;
    ultimoInstanteBotao = agora;
  }
}

// ---------------------------------------------------------------------
// Emite "beeps" no buzzer (uso simples e bloqueante, chamado fora da ISR)
// ---------------------------------------------------------------------
void beep(int vezes, int duracaoMs) {
  for (int i = 0; i < vezes; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(duracaoMs);
    digitalWrite(PIN_BUZZER, LOW);
    if (i < vezes - 1) delay(duracaoMs);
  }
}

// ---------------------------------------------------------------------
// Aplica velocidade (0-100%) e sentido no motor via MCPWM
// ---------------------------------------------------------------------
void atualizarMotor(float dutyPercent, bool horario) {
  dutyPercent = constrain(dutyPercent, 0.0f, 100.0f);

  if (horario) {
    mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, dutyPercent);
  } else {
    mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, dutyPercent);
  }
}

// ---------------------------------------------------------------------
// Atualiza o display OLED com velocidade, sentido e status
// ---------------------------------------------------------------------
void atualizarDisplay(float dutyPercent, bool horario, bool alerta) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Controle Motor DC (MCPWM)");
  display.drawLine(0, 10, TELA_LARGURA, 10, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(0, 20);
  display.printf("%3.0f%%", dutyPercent);

  display.setTextSize(1);
  display.setCursor(0, 45);
  display.print("Sentido: ");
  display.println(horario ? "HORARIO" : "ANTI-HOR.");

  display.setCursor(0, 55);
  display.print(alerta ? "STATUS: MAX/ALERTA" : "STATUS: OK");

  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println(F("=========================================================="));
  Serial.println(F(" Projeto 3 - Parte 2 / Exercicio 2: Motor DC com MCPWM"));
  Serial.println(F("=========================================================="));

  pinMode(PIN_BOTAO, INPUT_PULLUP);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  attachInterrupt(digitalPinToInterrupt(PIN_BOTAO), isrBotaoSentido, FALLING);

  analogReadResolution(12);

  // ---------- Inicializa o periférico MCPWM ----------
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, PIN_MOTOR_IN1);
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, PIN_MOTOR_IN2);

  mcpwm_config_t configuracaoPWM;
  configuracaoPWM.frequency    = MCPWM_FREQ_HZ;
  configuracaoPWM.cmpr_a       = 0;
  configuracaoPWM.cmpr_b       = 0;
  configuracaoPWM.counter_mode = MCPWM_UP_COUNTER;
  configuracaoPWM.duty_mode    = MCPWM_DUTY_MODE_0;
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &configuracaoPWM);

  // ---------- Inicializa o display OLED ----------
  Wire.begin(21, 22); // SDA, SCL
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ENDERECO)) {
    Serial.println(F("ERRO: falha ao iniciar o display OLED SSD1306!"));
  } else {
    display.clearDisplay();
    display.display();
  }

  Serial.println(F("Sistema pronto. Gire o potenciometro para ajustar a velocidade"));
  Serial.println(F("e pressione o botao para alternar o sentido de rotacao."));
  Serial.println(F("----------------------------------------------------------"));

  beep(1, 100); // beep de inicializacao
}

void loop() {
  // ---------- Leitura do potenciômetro -> velocidade (0-100%) ----------
  int leituraADC = analogRead(PIN_POTENCIOMETRO);
  velocidadeAtualPct = (leituraADC * 100.0f) / ADC_VALOR_MAX;

  // ---------- Trata o botão fora da ISR ----------
  if (flagBotaoPressionado) {
    flagBotaoPressionado = false;
    sentidoHorario = !sentidoHorario;
    beep(2, 80); // bipe duplo ao alternar o sentido
    Serial.println(">> Sentido de rotacao alternado!");
  }

  bool alerta = (velocidadeAtualPct >= LIMIAR_ALERTA_PCT);
  if (alerta) {
    beep(1, 30); // alerta curto de velocidade proxima do maximo
  }

  // ---------- Aplica no motor e atualiza saídas ----------
  atualizarMotor(velocidadeAtualPct, sentidoHorario);
  atualizarDisplay(velocidadeAtualPct, sentidoHorario, alerta);

  // ---------- Log via serial ----------
  Serial.printf("ADC: %4d | Velocidade: %6.2f%% | Sentido: %-10s | Alerta: %s\n",
                leituraADC, velocidadeAtualPct,
                sentidoHorario ? "HORARIO" : "ANTI-HORARIO",
                alerta ? "SIM" : "NAO");

  delay(UPDATE_PERIOD_MS);
}

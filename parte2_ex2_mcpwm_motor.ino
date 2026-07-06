// ==========================================================================
// SEL0433 - Aplicação de Microprocessadores
// Projeto 3 - Parte 2 / Exercício 2
// Aplicação proposta: Controlador de velocidade e sentido de rotação de um
// motor DC usando a biblioteca nativa MCPWM (Motor Control PWM) da ESP32.
// ==========================================================================

#include <Arduino.h>
#include "driver/mcpwm.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pinos
#define PIN_MOTOR_IN1   32   // MCPWM0A
#define PIN_MOTOR_IN2   33   // MCPWM0B
#define PIN_POTENCIOMETRO 34
#define PIN_BOTAO       27
#define PIN_BUZZER      25

// Display OLED
#define TELA_LARGURA    128
#define TELA_ALTURA     64
#define OLED_RESET      -1
#define OLED_ENDERECO   0x3C
Adafruit_SSD1306 display(TELA_LARGURA, TELA_ALTURA, &Wire, OLED_RESET);

// Configuração do MCPWM
#define MCPWM_FREQ_HZ   1000   // 1 kHz, adequado para acionamento de motor DC

// Parâmetros gerais
#define ADC_VALOR_MAX       4095
#define LIMIAR_ALERTA_PCT   95.0f
#define DEBOUNCE_MS         200
#define UPDATE_PERIOD_MS    150

// Variáveis globais
volatile bool sentidoHorario = true;
volatile bool flagBotaoPressionado = false;
volatile unsigned long ultimoInstanteBotao = 0;

float velocidadeAtualPct = 0.0f;

// ISR do botão (interrupção externa) - mantida curta, apenas sinaliza
void IRAM_ATTR isrBotaoSentido() {
  unsigned long agora = millis();
  if (agora - ultimoInstanteBotao > DEBOUNCE_MS) {
    flagBotaoPressionado = true;
    ultimoInstanteBotao = agora;
  }
}

// Emite "beeps" no buzzer
void beep(int vezes, int duracaoMs) {
  for (int i = 0; i < vezes; i++) {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(duracaoMs);
    digitalWrite(PIN_BUZZER, LOW);
    if (i < vezes - 1) delay(duracaoMs);
  }
}

// Aplica velocidade (0-100%) e sentido no motor via MCPWM
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

// Atualiza o display OLED com velocidade, sentido e status
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
  // Leitura do potenciômetro -> velocidade (0-100%)
  int leituraADC = analogRead(PIN_POTENCIOMETRO);
  velocidadeAtualPct = (leituraADC * 100.0f) / ADC_VALOR_MAX;

  // Trata o botão fora da ISR
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

  // Aplica no motor e atualiza saídas
  atualizarMotor(velocidadeAtualPct, sentidoHorario);
  atualizarDisplay(velocidadeAtualPct, sentidoHorario, alerta);

  // Log via serial
  Serial.printf("ADC: %4d | Velocidade: %6.2f%% | Sentido: %-10s | Alerta: %s\n",
                leituraADC, velocidadeAtualPct,
                sentidoHorario ? "HORARIO" : "ANTI-HORARIO",
                alerta ? "SIM" : "NAO");

  delay(UPDATE_PERIOD_MS);
}

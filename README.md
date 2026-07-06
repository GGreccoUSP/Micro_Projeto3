# SEL0433 — Projeto 3: Controle PWM e Comunicação

-- Giovanne Tomaszewski Grecco - 16228852


O projeto foi desenvolvido e testado na plataforma Wokwi. Ele implementa, em C++ o funcionamento de 3 exemplos diferentes: degradê de um LED, Controle PWM de servomotor via potenciômetro, e por fim, um Controlador de velocidade e sentido de rotação de um motor DC.

## Parte 1 — PWM do LED RGB (LEDC)

| Requisitos do projeto | Aplicações |
|---|---|
| LED RGB catodo comum, resistores 220 Ω | 3 canais LEDC independentes (R, G, B) |
| Resolução mínima 8 bits | `PWM_RESOLUTION = 8` (0–255) |
| Frequência 5 kHz | `PWM_FREQ_HZ = 5000` |
| Duty cycle 0–100% contínuo, incrementos distintos por cor | Verde +5%, Azul +10% (2×), Vermelho +15% (3×) — configurável em uma única constante `STEP_GREEN` |
| Mensagens via UART 115200 informando incrementos e duty cycles | `logSerial()` a cada ciclo |
| Desafio: controle via Bluetooth | `parte1_bonus_ble/` — controle manual via BLE UART (app "Serial Bluetooth Terminal" ou "nRF Connect"), com comandos `R<0-100>`, `G<0-100>`, `B<0-100>`, `AUTO` |

**Ligações:** R → resistor 220 Ω → GPIO 25 · G → resistor 220 Ω → GPIO 26 · B → resistor 220 Ω → GPIO 27 · Catodo comum → GND

Montagem do circuito da parte 1 na plataforma WOKWI:
<img width="681" height="680" alt="simulacao_3_1" src="https://github.com/user-attachments/assets/2491eb30-c4db-4e6d-875d-84d4e5d96225" />

## Parte 2 — Exercício 1: Servo + Potenciômetro

Usa a biblioteca **ESP32Servo**. O potenciômetro é lido pelo ADC (GPIO 34, 12 bits) e mapeado diretamente para o ângulo do servo (0–180°), atualizando a posição em tempo real conforme o usuário gira o potenciômetro.

**Ligações:** Potenciômetro → VCC/GND/SIG(GPIO 34) · Servo → V+ (5V) / GND / sinal (GPIO 18)

Montagem do circuito da parte 2 ex 1 na plataforma WOKWI:
<img width="713" height="716" alt="simulacao_final_2" src="https://github.com/user-attachments/assets/c88f7b3a-d39d-473a-8305-0d32c8a4a2f4" />

## Parte 2 — Exercício 2: Motor DC com MCPWM (aplicação proposta pelo grupo)

**Aplicação escolhida:** controlador de **velocidade e sentido de rotação** de um motor DC via ponte H, usando a biblioteca nativa **MCPWM**.

Recursos adicionais incorporados (além do PWM, conforme pedido no enunciado):
- **ADC** (potenciômetro) → define a velocidade (duty cycle 0–100%)
- **Botão com interrupção externa** → alterna o sentido de rotação (com debounce)
- **Buzzer** → alerta sonoro ao trocar de sentido e ao atingir velocidade ≥ 95%
- **Display OLED via I2C** → mostra velocidade, sentido e status do sistema
- **Comunicação serial** (115200 baud) → monitoramento em tempo real

**Ligações:** IN1 (ponte H) → GPIO 32 · IN2 (ponte H) → GPIO 33 · Potenciômetro → GPIO 34 · Botão → GPIO 27 (para GND) · Buzzer → GPIO 25 · OLED SSD1306 (I2C) → SDA GPIO 21 / SCL GPIO 22

> O catálogo padrão do Wokwi não tem uma ponte H pronta, então o `diagram.json` dessa pasta usa dois LEDs indicadores (IN1/IN2) para visualizar o sinal MCPWM (o brilho de cada LED acompanha o duty cycle/velocidade). Em uma montagem física real, IN1/IN2 vão para os pinos de entrada de uma ponte H de verdade (L298N, TB6612, DRV8833 etc.), que aciona o motor.

Montagem do circuito da parte 2 ex 2 na plataforma WOKWI:
<img width="739" height="751" alt="simulacao_final3" src="https://github.com/user-attachments/assets/7c322956-950a-4fee-b88d-9f98fde8ba77" />

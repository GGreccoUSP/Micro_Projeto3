# SEL0433 — Projeto 3: Controle PWM e Comunicação

Código-fonte completo para as **Parte 1** e **Parte 2** do projeto, pronto para simulação no **Wokwi** (ESP32 DevKit).

## Estrutura do repositório

```
projeto3_SEL0433/
├── parte1_pwm_rgb/                     -> Parte 1 (obrigatório)
│   ├── parte1_pwm_rgb.ino
│   ├── diagram.json                    -> circuito pronto para importar no Wokwi
│   └── libraries.txt
│
├── parte1_bonus_ble/                   -> Parte 1, desafio (opcional)
│   ├── parte1_bonus_ble.ino
│   └── libraries.txt
│
├── parte2_ex1_servo_potenciometro/     -> Parte 2, exercício 1 (obrigatório)
│   ├── parte2_ex1_servo_potenciometro.ino
│   ├── diagram.json
│   └── libraries.txt
│
└── parte2_ex2_mcpwm_motor/             -> Parte 2, exercício 2 (obrigatório)
    ├── parte2_ex2_mcpwm_motor.ino
    ├── diagram.json
    └── libraries.txt
```

## Como testar no Wokwi

1. Acesse [wokwi.com](https://wokwi.com/) e crie um novo projeto **ESP32**.
2. Cole o conteúdo do `.ino` correspondente no editor de código (`sketch.ino`).
3. Abra a aba de bibliotecas do projeto e adicione as bibliotecas listadas no `libraries.txt` da pasta (quando houver) pelo nome exato, via Library Manager do Wokwi.
4. Para o circuito: você pode montar manualmente arrastando as peças conforme a tabela de ligações de cada pasta, **ou** colar o conteúdo do `diagram.json` incluso na aba "diagram.json" do projeto Wokwi.
   > **Atenção:** os `diagram.json` foram escritos manualmente como ponto de partida. Confira as ligações na tela do simulador antes de rodar — pode ser necessário reposicionar peças ou ajustar algum nome de pino, já que a nomenclatura exata de cada peça pode variar entre versões do Wokwi.
5. Abra o Monitor Serial a **115200 baud** e clique em ▶️ (play) para simular.

## Parte 1 — PWM do LED RGB (LEDC)

| Requisito do enunciado | Como foi atendido |
|---|---|
| LED RGB catodo comum, resistores 220 Ω | 3 canais LEDC independentes (R, G, B) |
| Resolução mínima 8 bits | `PWM_RESOLUTION = 8` (0–255) |
| Frequência 5 kHz | `PWM_FREQ_HZ = 5000` |
| Duty cycle 0–100% contínuo, incrementos distintos por cor | Verde +5%, Azul +10% (2×), Vermelho +15% (3×) — configurável em uma única constante `STEP_GREEN` |
| Mensagens via UART 115200 informando incrementos e duty cycles | `logSerial()` a cada ciclo |
| Desafio: controle via Bluetooth | `parte1_bonus_ble/` — controle manual via BLE UART (app "Serial Bluetooth Terminal" ou "nRF Connect"), com comandos `R<0-100>`, `G<0-100>`, `B<0-100>`, `AUTO` |

**Ligações:** R → resistor 220 Ω → GPIO 25 · G → resistor 220 Ω → GPIO 26 · B → resistor 220 Ω → GPIO 27 · Catodo comum → GND

## Parte 2 — Exercício 1: Servo + Potenciômetro

Usa a biblioteca **ESP32Servo**. O potenciômetro é lido pelo ADC (GPIO 34, 12 bits) e mapeado diretamente para o ângulo do servo (0–180°), atualizando a posição em tempo real conforme o usuário gira o potenciômetro.

**Ligações:** Potenciômetro → VCC/GND/SIG(GPIO 34) · Servo → V+ (5V) / GND / sinal (GPIO 18)

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

**Diferencial sugerido (não implementado neste código, mas fácil de agregar):** adicionar BLE ou Wi-Fi para enviar comandos remotos de velocidade/sentido, reaproveitando o mesmo padrão de comandos por texto usado no desafio da Parte 1 (`parte1_bonus_ble`).

## Observação sobre versões do core arduino-esp32

O código usa a API "clássica" do LEDC (`ledcSetup` + `ledcAttachPin` + `ledcWrite(canal, duty)`), compatível com o core arduino-esp32 **2.x** (o mais comum no Wokwi). Se você estiver usando o core **3.x** (onde essas funções foram descontinuadas), troque pelas chamadas equivalentes `ledcAttach(pino, freq, resolucao)` e `ledcWrite(pino, duty)` — há um comentário indicando isso diretamente no arquivo `parte1_pwm_rgb.ino`.

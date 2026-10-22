# Lista de Conexões

Referência: `kicad/playboyzinho.kicad_sch`. A tabela abaixo descreve as conexões atuais do esquemático, com tela e cartão SD em barramentos separados.

## ESP32-S3

| Pino do ESP32-S3 | Sinal | Destino / Função |
|---|---|---|
| GPIO4 | SD_SCK | Clock SPI do cartão SD |
| GPIO5 | SD_MISO | SPI MISO do cartão SD |
| GPIO6 | SD_MOSI | SPI MOSI do cartão SD |
| GPIO7 | SD_CS | Chip Select do cartão SD |
| GPIO15 | TFT_SCK | Clock SPI da tela |
| GPIO16 | TFT_MISO | SPI MISO da tela |
| GPIO18 | TFT_MOSI | SPI MOSI da tela |
| GPIO8 | TFT_DC | Tela - D/C |
| GPIO9 | TFT_RESET | Tela - RESET |
| GPIO10 | TFT_CS | Tela - CS |
| GPIO11 | AMP_SD | MAX98357 - SD/MODE (habilitação e desligamento) |
| GPIO12 | DIN | MAX98357 - DIN |
| GPIO13 | BCLK | MAX98357 - BCLK |
| GPIO14 | LRC | MAX98357 - LRC |
| GPIO2 | Button-1 | Botão SW1 |
| GPIO42 | Button-2 | Botão SW2 |
| GPIO41 | Button-3 | Botão SW3 |
| GPIO39 | Button-4 | Botão SW4 |
| GPIO40 | Button-5 | Botão SW5 |
| GPIO38 | Button-6 | Botão SW6 |
| GPIO21 | Button-7 | Botão SW7 |
| GPIO47 | Button-8 | Botão SW8 |
| GPIO48 | LED RGB integrado | Conexão interna da placa, confirmada por teste; sem conexão externa no projeto |
| GPIO35, GPIO36 e GPIO37 | Reservados à PSRAM | Sem conexão externa no projeto |
| 3V3 | 3V3 | Terminal LED da tela |
| 5V | +5V | Alimentação da placa pela saída do MT3608 |
| GND | GND | Referência comum do circuito |

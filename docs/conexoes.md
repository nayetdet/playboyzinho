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
| GPIO11 | Sem conexão externa | O SD/MODE do amplificador é controlado pela chave SW10 |
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
| 3V3 | 3V3 | Terminal LED da tela e contato 3 de SW10 |
| 5V | +5V | Alimentação da placa pela saída do MT3608 |
| GND | GND | Referência comum do circuito |

## Tela e cartão SD (U4)

| Terminal do módulo | Rede | Ligação |
|---|---|---|
| 1 — VCC | +5V | Saída VOUT+ do MT3608 |
| 2 — GND | GND | Terra comum |
| 3 — CS | TFT_CS | GPIO10 |
| 4 — RESET | TFT_RESET | GPIO9 |
| 5 — DC | TFT_DC | GPIO8 |
| 6 — SDI (MOSI) | TFT_MOSI | GPIO18 |
| 7 — SCK | TFT_SCK | GPIO15 |
| 8 — LED | 3V3 | Saída de 3,3 V do ESP32-S3 |
| 9 — SDO (MISO) | TFT_MISO | GPIO16 |
| 10 a 14 — touch | Sem conexão externa | T_CLK, T_CS, T_DIN, T_DO e T_IRQ |
| 15 — SD_CS | SD_CS | GPIO7 |
| 16 — SD_MOSI | SD_MOSI | GPIO6 |
| 17 — SD_MISO | SD_MISO | GPIO5 |
| 18 — SD_SCK | SD_SCK | GPIO4 |

Os sinais SPI da tela e do SD são separados, mesmo com ambos no módulo U4.

## Amplificador (U7) e chave de áudio (SW10)

| Terminal do MAX98357 | Rede | Ligação |
|---|---|---|
| 1 — LRC | LRC | GPIO14 |
| 2 — BCLK | BCLK | GPIO13 |
| 3 — DIN | DIN | GPIO12 |
| 4 — GAIN | Sem conexão externa | Configuração depende dos componentes do módulo |
| 5 — SD/MODE | AMP_SD | Terminal comum 2 de SW10 |
| 6 — GND | GND | Terra comum |
| 7 — VIN | +5V | Saída VOUT+ do MT3608 |

| Terminal de SW10 | Ligação | Função |
|---|---|---|
| 1 | GND | Conectado ao comum 2, desliga internamente o amplificador |
| 2 — comum | AMP_SD → U7, pino 5 | Entrada SD/MODE do amplificador |
| 3 | 3V3 | Conectado ao comum 2, habilita o amplificador |

SW10 não interrompe o VIN de 5 V. O GPIO11 fica sem conexão externa e não controla essa chave. A chave física deve desconectar um contato antes de conectar o outro; o terminal comum deve ser identificado conforme o modelo utilizado.

O alto-falante LS1 está identificado no esquemático como **8 Ω / 1 W**. Seus dois pinos estão marcados como sem conexão no desenho, com a indicação de montagem diretamente no borne de saída do amplificador. O borne não aparece no símbolo de U7; os fios do falante vão aos dois terminais de saída, sem ligação ao GND.

## Alimentação e chave geral (SW9)

| Origem | Destino / rede |
|---|---|
| U2, pino 1 — BAT+ | +BATT → U5 (TP4056), pino 2 — B+ |
| U2, pino 2 — BAT− | -BATT → U5, pino 3 — B− |
| U5, pino 1 — OUT+ | Terminal comum 2 de SW9 |
| SW9, contato 1 | Sem conexão; posição desligada |
| SW9, contato 3 | U3 (MT3608), pino 3 — VIN+; U6 (indicador), pino 2 — BAT+ |
| U5, pino 4 — OUT− | GND do console |
| U3, pinos 2 — VOUT− e 4 — VIN− | GND |
| U3, pino 1 — VOUT+ | +5V → U1, pino 21; U4, pino 1; U7, pino 7 |
| U6, pino 1 — BAT− | GND |
| U1, pino 1 — 3V3 | U4, pino 8 — LED; SW10, contato 3 |

A rede -BATT é distinta de GND: o negativo da bateria vai a B− do TP4056, enquanto o terra do console vem de OUT−. Os terminais IN+ e IN− do módulo TP4056 não têm fios externos no esquemático; a recarga usa o USB do próprio módulo. A conexão da bateria ao carregador permanece mesmo com SW9 desligada.

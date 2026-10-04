# Lista de Conexões

## ESP32-S3

| Pino do ESP32-S3 | Sinal | Destino / Função |
|---|---|---|
| GPIO4 | LRC | MAX98357 - LRC |
| GPIO5 | BCLK | MAX98357 - BCLK |
| GPIO6 | DIN | MAX98357 - DIN |
| GPIO8 | RESET | Display ILI9341 - RESET |
| GPIO9 | DC | Display ILI9341 - D/C |
| GPIO10 | CS | Display ILI9341 - CS |
| GPIO11 | MOSI | SPI MOSI do display e cartão SD |
| GPIO12 | SCK | SPI Clock do display e cartão SD |
| GPIO13 | MISO | SPI MISO do display e cartão SD |
| GPIO14 | SD_CS | Chip Select do cartão SD |
| GPIO42 | Button-1 | Botão SW1 |
| GPIO41 | Button-2 | Botão SW2 |
| GPIO40 | Button-3 | Botão SW3 |
| GPIO39 | Button-4 | Botão SW4 |
| GPIO38 | Button-5 | Botão SW5 |
| GPIO37 | Button-6 | Botão SW6 |
| GPIO36 | Button-7 | Botão SW7 |
| GPIO35 | Button-8 | Botão SW8 |
| 5V | VCC | Barramento de alimentação +5 V |
| GND | GND | Terra comum do circuito |

## Botões

Os botões são conectados entre o respectivo GPIO e o GND.

| Botão | Sinal | GPIO |
|---|---|---|
| SW1 | Button-42 | GPIO |
| SW2 | Button-41 | GPIO16 |
| SW3 | Button-40 | GPIO17 |
| SW4 | Button-39 | GPIO18 |
| SW5 | Button-38 | GPIO1 |
| SW6 | Button-37 | GPIO2 |
| SW7 | Button-36 | GPIO2 |
| SW8 | Button-35 | GPIO2 |

### Configuração dos botões

Os GPIOs devem ser configurados utilizando o resistor de pull-up interno do ESP32-S3.

```cpp
pinMode(15, INPUT_PULLUP);
pinMode(16, INPUT_PULLUP);
pinMode(17, INPUT_PULLUP);
pinMode(18, INPUT_PULLUP);
pinMode(1, INPUT_PULLUP);
pinMode(2, INPUT_PULLUP);

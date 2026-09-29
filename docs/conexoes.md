# Lista de Conexões

## ESP32-S3

| Pino do ESP32-S3 | Sinal | Destino / Função |
|---|---|---|
| GPIO1 | Button-5 | Botão SW5 |
| GPIO2 | Button-6 | Botão SW6 |
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
| GPIO15 | Button-1 | Botão SW1 |
| GPIO16 | Button-2 | Botão SW2 |
| GPIO17 | Button-3 | Botão SW3 |
| GPIO18 | Button-4 | Botão SW4 |
| 5V | VCC | Barramento de alimentação +5 V |
| GND | GND | Terra comum do circuito |

## Botões

Os botões são conectados entre o respectivo GPIO e o GND.

| Botão | Sinal | GPIO |
|---|---|---|
| SW1 | Button-1 | GPIO15 |
| SW2 | Button-2 | GPIO16 |
| SW3 | Button-3 | GPIO17 |
| SW4 | Button-4 | GPIO18 |
| SW5 | Button-5 | GPIO1 |
| SW6 | Button-6 | GPIO2 |

### Configuração dos botões

Os GPIOs devem ser configurados utilizando o resistor de pull-up interno do ESP32-S3.

```cpp
pinMode(15, INPUT_PULLUP);
pinMode(16, INPUT_PULLUP);
pinMode(17, INPUT_PULLUP);
pinMode(18, INPUT_PULLUP);
pinMode(1, INPUT_PULLUP);
pinMode(2, INPUT_PULLUP);

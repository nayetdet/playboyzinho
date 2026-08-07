#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4

#define TFT_SCK  18
#define TFT_MOSI 23

Adafruit_ILI9341 tft = Adafruit_ILI9341(
  TFT_CS,
  TFT_DC,
  TFT_RST
);

void setup() {
  Serial.begin(115200);

  // Inicializa o SPI usando os pinos escolhidos
  SPI.begin(
    TFT_SCK,   // SCK
    -1,        // MISO - não usado
    TFT_MOSI,  // MOSI
    TFT_CS     // SS
  );

  Serial.println("Inicializando tela...");

  tft.begin();
  tft.setRotation(1);

  // Teste visual
  tft.fillScreen(ILI9341_RED);
  delay(1000);

  tft.fillScreen(ILI9341_GREEN);
  delay(1000);

  tft.fillScreen(ILI9341_BLUE);
  delay(1000);

  tft.fillScreen(ILI9341_BLACK);

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(3);

  tft.setCursor(30, 50);
  tft.println("ESP32");

  tft.setTextColor(ILI9341_CYAN);
  tft.setTextSize(2);

  tft.setCursor(30, 100);
  tft.println("Tela funcionando!");

  tft.drawRect(20, 150, 280, 60, ILI9341_YELLOW);
}

void loop() {
}

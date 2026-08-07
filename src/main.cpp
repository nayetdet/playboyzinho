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

constexpr int SCREEN_WIDTH = 320;
constexpr int SCREEN_HEIGHT = 240;
constexpr int MAX_ITERATIONS = 80;
constexpr int ZOOM_STEPS = 90;

constexpr float START_CENTER_REAL = -0.6f;
constexpr float START_CENTER_IMAGINARY = 0.0f;
constexpr float START_VIEW_WIDTH = 3.2f;

// Regiao conhecida como "Seahorse Valley", rica em pequenos detalhes.
constexpr float ZOOM_CENTER_REAL = -0.7436439f;
constexpr float ZOOM_CENTER_IMAGINARY = 0.1318259f;
constexpr float ZOOM_VIEW_WIDTH = 0.035f;

uint32_t animationFrame = 0;

// Paleta classica do Mandelbrot, do marrom ao azul e ao dourado.
const uint8_t PALETTE[][3] = {
  { 66,  30,  15}, { 25,   7,  26}, {  9,   1,  47}, {  4,   4,  73},
  {  0,   7, 100}, { 12,  44, 138}, { 24,  82, 177}, { 57, 125, 209},
  {134, 181, 229}, {211, 236, 248}, {241, 233, 191}, {248, 201,  95},
  {255, 170,   0}, {204, 128,   0}, {153,  87,   0}, {106,  52,   3}
};

constexpr int PALETTE_SIZE = sizeof(PALETTE) / sizeof(PALETTE[0]);
constexpr int PALETTE_BLEND_STEPS = 16;

uint16_t fractalColor(int iterations, int paletteOffset) {
  if (iterations == MAX_ITERATIONS) {
    return ILI9341_BLACK;
  }

  // Interpola as cores para evitar faixas muito marcadas.
  const int colorPosition =
    (iterations * 4 + paletteOffset) % (PALETTE_SIZE * PALETTE_BLEND_STEPS);
  const int colorIndex = (colorPosition / PALETTE_BLEND_STEPS) % PALETTE_SIZE;
  const int nextColorIndex = (colorIndex + 1) % PALETTE_SIZE;
  const int blend = colorPosition % PALETTE_BLEND_STEPS;

  const uint8_t red =
    (PALETTE[colorIndex][0] * (PALETTE_BLEND_STEPS - blend)
      + PALETTE[nextColorIndex][0] * blend) / PALETTE_BLEND_STEPS;
  const uint8_t green =
    (PALETTE[colorIndex][1] * (PALETTE_BLEND_STEPS - blend)
      + PALETTE[nextColorIndex][1] * blend) / PALETTE_BLEND_STEPS;
  const uint8_t blue =
    (PALETTE[colorIndex][2] * (PALETTE_BLEND_STEPS - blend)
      + PALETTE[nextColorIndex][2] * blend) / PALETTE_BLEND_STEPS;

  return tft.color565(red, green, blue);
}

void drawMandelbrot(float centerReal, float centerImaginary,
                    float viewWidth, int paletteOffset) {
  static uint16_t scanline[SCREEN_WIDTH];
  const float viewHeight = viewWidth * SCREEN_HEIGHT / SCREEN_WIDTH;
  const float realMin = centerReal - viewWidth * 0.5f;
  const float imaginaryMin = centerImaginary - viewHeight * 0.5f;
  const float realStep = viewWidth / (SCREEN_WIDTH - 1);
  const float imaginaryStep = viewHeight / (SCREEN_HEIGHT - 1);

  tft.startWrite();
  tft.setAddrWindow(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

  for (int y = 0; y < SCREEN_HEIGHT; ++y) {
    const float ci = imaginaryMin + y * imaginaryStep;

    for (int x = 0; x < SCREEN_WIDTH; ++x) {
      const float cr = realMin + x * realStep;
      float zr = 0.0f;
      float zi = 0.0f;
      float zrSquared = 0.0f;
      float ziSquared = 0.0f;
      int iterations = 0;

      while (zrSquared + ziSquared <= 4.0f && iterations < MAX_ITERATIONS) {
        zi = 2.0f * zr * zi + ci;
        zr = zrSquared - ziSquared + cr;
        zrSquared = zr * zr;
        ziSquared = zi * zi;
        ++iterations;
      }

      scanline[x] = fractalColor(iterations, paletteOffset);
    }

    tft.writePixels(scanline, SCREEN_WIDTH);
  }

  tft.endWrite();
}

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
  tft.fillScreen(ILI9341_BLACK);

  Serial.println("Iniciando animacao do fractal de Mandelbrot...");
}

void loop() {
  // A primeira metade do ciclo aproxima; a segunda se afasta.
  const uint32_t cycleFrame = animationFrame % (ZOOM_STEPS * 2);
  float progress;

  if (cycleFrame <= ZOOM_STEPS) {
    progress = static_cast<float>(cycleFrame) / ZOOM_STEPS;
  } else {
    progress = static_cast<float>(ZOOM_STEPS * 2 - cycleFrame) / ZOOM_STEPS;
  }

  // Smoothstep deixa as inversoes do zoom suaves, sem trancos.
  const float easedProgress = progress * progress * (3.0f - 2.0f * progress);
  const float centerReal = START_CENTER_REAL
    + (ZOOM_CENTER_REAL - START_CENTER_REAL) * easedProgress;
  const float centerImaginary = START_CENTER_IMAGINARY
    + (ZOOM_CENTER_IMAGINARY - START_CENTER_IMAGINARY) * easedProgress;
  const float viewWidth = START_VIEW_WIDTH
    * powf(ZOOM_VIEW_WIDTH / START_VIEW_WIDTH, easedProgress);
  const int paletteOffset = (animationFrame * 2U)
    % (PALETTE_SIZE * PALETTE_BLEND_STEPS);

  drawMandelbrot(centerReal, centerImaginary, viewWidth, paletteOffset);
  ++animationFrame;
}

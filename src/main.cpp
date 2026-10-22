#include <Arduino.h>

// 1 testa apenas o amplificador e a SW10, sem tela ou botoes.
// Coloque em 0 para voltar aos modos anteriores.
#define AUDIO_ONLY_TEST 0
// No teste de audio: 0 = bipes gerados pelo ESP; 1 = /audio.pcm do SD.
#define AUDIO_TEST_USE_SD 0

// Testes temporarios: tela tem prioridade. Ambos em 0 restauram o player.
#define BUTTON_SCREEN_TEST 0
#define LED_GPIO_TEST 0

// Volume do player e do tom inicial: 0 = silencio, 100 = volume original.
// Altere esta porcentagem e grave o firmware novamente na placa.
constexpr int AUDIO_VOLUME_PERCENT = 10;
static_assert(AUDIO_VOLUME_PERCENT >= 0 && AUDIO_VOLUME_PERCENT <= 100,
              "O volume deve estar entre 0 e 100 por cento.");

#if AUDIO_ONLY_TEST
#include "audio_only_test.h"
#else

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// TFT ILI9341
#define TFT_CS 10
#define TFT_DC 8
#define TFT_RST 9
#define TFT_MOSI 18
#define TFT_SCK 15
#define TFT_MISO 16

// SW1 a SW8: pressionar conecta a entrada ao GND (ativo em LOW).
constexpr uint8_t BUTTON_PINS[] = {2, 42, 41, 39, 40, 38, 21, 47};
constexpr size_t BUTTON_COUNT = sizeof(BUTTON_PINS) / sizeof(BUTTON_PINS[0]);
constexpr uint32_t BUTTON_DEBOUNCE_MS = 30;
struct ButtonState {
  bool lastReading = HIGH;
  bool stableState = HIGH;
  uint32_t changedAt = 0;
};
ButtonState buttons[BUTTON_COUNT];

void handleButtonStateChange(size_t index, bool pressed);

void pollButtons() {
  const uint32_t now = millis();
  for (size_t index = 0; index < BUTTON_COUNT; ++index) {
    ButtonState &button = buttons[index];
    const bool reading = digitalRead(BUTTON_PINS[index]);
    if (reading != button.lastReading) {
      button.lastReading = reading;
      button.changedAt = now;
    }
    if (reading != button.stableState &&
        now - button.changedAt >= BUTTON_DEBOUNCE_MS) {
      button.stableState = reading;
      handleButtonStateChange(index, reading == LOW);
    }
  }
}

#if BUTTON_SCREEN_TEST

SPIClass tftSpi(FSPI);
Adafruit_ILI9341 tft(&tftSpi, TFT_DC, TFT_CS, TFT_RST);
uint32_t buttonPressCounts[BUTTON_COUNT] = {};

void drawButtonRow(size_t index) {
  const bool pressed = buttons[index].stableState == LOW;
  const uint16_t background = pressed ? ILI9341_GREEN : ILI9341_BLACK;
  const int16_t y = 47 + index * 18;
  tft.fillRect(6, y, 308, 16, background);
  tft.setTextSize(1);
  tft.setTextColor(pressed ? ILI9341_BLACK : ILI9341_WHITE, background);
  tft.setCursor(10, y + 4);
  tft.printf("SW%u  GPIO%-2u  %-11s  apertos: %lu",
             static_cast<unsigned>(index + 1),
             static_cast<unsigned>(BUTTON_PINS[index]),
             pressed ? "PRESSIONADO" : "SOLTO",
             static_cast<unsigned long>(buttonPressCounts[index]));
}

void handleButtonStateChange(size_t index, bool pressed) {
  if (pressed) {
    ++buttonPressCounts[index];
    // Mantem o ultimo botao identificado mesmo depois de soltar o jumper.
    tft.fillRect(6, 212, 308, 24, ILI9341_BLACK);
    tft.setCursor(10, 214);
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
    tft.printf("SW%u / GPIO%u", static_cast<unsigned>(index + 1),
               static_cast<unsigned>(BUTTON_PINS[index]));
  }
  drawButtonRow(index);
  Serial.printf("SW%u (GPIO%u): %s | apertos: %lu\n",
                static_cast<unsigned>(index + 1),
                static_cast<unsigned>(BUTTON_PINS[index]),
                pressed ? "PRESSIONADO" : "SOLTO",
                static_cast<unsigned long>(buttonPressCounts[index]));
}

void setup() {
  Serial.begin(115200);
  pinMode(11, OUTPUT);  // Amplificador desligado.
  digitalWrite(11, LOW);
  pinMode(7, OUTPUT);   // Cartao SD desmarcado; nao e utilizado neste teste.
  digitalWrite(7, HIGH);
  for (uint8_t pin : BUTTON_PINS) pinMode(pin, INPUT_PULLUP);

  tftSpi.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.begin(40000000);
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextWrap(false);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 6);
  tft.println("TESTE DOS BOTOES");
  tft.setTextSize(1);
  tft.setCursor(10, 30);
  tft.println("Ligue um GPIO ao GND para testar.");
  for (size_t index = 0; index < BUTTON_COUNT; ++index) drawButtonRow(index);
  tft.setCursor(10, 198);
  tft.println("Ultimo botao pressionado:");
  Serial.println("Teste de botoes na tela: conecte cada entrada ao GND.");
}

void loop() {
  pollButtons();
  delay(1);
}

#elif LED_GPIO_TEST
#include <esp32-hal-rgb-led.h>

constexpr uint8_t TEST_LED_PIN = 48;
void handleButtonStateChange(size_t, bool) {}

void setup() {
  Serial.begin(115200);
  // Mantem o amplificador desligado durante o teste.
  pinMode(11, OUTPUT);
  digitalWrite(11, LOW);
  delay(500);
  Serial.println("Teste do LED RGB no GPIO48.");
  neopixelWrite(TEST_LED_PIN, 0, 0, 0);
}

void loop() {
  Serial.println("GPIO48: vermelho");
  neopixelWrite(TEST_LED_PIN, 32, 0, 0);
  delay(700);
  Serial.println("GPIO48: verde");
  neopixelWrite(TEST_LED_PIN, 0, 32, 0);
  delay(700);
  Serial.println("GPIO48: azul");
  neopixelWrite(TEST_LED_PIN, 0, 0, 32);
  delay(700);
  Serial.println("GPIO48: apagado");
  neopixelWrite(TEST_LED_PIN, 0, 0, 0);
  delay(700);
}

#else

#include <SD.h>
#include <esp_heap_caps.h>
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// Cartao SD em um controlador SPI independente da tela.
#define SD_CS 7
#define SD_MOSI 6
#define SD_MISO 5
#define SD_SCK 4

// MAX98357A
#define I2S_BCLK 13
#define I2S_LRC 14
#define I2S_DIN 12
#define AMP_SD 11

// Arquivo rawvideo gerado pelo FFmpeg: RGB565 little-endian, 320x240, 10 FPS.
constexpr char VIDEO_PATH[] = "/video.rgb565";
constexpr uint16_t SCREEN_WIDTH = 320;
constexpr uint16_t SCREEN_HEIGHT = 240;
constexpr uint8_t VIDEO_FPS = 10;
constexpr size_t FRAME_PIXELS = SCREEN_WIDTH * SCREEN_HEIGHT;
constexpr size_t FRAME_BYTES = FRAME_PIXELS * sizeof(uint16_t);
constexpr uint32_t FRAME_INTERVAL_MS = 1000 / VIDEO_FPS;
constexpr char AUDIO_PATH[] = "/audio.pcm";
// 16 kHz estéreo cabe integralmente na PSRAM de 8 MB. Isso remove qualquer
// leitura do SD durante a reprodução de áudio.
constexpr uint32_t AUDIO_SAMPLE_RATE = 16000;
// Escritas curtas limitam o tempo de espera do controle de pausa pelo mutex.
constexpr size_t AUDIO_CHUNK_BYTES = 1024;

SPIClass tftSpi(FSPI);  // SPI2 no ESP32-S3.
SPIClass sdSpi(HSPI);   // SPI3 no ESP32-S3.
Adafruit_ILI9341 tft(&tftSpi, TFT_DC, TFT_CS, TFT_RST);
File videoFile;
File audioFile;
uint16_t *frameBuffer = nullptr;
uint8_t *audioData = nullptr;
size_t audioBytes = 0;
uint32_t renderedFrames = 0;
uint32_t nextFrameAt = 0;
bool playerReady = false;
bool playbackEnded = false;
bool playbackPaused = false;
bool audioMuted = false;
// Guarda o ultimo SW acionado (1 a 8); 0 significa nenhum aperto ainda.
uint8_t lastPressedButton = 0;
// Protege o estado do audio e o I2S, compartilhados entre os dois nucleos.
SemaphoreHandle_t audioMutex = nullptr;
bool audioPlaying = false;
size_t audioOffset = 0;  // Acessado somente com audioMutex adquirido.
QueueHandle_t buttonPressQueue = nullptr;
constexpr uint32_t BUTTON_POLL_INTERVAL_MS = 5;

int16_t scaleAudioSample(int16_t sample) {
  // Multiplica em 32 bits para evitar overflow das amostras PCM de 16 bits.
  return static_cast<int16_t>(static_cast<int32_t>(sample) *
                              AUDIO_VOLUME_PERCENT / 100);
}

void updateAmplifier() {
  digitalWrite(AMP_SD, playerReady && !playbackPaused && !audioMuted ? HIGH : LOW);
}

void drawButtonOverlay() {
  if (lastPressedButton == 0) return;
  // Numero amarelo sobre fundo preto no canto superior direito (320x240).
  constexpr int16_t badgeX = SCREEN_WIDTH - 26;
  tft.fillRect(badgeX, 4, 22, 24, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
  tft.setCursor(badgeX + 5, 8);
  tft.print(lastPressedButton);
}

bool restartPlayback() {
  // Reutiliza os buffers e a tarefa de audio; reabre somente o arquivo de video.
  videoFile = SD.open(VIDEO_PATH, FILE_READ);
  if (!videoFile || videoFile.size() == 0 || videoFile.size() % FRAME_BYTES != 0) {
    videoFile.close();
    Serial.println("Falha ao reabrir o video. Aperte um botao para tentar novamente.");
    return false;
  }

  xSemaphoreTake(audioMutex, portMAX_DELAY);
  audioOffset = 0;
  i2s_zero_dma_buffer(I2S_NUM_0);
  if (i2s_start(I2S_NUM_0) != ESP_OK) {
    xSemaphoreGive(audioMutex);
    videoFile.close();
    Serial.println("Falha ao reiniciar I2S. Aperte um botao para tentar novamente.");
    return false;
  }
  renderedFrames = 0;
  playbackPaused = false;
  playbackEnded = false;
  nextFrameAt = millis();
  audioPlaying = true;
  playerReady = true;
  updateAmplifier();
  xSemaphoreGive(audioMutex);
  Serial.println("Video e audio reiniciados do comeco.");
  return true;
}

void handleButtonPress(size_t index) {
  lastPressedButton = static_cast<uint8_t>(index + 1);
  // Atualiza imediatamente, inclusive durante a pausa e no ultimo frame.
  if (playerReady || renderedFrames > 0) drawButtonOverlay();
  Serial.printf("SW%u (GPIO%u) pressionado: ",
                static_cast<unsigned>(index + 1), BUTTON_PINS[index]);
  if (playbackEnded) {
    // Este aperto apenas reinicia: nao pausa nem altera o mute em seguida.
    restartPlayback();
    return;
  }
  if (!playerReady) {
    Serial.println("player indisponivel ou video encerrado.");
    return;
  }

  if (index < 4) {
    xSemaphoreTake(audioMutex, portMAX_DELAY);
    playbackPaused = !playbackPaused;
    audioPlaying = !playbackPaused;
    if (playbackPaused) {
      updateAmplifier();
      // Conserva os buffers DMA e a posicao do audio durante a pausa.
      i2s_stop(I2S_NUM_0);
    } else {
      i2s_start(I2S_NUM_0);
      nextFrameAt = millis();
      updateAmplifier();
    }
    xSemaphoreGive(audioMutex);
    Serial.println(playbackPaused ? "video e audio pausados." : "reproducao retomada.");
  } else {
    audioMuted = !audioMuted;
    // O I2S continua avancando enquanto o amplificador esta silenciado.
    updateAmplifier();
    Serial.println(audioMuted ? "som desativado." : "som ativado.");
  }
}

void handleButtonStateChange(size_t index, bool pressed) {
  // Esta funcao roda na tarefa de entrada: nao acessa tela, SD ou player.
  if (pressed) {
    const uint8_t buttonIndex = static_cast<uint8_t>(index);
    if (xQueueSend(buttonPressQueue, &buttonIndex, 0) != pdPASS) {
      Serial.println("Fila de botoes cheia: aperto descartado.");
    }
  }
}

void buttonInputTask(void *) {
  for (;;) {
    pollButtons();
    vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_INTERVAL_MS));
  }
}

bool startButtonInput() {
  buttonPressQueue = xQueueCreate(32, sizeof(uint8_t));
  if (!buttonPressQueue) return false;
  // Leitura no nucleo 0, independente do desenho do video no loop().
  // O audio continua com prioridade maior (5).
  if (xTaskCreatePinnedToCore(buttonInputTask, "buttons", 2048, nullptr, 2,
                              nullptr, 0) != pdPASS) {
    vQueueDelete(buttonPressQueue);
    buttonPressQueue = nullptr;
    return false;
  }
  return true;
}

void processButtonPresses() {
  if (!buttonPressQueue) return;
  uint8_t index;
  while (xQueueReceive(buttonPressQueue, &index, 0) == pdPASS) {
    // Acoes e desenho permanecem no mesmo contexto que o video.
    handleButtonPress(index);
  }
}

void showError(const char *message) {
  digitalWrite(AMP_SD, LOW);
  Serial.println(message);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_RED, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.setCursor(8, 20);
  tft.println("Erro no player");
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(1);
  tft.setCursor(8, 55);
  tft.println(message);
}

bool initDisplayAndSd() {
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  tftSpi.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.begin(40000000);
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);

  sdSpi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  bool sdReady = false;
  for (uint8_t attempt = 0; attempt < 3 && !sdReady; ++attempt) {
    delay(250);
    sdReady = SD.begin(SD_CS, sdSpi, 40000000);
    if (!sdReady) {
      SD.end();
      sdReady = SD.begin(SD_CS, sdSpi, 20000000);
    }
    if (!sdReady) SD.end();
  }
  if (!sdReady) {
    showError("Falha ao iniciar SD");
    return false;
  }

  videoFile = SD.open(VIDEO_PATH, FILE_READ);
  if (!videoFile) {
    showError("Nao achei /video.rgb565");
    return false;
  }
  if (videoFile.size() == 0 || videoFile.size() % FRAME_BYTES != 0) {
    showError("Video com tamanho invalido");
    videoFile.close();
    return false;
  }

  audioFile = SD.open(AUDIO_PATH, FILE_READ);
  if (!audioFile) {
    showError("Nao achei /audio.pcm");
    videoFile.close();
    return false;
  }

  // Um frame inteiro fica na PSRAM; nao ha descompressao nem fila de video.
  frameBuffer = static_cast<uint16_t *>(
      heap_caps_malloc(FRAME_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!frameBuffer) {
    showError("Sem memoria para frame");
    videoFile.close();
    return false;
  }

  audioBytes = audioFile.size();
  if (audioBytes == 0 || audioBytes > 7 * 1024 * 1024) {
    showError("Audio grande demais");
    videoFile.close();
    audioFile.close();
    return false;
  }
  if (audioBytes % (2 * sizeof(int16_t)) != 0) {
    showError("PCM estereo com tamanho invalido");
    videoFile.close();
    audioFile.close();
    return false;
  }
  audioData = static_cast<uint8_t *>(
      heap_caps_malloc(audioBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!audioData) {
    showError("Sem memoria para audio");
    videoFile.close();
    audioFile.close();
    return false;
  }

  size_t loaded = 0;
  while (loaded < audioBytes) {
    const size_t received = audioFile.read(audioData + loaded,
                                           audioBytes - loaded);
    if (received == 0) {
      showError("Falha ao ler audio");
      videoFile.close();
      audioFile.close();
      return false;
    }
    loaded += received;
  }
  audioFile.close();

  // Ajusta uma unica vez na PSRAM, sem custo extra durante a reproducao
  // e sem reduzir o volume novamente quando o audio reinicia.
  int16_t *samples = reinterpret_cast<int16_t *>(audioData);
  for (size_t i = 0; i < audioBytes / sizeof(int16_t); ++i) {
    samples[i] = scaleAudioSample(samples[i]);
  }
  Serial.printf("Volume: %d%%\n", AUDIO_VOLUME_PERCENT);

  Serial.printf("Abrindo %s: %lu frames (%lu bytes).\n", VIDEO_PATH,
                static_cast<unsigned long>(videoFile.size() / FRAME_BYTES),
                static_cast<unsigned long>(videoFile.size()));
  return true;
}

void audioTask(void *) {
  // PCM estéreo, 16-bit little-endian, inteiramente residente na PSRAM.
  for (;;) {
    xSemaphoreTake(audioMutex, portMAX_DELAY);
    if (!audioPlaying) {
      xSemaphoreGive(audioMutex);
      vTaskDelay(pdMS_TO_TICKS(5));
      continue;
    }

    const size_t bytesToWrite = min(AUDIO_CHUNK_BYTES, audioBytes - audioOffset);
    size_t written = 0;
    i2s_write(I2S_NUM_0, audioData + audioOffset, bytesToWrite, &written,
              portMAX_DELAY);
    audioOffset += written;
    if (audioOffset >= audioBytes) audioOffset = 0;
    xSemaphoreGive(audioMutex);
    vTaskDelay(1);
  }
}

void playStartupTone() {
  // Confirma no alto-falante que o I2S/MAX98357A esta funcional antes de
  // iniciar a reproducao. Sao 120 ms de tom de 440 Hz.
  int16_t samples[256 * 2];
  uint32_t phase = 0;
  constexpr uint32_t phaseStep =
      static_cast<uint32_t>((static_cast<uint64_t>(440) << 32) /
                            AUDIO_SAMPLE_RATE);
  for (uint8_t block = 0; block < 11; ++block) {
    for (size_t i = 0; i < 256; ++i) {
      phase += phaseStep;
      const int16_t sample = scaleAudioSample(
          (phase & 0x80000000UL) ? 3500 : -3500);
      samples[i * 2] = sample;
      samples[i * 2 + 1] = sample;
    }
    size_t written = 0;
    i2s_write(I2S_NUM_0, samples, sizeof(samples), &written, portMAX_DELAY);
  }
}

bool initAudio() {
  const i2s_config_t config = {
      .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = AUDIO_SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      // 32 * 512 frames estéreo = ~1,0 s de reserva adicional no DMA.
      .dma_buf_count = 32,
      .dma_buf_len = 512,
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = 0};
  const i2s_pin_config_t pins = {
      .mck_io_num = I2S_PIN_NO_CHANGE,
      .bck_io_num = I2S_BCLK,
      .ws_io_num = I2S_LRC,
      .data_out_num = I2S_DIN,
      .data_in_num = I2S_PIN_NO_CHANGE};

  if (i2s_driver_install(I2S_NUM_0, &config, 0, nullptr) != ESP_OK ||
      i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK) {
    showError("Falha ao iniciar audio");
    return false;
  }
  i2s_zero_dma_buffer(I2S_NUM_0);
  // SD/MODE em HIGH habilita o amplificador e seleciona o canal esquerdo.
  digitalWrite(AMP_SD, HIGH);
  playStartupTone();
  digitalWrite(AMP_SD, LOW);
  i2s_stop(I2S_NUM_0);
  i2s_zero_dma_buffer(I2S_NUM_0);
  audioMutex = xSemaphoreCreateMutex();
  if (!audioMutex) {
    showError("Falha ao criar mutex de audio");
    return false;
  }
  if (xTaskCreatePinnedToCore(audioTask, "audio", 4096, nullptr, 5, nullptr, 0) != pdPASS) {
    showError("Falha ao criar tarefa de audio");
    return false;
  }
  return true;
}

bool renderNextFrame() {
  // Apenas esta tarefa acessa o SD; o audio ja esta inteiramente na PSRAM.
  const size_t received = videoFile.readBytes(
      reinterpret_cast<char *>(frameBuffer), FRAME_BYTES);
  if (received != FRAME_BYTES) {
    return false;
  }

  tft.startWrite();
  tft.setAddrWindow(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  // O arquivo usa rgb565le: o buffer vira uint16_t nativo e a biblioteca
  // envia os bytes na ordem esperada pelo ILI9341.
  tft.writePixels(frameBuffer, FRAME_PIXELS, true, false);
  tft.endWrite();
  // O frame ocupa toda a tela; repoe a identificacao a cada desenho.
  drawButtonOverlay();
  ++renderedFrames;
  return true;
}

void setup() {
  pinMode(AMP_SD, OUTPUT);
  digitalWrite(AMP_SD, LOW);
  for (uint8_t pin : BUTTON_PINS) {
    pinMode(pin, INPUT_PULLUP);
  }

  Serial.begin(115200);
  delay(300);

  if (!initDisplayAndSd()) return;
  if (!initAudio()) return;
  if (!startButtonInput()) {
    showError("Falha ao iniciar leitura dos botoes");
    return;
  }
  xSemaphoreTake(audioMutex, portMAX_DELAY);
  nextFrameAt = millis();
  audioPlaying = true;
  playerReady = true;
  i2s_start(I2S_NUM_0);
  updateAmplifier();
  xSemaphoreGive(audioMutex);
  Serial.println("Reproducao raw iniciada com audio PCM.");
  Serial.println("SW1-SW4: pausar/retomar. SW5-SW8: desativar/ativar som.");
  Serial.println("No fim do video, qualquer botao reinicia a reproducao.");
}

void loop() {
  // Os apertos sao capturados em paralelo, inclusive enquanto um frame e enviado.
  processButtonPresses();
  if (!playerReady || playbackPaused) {
    delay(1);
    return;
  }

  const uint32_t now = millis();
  if (static_cast<int32_t>(now - nextFrameAt) < 0) {
    delay(1);
    return;
  }

  if (!renderNextFrame()) {
    playbackEnded = videoFile.position() >= videoFile.size();
    Serial.println(playbackEnded ? "Fim do video. Aperte qualquer botao para reiniciar."
                                 : "Falha ao ler o video.");
    videoFile.close();
    xSemaphoreTake(audioMutex, portMAX_DELAY);
    audioPlaying = false;
    digitalWrite(AMP_SD, LOW);
    i2s_stop(I2S_NUM_0);
    i2s_zero_dma_buffer(I2S_NUM_0);
    playerReady = false;  // Mantem o ultimo frame na tela.
    xSemaphoreGive(audioMutex);
    return;
  }

  nextFrameAt += FRAME_INTERVAL_MS;
  // Se uma leitura/desenho atrasar muito, retoma o relogio sem acumular lag.
  if (static_cast<int32_t>(millis() - nextFrameAt) > FRAME_INTERVAL_MS) {
    nextFrameAt = millis() + FRAME_INTERVAL_MS;
  }

  if (renderedFrames % VIDEO_FPS == 0) {
    Serial.printf("Frames desenhados: %lu\n",
                  static_cast<unsigned long>(renderedFrames));
  }
}

#endif  // BUTTON_SCREEN_TEST / LED_GPIO_TEST

#endif  // AUDIO_ONLY_TEST

#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <esp_heap_caps.h>
#include <driver/i2s.h>

// TFT ILI9341
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8
#define TFT_MOSI 11
#define TFT_SCK 12
#define TFT_MISO 13

// SD no mesmo barramento SPI da TFT; somente o CS e separado.
#define SD_CS 14

// MAX98357A
#define I2S_BCLK 5
#define I2S_LRC 4
#define I2S_DIN 6

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
constexpr size_t AUDIO_CHUNK_BYTES = 16384;

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
File videoFile;
File audioFile;
uint16_t *frameBuffer = nullptr;
uint8_t *audioData = nullptr;
size_t audioBytes = 0;
SemaphoreHandle_t sdMutex = nullptr;
uint32_t renderedFrames = 0;
uint32_t nextFrameAt = 0;
bool playerReady = false;
volatile bool audioPlaying = false;

void showError(const char *message) {
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

  SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.begin(40000000);
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);

  bool sdReady = false;
  for (uint8_t attempt = 0; attempt < 3 && !sdReady; ++attempt) {
    delay(250);
    sdReady = SD.begin(SD_CS, SPI, 40000000);
    if (!sdReady) {
      SD.end();
      sdReady = SD.begin(SD_CS, SPI, 20000000);
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
  if (videoFile.size() % FRAME_BYTES != 0) {
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

  Serial.printf("Abrindo %s: %lu frames (%lu bytes).\n", VIDEO_PATH,
                static_cast<unsigned long>(videoFile.size() / FRAME_BYTES),
                static_cast<unsigned long>(videoFile.size()));
  return true;
}

void audioTask(void *) {
  // PCM estéreo, 16-bit little-endian, inteiramente residente na PSRAM.
  size_t offset = 0;
  for (;;) {
    if (!audioPlaying) {
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }

    const size_t bytesToWrite = min(AUDIO_CHUNK_BYTES, audioBytes - offset);
    size_t written = 0;
    i2s_write(I2S_NUM_0, audioData + offset, bytesToWrite, &written,
              portMAX_DELAY);
    offset += written;
    if (offset >= audioBytes) offset = 0;
  }
}

void playStartupTone() {
  // Confirma no alto-falante que o I2S/MAX98357A esta funcional antes de
  // iniciar a leitura do SD. Sao 120 ms de tom de 440 Hz.
  int16_t samples[256 * 2];
  uint32_t phase = 0;
  constexpr uint32_t phaseStep =
      static_cast<uint32_t>((static_cast<uint64_t>(440) << 32) /
                            AUDIO_SAMPLE_RATE);
  for (uint8_t block = 0; block < 11; ++block) {
    for (size_t i = 0; i < 256; ++i) {
      phase += phaseStep;
      const int16_t sample = (phase & 0x80000000UL) ? 3500 : -3500;
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
  playStartupTone();
  xTaskCreatePinnedToCore(audioTask, "audio", 4096, nullptr, 5, nullptr, 0);
  return true;
}

bool renderNextFrame() {
  // SD e TFT compartilham o mesmo SPI. O lock precisa cobrir tambem o envio
  // para a tela: sem isso a tarefa de audio pode iniciar uma leitura do SD no
  // meio de um frame, embaralhando a imagem.
  xSemaphoreTake(sdMutex, portMAX_DELAY);
  const size_t received = videoFile.readBytes(
      reinterpret_cast<char *>(frameBuffer), FRAME_BYTES);
  if (received != FRAME_BYTES) {
    xSemaphoreGive(sdMutex);
    return false;
  }

  tft.startWrite();
  tft.setAddrWindow(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  // O arquivo usa rgb565le: o buffer vira uint16_t nativo e a biblioteca
  // envia os bytes na ordem esperada pelo ILI9341.
  tft.writePixels(frameBuffer, FRAME_PIXELS, true, false);
  tft.endWrite();
  xSemaphoreGive(sdMutex);
  ++renderedFrames;
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(300);

  if (!initDisplayAndSd()) return;
  sdMutex = xSemaphoreCreateMutex();
  if (!sdMutex || !initAudio()) return;
  nextFrameAt = millis();
  audioPlaying = true;
  playerReady = true;
  Serial.println("Reproducao raw iniciada com audio PCM.");
}

void loop() {
  if (!playerReady) {
    delay(100);
    return;
  }

  const uint32_t now = millis();
  if (static_cast<int32_t>(now - nextFrameAt) < 0) {
    delay(1);
    return;
  }

  if (!renderNextFrame()) {
    Serial.println("Fim de /video.rgb565");
    videoFile.close();
    audioPlaying = false;
    i2s_zero_dma_buffer(I2S_NUM_0);
    playerReady = false;  // Mantem o ultimo frame na tela.
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

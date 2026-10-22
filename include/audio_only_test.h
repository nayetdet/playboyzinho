#pragma once

#include <driver/i2s.h>
#if AUDIO_TEST_USE_SD
#include <SPI.h>
#include <SD.h>
#include <esp_heap_caps.h>
#else
#include <cmath>
#endif

// Cartao SD: SCK=4, MISO=5, MOSI=6, CS=7.
// MAX98357: BCLK=13, LRC=14, DIN=12.
// SD/MODE do amplificador e controlado exclusivamente pela chave SW10.
// O GPIO11 nao e configurado nem acionado neste teste.
constexpr uint32_t AUDIO_TEST_SAMPLE_RATE = 16000;
bool audioTestReady = false;

#if AUDIO_TEST_USE_SD
constexpr char AUDIO_TEST_PATH[] = "/audio.pcm";
constexpr size_t AUDIO_TEST_CHUNK_BYTES = 4096;

SPIClass audioTestSdSpi(HSPI);
uint8_t *audioTestData = nullptr;
size_t audioTestBytes = 0;
size_t audioTestOffset = 0;

bool loadTestAudio() {
  audioTestSdSpi.begin(4, 5, 6, 7);
  bool sdReady = false;
  for (uint8_t attempt = 0; attempt < 3 && !sdReady; ++attempt) {
    delay(250);
    sdReady = SD.begin(7, audioTestSdSpi, 20000000);
    if (!sdReady) SD.end();
  }
  if (!sdReady) {
    Serial.println("Erro: falha ao iniciar o cartao SD.");
    return false;
  }

  File file = SD.open(AUDIO_TEST_PATH, FILE_READ);
  if (!file) {
    Serial.println("Erro: nao achei /audio.pcm no SD.");
    return false;
  }
  audioTestBytes = file.size();
  // PCM sem cabecalho, 16 bits little-endian, estereo: 4 bytes por frame.
  if (audioTestBytes == 0 || audioTestBytes > 7 * 1024 * 1024 ||
      audioTestBytes % 4 != 0) {
    Serial.println("Erro: PCM invalido ou maior que 7 MiB.");
    file.close();
    return false;
  }
  audioTestData = static_cast<uint8_t *>(
      heap_caps_malloc(audioTestBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!audioTestData) {
    Serial.println("Erro: sem PSRAM para carregar o audio.");
    file.close();
    return false;
  }

  size_t loaded = 0;
  while (loaded < audioTestBytes) {
    const size_t received = file.read(audioTestData + loaded, audioTestBytes - loaded);
    if (received == 0) {
      Serial.println("Erro: falha ao ler /audio.pcm.");
      file.close();
      heap_caps_free(audioTestData);
      audioTestData = nullptr;
      return false;
    }
    loaded += received;
  }
  file.close();

  // Aplica o volume uma vez; repetir o arquivo nao reduz o volume novamente.
  int16_t *samples = reinterpret_cast<int16_t *>(audioTestData);
  for (size_t i = 0; i < audioTestBytes / sizeof(int16_t); ++i) {
    samples[i] = static_cast<int16_t>(static_cast<int32_t>(samples[i]) *
                                     AUDIO_VOLUME_PERCENT / 100);
  }
  Serial.printf("Audio carregado: %lu bytes, volume %d%%.\n",
                static_cast<unsigned long>(audioTestBytes), AUDIO_VOLUME_PERCENT);
  return true;
}

#else
// Toque ascendente: 1000, 1500 e 2000 Hz, seguido de uma pausa.
// Os dois canais recebem o mesmo sinal para testar qualquer selecao de canal.
constexpr uint16_t TEST_TONE_FREQUENCIES[] = {1000, 1500, 2000};
constexpr size_t TEST_TONE_COUNT = sizeof(TEST_TONE_FREQUENCIES) /
                                   sizeof(TEST_TONE_FREQUENCIES[0]);
constexpr uint32_t NOTE_ON_FRAMES = AUDIO_TEST_SAMPLE_RATE * 180 / 1000;
constexpr uint32_t NOTE_SLOT_FRAMES = AUDIO_TEST_SAMPLE_RATE * 240 / 1000;
constexpr uint32_t TONE_CYCLE_FRAMES = AUDIO_TEST_SAMPLE_RATE * 1200 / 1000;
constexpr uint32_t TONE_FADE_FRAMES = AUDIO_TEST_SAMPLE_RATE * 5 / 1000;
constexpr size_t TONE_FRAMES = 256;
int16_t toneSamples[TONE_FRAMES * 2];
size_t toneByteOffset = 0;
uint32_t toneFrame = 0;

void generateTestTone() {
  constexpr float toneRadiansPerCycle = 6.28318530718f;
  constexpr int32_t amplitude = 8000 * AUDIO_VOLUME_PERCENT / 100;
  for (size_t frame = 0; frame < TONE_FRAMES; ++frame) {
    const size_t note = toneFrame / NOTE_SLOT_FRAMES;
    const uint32_t noteFrame = toneFrame % NOTE_SLOT_FRAMES;
    int16_t sample = 0;
    if (note < TEST_TONE_COUNT && noteFrame < NOTE_ON_FRAMES) {
      const float phase = toneRadiansPerCycle * TEST_TONE_FREQUENCIES[note] *
                          noteFrame / AUDIO_TEST_SAMPLE_RATE;
      // Entrada e saida suaves de 5 ms para reduzir estalos entre os tons.
      float envelope = 1.0f;
      if (noteFrame < TONE_FADE_FRAMES) {
        envelope = static_cast<float>(noteFrame) / TONE_FADE_FRAMES;
      } else if (NOTE_ON_FRAMES - noteFrame <= TONE_FADE_FRAMES) {
        envelope = static_cast<float>(NOTE_ON_FRAMES - noteFrame) / TONE_FADE_FRAMES;
      }
      sample = static_cast<int16_t>(amplitude * envelope * std::sin(phase));
    }
    toneSamples[frame * 2] = sample;
    toneSamples[frame * 2 + 1] = sample;
    toneFrame = (toneFrame + 1) % TONE_CYCLE_FRAMES;
  }
}
#endif

bool startTestI2s() {
  const i2s_config_t config = {
      .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = AUDIO_TEST_SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 512,
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = 0};
  const i2s_pin_config_t pins = {
      .mck_io_num = I2S_PIN_NO_CHANGE,
      .bck_io_num = 13,
      .ws_io_num = 14,
      .data_out_num = 12,
      .data_in_num = I2S_PIN_NO_CHANGE};
  if (i2s_driver_install(I2S_NUM_0, &config, 0, nullptr) != ESP_OK) {
    Serial.println("Erro: falha ao instalar I2S.");
    return false;
  }
  if (i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK) {
    Serial.println("Erro: falha ao configurar os pinos I2S.");
    i2s_driver_uninstall(I2S_NUM_0);
    return false;
  }
  i2s_zero_dma_buffer(I2S_NUM_0);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(300);
#if AUDIO_TEST_USE_SD
  Serial.println("Teste de audio: /audio.pcm, PCM estereo 16 bits / 16 kHz.");
  if (!loadTestAudio()) return;
#else
  Serial.printf("Teste sem SD: toque de 1000/1500/2000 Hz, volume %d%%.\n", AUDIO_VOLUME_PERCENT);
#endif
  if (!startTestI2s()) return;
  audioTestReady = true;
  Serial.println("Use SW10 para ligar/desligar o som. GPIO11 nao e utilizado.");
}

void loop() {
  if (!audioTestReady) {
    delay(100);
    return;
  }
#if AUDIO_TEST_USE_SD
  const size_t chunk = min(AUDIO_TEST_CHUNK_BYTES, audioTestBytes - audioTestOffset);
  const uint8_t *data = audioTestData + audioTestOffset;
#else
  if (toneByteOffset == 0) generateTestTone();
  const size_t chunk = sizeof(toneSamples) - toneByteOffset;
  const uint8_t *data = reinterpret_cast<const uint8_t *>(toneSamples) + toneByteOffset;
#endif
  size_t written = 0;
  const esp_err_t result = i2s_write(I2S_NUM_0, data, chunk, &written, portMAX_DELAY);
  if (result != ESP_OK || written == 0) {
    Serial.println("Erro: falha ao enviar audio ao I2S.");
    i2s_stop(I2S_NUM_0);
    i2s_zero_dma_buffer(I2S_NUM_0);
    audioTestReady = false;
    return;
  }
#if AUDIO_TEST_USE_SD
  audioTestOffset += written;
  if (audioTestOffset >= audioTestBytes) {
    audioTestOffset = 0;
    Serial.println("Reiniciando /audio.pcm.");
  }
#else
  toneByteOffset += written;
  if (toneByteOffset >= sizeof(toneSamples)) toneByteOffset = 0;
#endif
  delay(1);
}

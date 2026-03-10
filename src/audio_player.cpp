#include "audio_player.h"
#include "Audio.h"
#include "pmu_manager.h"
#include <LittleFS.h>
#include "es8311.h"

static constexpr int AUDIO_BUFFER_SIZE = 8000;
static constexpr int AUDIO_BUFFER_SIZE_PSRAM = 80000;
static constexpr int CODEC_STABILIZE_MS = 10;

static Audio audio;
static volatile bool audioFinished = false;
static TaskHandle_t audioTaskHandle = NULL;
static SemaphoreHandle_t audioMutex = NULL;
static es8311_handle_t codecHandle = NULL;

// Thread-safe audio access
static inline void audioLock()
{
    if (audioMutex)
        xSemaphoreTake(audioMutex, portMAX_DELAY);
}

static inline void audioUnlock()
{
    if (audioMutex)
        xSemaphoreGive(audioMutex);
}

// ── ES8311 codec init ────────────────────────────────────
static bool initCodec()
{
    codecHandle = es8311_create(I2C_NUM_0, ES8311_ADDRESS_0);
    if (!codecHandle)
    {
        Serial.println("[Audio] ES8311 create failed");
        return false;
    }

    const es8311_clock_config_t clk = {
        .mclk_inverted = false,
        .sclk_inverted = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency = AudioConfig::MCLK_FREQ,
        .sample_frequency = AudioConfig::SAMPLE_RATE};

    esp_err_t err = es8311_init(codecHandle, &clk,
                                ES8311_RESOLUTION_16, ES8311_RESOLUTION_16);
    if (err != ESP_OK)
    {
        Serial.printf("[Audio] ES8311 init failed: %d\n", err);
        es8311_delete(codecHandle);
        codecHandle = NULL;
        return false;
    }

    es8311_voice_volume_set(codecHandle, AudioConfig::DEFAULT_VOLUME, NULL); // Start at default; user volume applied later
    es8311_microphone_config(codecHandle, false);
    es8311_voice_mute(codecHandle, true); // Start muted

    Serial.println("[Audio] ES8311 codec initialized");
    return true;
}

// Audio task runs on Core 0 — blocks when idle, woken by xTaskNotifyGive
void audioTask(void *parameter)
{
    while (true)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        while (audio.isRunning())
        {
            audioLock();
            audio.loop();
            audioUnlock();
            vTaskDelay(1);
        }
    }
}

bool audioPlayerInit()
{
    if (audioTaskHandle != NULL)
        return true; // Already initialized

    audioMutex = xSemaphoreCreateMutex();

    // Wire.begin() already called by initHardware() in main.cpp
    initCodec();

    // Init I2S output — 4-pin: BCLK, LRC, DOUT, MCLK
    audio.setPinout(AudioConfig::BCLK, AudioConfig::LRC,
                    AudioConfig::DOUT, AudioConfig::MCLK);
    audio.setBufsize(AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE_PSRAM);
    audio.setVolume(AudioConfig::MAX_VOLUME); // I2S at max; user volume controlled via ES8311 codec

    // Audio task on Core 0 — needs enough stack and priority to avoid underruns
    xTaskCreatePinnedToCore(
        audioTask,
        "AudioTask",
        8192,
        NULL,
        5, // Higher priority prevents buffer underruns (crackling)
        &audioTaskHandle,
        0);

    Serial.println("[Audio] Player initialized (ES8311 + I2S)");
    return true;
}

bool playAudioFile(const char *filename)
{
    if (!LittleFS.exists(filename))
    {
        Serial.printf("[Audio] File not found: %s\n", filename);
        return false;
    }
    Serial.printf("[Audio] Codec=%s, Amp ON, unmute...\n",
                  codecHandle ? "OK" : "NULL!");
    enableAmp();
    delay(CODEC_STABILIZE_MS);
    audioFinished = false;
    audioLock();
    audio.connecttoFS(LittleFS, filename);
    audioUnlock();
    if (audioTaskHandle)
        xTaskNotifyGive(audioTaskHandle);
    else
        Serial.println("[Audio] WARNING: audioTask not running!");
    Serial.printf("[Audio] Playing: %s\n", filename);
    return true;
}

bool isPlaying()
{
    return !audioFinished && audio.isRunning();
}

bool isAudioFinished()
{
    return audioFinished;
}

void stopAudio()
{
    audioLock();
    audio.stopSong();
    audioUnlock();
    audioFinished = true;
    disableAmp();
}

void setVolume(uint8_t vol)
{
    // vol is 0-100 percentage, passed directly to ES8311 codec
    uint8_t clamped = (vol > AudioConfig::MAX_VOLUME_PCT) ? AudioConfig::MAX_VOLUME_PCT : vol;
    if (codecHandle)
        es8311_voice_volume_set(codecHandle, clamped, NULL);
}

void enableAmp()
{
    PmuManager::setSpeakerAmpEnabled(true);
    if (codecHandle)
        es8311_voice_mute(codecHandle, false);
}

void disableAmp()
{
    if (codecHandle)
        es8311_voice_mute(codecHandle, true);
    PmuManager::setSpeakerAmpEnabled(false);
}

void audioPlayerSuspend()
{
    disableAmp();
    if (codecHandle)
    {
        es8311_delete(codecHandle);
        codecHandle = NULL;
    }
    PmuManager::setCodecPowerEnabled(false);
}

void audioPlayerResume()
{
    PmuManager::setCodecPowerEnabled(true);
    delay(10); // ALDO1 settling time
    initCodec();
}

bool playAudioURL(const char *url)
{

    enableAmp();
    delay(CODEC_STABILIZE_MS);
    audioFinished = false;
    audioLock();
    audio.connecttohost(url);
    audioUnlock();
    if (audioTaskHandle)
        xTaskNotifyGive(audioTaskHandle);
    Serial.printf("Streaming: %s\n", url);
    return true;
}

void audio_info(const char *info)
{
    Serial.print("Audio Info: ");
    Serial.println(info);
}

void audio_eof_mp3(const char *info)
{
    Serial.print("Audio finished: ");
    Serial.println(info);
    audioFinished = true;
    disableAmp();
}

bool playAudioFileBlocking(const char *filename, PlaybackCallback onLoop)
{
    if (!playAudioFile(filename))
        return false;

    while (!isAudioFinished())
    {
        if (onLoop)
            onLoop();

        delay(1);
    }

    return true;
}

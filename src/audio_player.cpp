#include "audio_player.h"
#include "Audio.h"
#include <LittleFS.h>

static constexpr int AUDIO_BUFFER_SIZE = 8000;
static constexpr int AUDIO_BUFFER_SIZE_PSRAM = 80000;
static constexpr int AMP_STABILIZE_MS = 10;

static Audio audio;
static volatile bool audioFinished = false;
static TaskHandle_t audioTaskHandle = NULL;
static SemaphoreHandle_t audioMutex = NULL;

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

// Audio task runs on Core 0 to not block WiFi/HTTP on Core 1
void audioTask(void *parameter)
{
    while (true)
    {
        audioLock();
        audio.loop();
        audioUnlock();
        vTaskDelay(1); // Yield to other tasks
    }
}

bool audioPlayerInit()
{
    if (audioTaskHandle != NULL)
        return true; // Already initialized

    // Create mutex for thread-safe audio access
    audioMutex = xSemaphoreCreateMutex();

    pinMode(AudioConfig::SD_PIN, OUTPUT);
    disableAmp();

    audio.setPinout(AudioConfig::BCLK, AudioConfig::LRC, AudioConfig::DOUT);
    audio.setBufsize(AUDIO_BUFFER_SIZE, AUDIO_BUFFER_SIZE_PSRAM);
    audio.setVolume(AudioConfig::DEFAULT_VOLUME);
    audio.forceMono(true);

    // Create audio task on Core 0
    xTaskCreatePinnedToCore(
        audioTask,        // Task function
        "AudioTask",      // Name
        4096,             // Stack size
        NULL,             // Parameters
        1,                // Priority
        &audioTaskHandle, // Task handle
        0                 // Core 0
    );

    Serial.println("Audio player initialized");
    return true;
}

bool playAudioFile(const char *filename)
{
    if (!LittleFS.exists(filename))
    {
        Serial.printf("File not found: %s\n", filename);
        return false;
    }
    enableAmp();
    delay(AMP_STABILIZE_MS);
    audioFinished = false;
    audioLock();
    audio.connecttoFS(LittleFS, filename);
    audioUnlock();
    Serial.printf("Playing: %s\n", filename);
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
    audioLock();
    audio.setVolume(vol);
    audioUnlock();
}

void enableAmp()
{
    digitalWrite(AudioConfig::SD_PIN, HIGH);
}

void disableAmp()
{
    digitalWrite(AudioConfig::SD_PIN, LOW);
}

bool playAudioURL(const char *url)
{
    enableAmp();
    delay(AMP_STABILIZE_MS);
    audioFinished = false;
    audioLock();
    audio.connecttohost(url);
    audioUnlock();
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

#include "audio_player.h"
#include "Audio.h"
#include <LittleFS.h>

static Audio audio;
static volatile bool audioFinished = false;

bool audioPlayerInit()
{
    audio.setPinout(AudioConfig::BCLK, AudioConfig::LRC, AudioConfig::DOUT);

    // Use larger buffer for better quality (PSRAM available)
    audio.setBufsize(1600 * 5, 1600 * 50);

    audio.setVolume(AudioConfig::DEFAULT_VOLUME);

    // Mono output (MAX98357 is mono amp)
    audio.forceMono(true);

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
    audioFinished = false; // Reset flag before playing
    audio.connecttoFS(LittleFS, filename);
    Serial.printf("Playing: %s\n", filename);
    return true;
}

void audioPlayerLoop()
{
    audio.loop();
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
    audio.stopSong();
    audioFinished = true;
}

void setVolume(uint8_t vol)
{
    audio.setVolume(vol); // 0-21
}

bool playAudioURL(const char *url)
{
    audioFinished = false;
    audio.connecttohost(url);
    Serial.printf("Streaming: %s\n", url);
    return true;
}

// Optional callbacks for audio events
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
}

bool playAudioFileBlocking(const char *filename, PlaybackCallback onLoop)
{
    if (!playAudioFile(filename))
        return false;
    
    while (!isAudioFinished())
    {
        audioPlayerLoop();
        
        if (onLoop)
            onLoop();
        
        delay(1);
    }
    
    return true;
}

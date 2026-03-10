#pragma once

#include <Arduino.h>
#include <cstdint>

namespace AudioConfig
{
    // I2S pins for ES8311 codec (Waveshare ESP32-S3-Touch-LCD-3.5B)
    constexpr uint8_t BCLK = 13; // Bit Clock
    constexpr uint8_t LRC = 15;  // Left/Right Clock (Word Select)
    constexpr uint8_t DOUT = 16; // Data Out
    constexpr uint8_t MCLK = 44; // Master Clock (required by ES8311)

    // ES8311 codec init — optimized for 44.1 kHz MP3 playback
    constexpr int SAMPLE_RATE = 44100;
    constexpr int MCLK_MULTIPLE = 256;
    constexpr int MCLK_FREQ = SAMPLE_RATE * MCLK_MULTIPLE; // 11,289,600 Hz
    // Volume range: 0-21 (ESP32-audioI2S library I2S digital gain)
    constexpr uint8_t MAX_VOLUME = 21;

    // Codec / percentage volume constants
    constexpr uint8_t MAX_VOLUME_PCT = 100; // Max ES8311 codec percentage
    constexpr uint8_t DEFAULT_VOLUME = 80;  // Default volume (0-100%)
}

bool audioPlayerInit();
bool playAudioFile(const char *filename);
bool playAudioURL(const char *url);
bool isPlaying();
bool isAudioFinished();
void stopAudio();
void setVolume(uint8_t vol); // 0-100 percentage → ES8311 codec
void enableAmp();            // Enable audio output (unmute codec)
void disableAmp();           // Disable audio output (mute codec)
void audioPlayerSuspend();   // Power down codec before light sleep
void audioPlayerResume();    // Re-init codec after light sleep wake

// Callback type for work during playback (e.g., handle server, update volume)
using PlaybackCallback = void (*)();

// Play audio file and block until finished, calling callback each iteration
bool playAudioFileBlocking(const char *filename, PlaybackCallback onLoop = nullptr);

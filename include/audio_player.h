#pragma once

#include <Arduino.h>
#include <cstdint>

namespace AudioConfig
{
    // I2S pins for MAX98357
    constexpr uint8_t BCLK = 7;
    constexpr uint8_t LRC = 15;
    constexpr uint8_t DOUT = 6;

    // Volume range: 0-21 (lower = less distortion on MAX98357)
    constexpr uint8_t DEFAULT_VOLUME = 12;
    constexpr uint8_t MAX_VOLUME = 21;
}

bool audioPlayerInit();
bool playAudioFile(const char *filename);
bool playAudioURL(const char *url);
void audioPlayerLoop();
bool isPlaying();
bool isAudioFinished();
void stopAudio();
void setVolume(uint8_t vol); // 0-21

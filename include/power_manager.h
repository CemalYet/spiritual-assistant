#pragma once

#include <cstdint>

namespace PowerManager
{
    enum class State : uint8_t
    {
        ACTIVE,
        DIM,
        SCREEN_OFF
    };

    void init();
    void tick();
    void reportActivity();
    void enterLightSleep();
    State getState();
    bool isScreenOn();

    uint8_t acquireWakeLock(const char *name);
    void releaseWakeLock(uint8_t id);
    bool hasActiveWakeLocks();

    void wakeScreen();

    constexpr uint32_t DIM_TIMEOUT_MS = 10'000;
    constexpr uint32_t DIM_TIMEOUT_SAVER_MS = 5'000;
    constexpr uint32_t SCREEN_OFF_TIMEOUT_MS = 30'000;
    constexpr uint8_t ACTIVE_BRIGHTNESS = 140;
    constexpr uint8_t DIM_BRIGHTNESS = 38;
    constexpr uint8_t MAX_WAKE_LOCKS = 8;
}

class ScopedWakeLock
{
    uint8_t id_;

public:
    explicit ScopedWakeLock(const char *name) : id_(PowerManager::acquireWakeLock(name)) {}
    ~ScopedWakeLock() { PowerManager::releaseWakeLock(id_); }
    ScopedWakeLock(const ScopedWakeLock &) = delete;
    ScopedWakeLock &operator=(const ScopedWakeLock &) = delete;
};

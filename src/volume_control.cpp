#include "volume_control.h"
#include "app_state.h"
#include "audio_player.h"
#include "settings_manager.h"

namespace
{
    static constexpr int kMinVolumePct = 0;

    inline uint8_t clampVolume(uint8_t volumePct)
    {
        return volumePct > AudioConfig::MAX_VOLUME_PCT ? AudioConfig::MAX_VOLUME_PCT : volumePct;
    }
}

namespace VolumeControl
{
    bool isValid(int volumePct)
    {
        return volumePct >= kMinVolumePct && volumePct <= AudioConfig::MAX_VOLUME_PCT;
    }

    uint8_t normalize(int volumePct)
    {
        if (volumePct < kMinVolumePct)
            return static_cast<uint8_t>(kMinVolumePct);
        if (volumePct > AudioConfig::MAX_VOLUME_PCT)
            return AudioConfig::MAX_VOLUME_PCT;
        return static_cast<uint8_t>(volumePct);
    }

    void applyRuntime(uint8_t volumePct)
    {
        const uint8_t clamped = clampVolume(volumePct);
        ::setVolume(clamped);
        AppStateHelper::setVolume(clamped);
    }

    bool persist(uint8_t volumePct)
    {
        const uint8_t clamped = clampVolume(volumePct);
        return SettingsManager::setVolume(clamped);
    }

    bool commit(uint8_t volumePct)
    {
        const uint8_t clamped = clampVolume(volumePct);
        applyRuntime(clamped);
        return persist(clamped);
    }
}

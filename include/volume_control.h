#pragma once

#include <stdint.h>

namespace VolumeControl
{
    // Validate incoming API/UI integer volume range.
    bool isValid(int volumePct);

    // Clamp signed integer input into valid 0-100 range.
    uint8_t normalize(int volumePct);

    // Update live runtime volume only (codec + app state), no flash write.
    void applyRuntime(uint8_t volumePct);

    // Persist the already-applied runtime volume to NVS.
    bool persist(uint8_t volumePct);

    // Commit volume end-to-end (runtime + persistence).
    bool commit(uint8_t volumePct);
}

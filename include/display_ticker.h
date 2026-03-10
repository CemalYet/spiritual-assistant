#pragma once

namespace DisplayTicker
{
    /// Called from main loop every iteration — throttled to 1/sec internally.
    void tick();

    /// Force-populate AppState with current time, Gregorian date, Hijri date
    /// and NTP sync status. Call once after boot and after any time-sync event.
    void forceUpdate();
}

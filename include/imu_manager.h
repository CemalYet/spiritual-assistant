#pragma once

namespace ImuManager
{
    bool init();
    bool isAvailable();
    bool clearWakeStatus();
    bool armWakeOnMotion();
    void disarmAfterWake();
}

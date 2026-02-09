#pragma once

namespace WifiManager
{
    void init(bool connected);
    void tick();
    void reconnect();
    void disconnect();
    void resetTimeout();
}

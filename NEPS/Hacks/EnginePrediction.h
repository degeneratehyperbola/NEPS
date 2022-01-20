#pragma once

struct UserCmd;

namespace EnginePrediction
{
    void run(UserCmd* cmd) noexcept;
    void start(UserCmd* cmd) noexcept;
    void end(UserCmd* cmd) noexcept;
    int getFlags() noexcept;
}

#pragma once

namespace Config
{
    extern float staminaCost1H;
    extern float staminaCost2H;
    extern bool restrictCancelWindow;
    extern int cancelWindow1H;
    extern int cancelWindow2H;

    void Load();
}

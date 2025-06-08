#include "Config.h"
#include <SimpleIni.h>
#include "CancelAttackHandler.h"

float Config::staminaCost1H = 0.0f;
float Config::staminaCost2H = 0.0f;
bool Config::restrictCancelWindow = false;
int Config::cancelWindow1H = 0;
int Config::cancelWindow2H = 0;

void Config::Load()
{
    CSimpleIniW ini;
    ini.SetUnicode();
    ini.LoadFile(R"(Data\SKSE\Plugins\CancelAttackSKSE.ini)");

    staminaCost1H = std::clamp(static_cast<float>(ini.GetDoubleValue(L"General", L"StaminaCost1H", 0.0)), 0.0f, 100.0f);
    staminaCost2H = std::clamp(static_cast<float>(ini.GetDoubleValue(L"General", L"StaminaCost2H", 0.0)), 0.0f, 100.0f);
    restrictCancelWindow = ini.GetBoolValue(L"General", L"RestrictCancelWindow", false);
    cancelWindow1H = std::clamp(static_cast<int>(ini.GetLongValue(L"General", L"CancelWindow1H", 0)), 0, 1000);
    cancelWindow2H = std::clamp(static_cast<int>(ini.GetLongValue(L"General", L"CancelWindow2H", 0)), 0, 1000);
}

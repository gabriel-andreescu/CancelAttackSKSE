#include "Settings.h"

#include <SKSE/SKSE.h>

#include <BMK/Settings.h>

#include <CLIBUtil/simpleINI.hpp>
#include <spdlog/spdlog.h>

#include <cmath>
#include <utility>

namespace Settings {
namespace {
    Values current; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    void ReadGeneral(CSimpleIniA& a_ini, Values& a_values) {
        clib_util::ini::get_value(
            a_ini,
            a_values.debugLogging,
            "General",
            "bDebugLogging",
            nullptr,
            clib_util::ini::bool_format::kNumeric
        );
        auto binding = std::to_underlying(a_values.binding);
        clib_util::ini::get_value(a_ini, binding, "General", "iBinding");
        a_values.binding = static_cast<Binding>(binding);
        clib_util::ini::get_value(a_ini, a_values.customKey, "General", "iCustomKey");
        auto additionalBinding = std::to_underlying(a_values.additionalBinding);
        clib_util::ini::get_value(a_ini, additionalBinding, "General", "iAdditionalBinding");
        a_values.additionalBinding = static_cast<AdditionalBinding>(additionalBinding);
        clib_util::ini::get_value(a_ini, a_values.additionalCustomKey, "General", "iAdditionalCustomKey");
        auto behavior = std::to_underlying(a_values.behavior);
        clib_util::ini::get_value(a_ini, behavior, "General", "iBehavior");
        a_values.behavior = static_cast<Behavior>(behavior);
    }

    void ReadTimingAndStamina(CSimpleIniA& a_ini, Values& a_values) {
        clib_util::ini::get_value(a_ini, a_values.windowStart, "Timing", "fWindowStart");
        clib_util::ini::get_value(a_ini, a_values.windowEnd, "Timing", "fWindowEnd");
        clib_util::ini::get_value(a_ini, a_values.oneHandedStaminaCost, "Stamina", "fOneHandedCost");
        clib_util::ini::get_value(a_ini, a_values.twoHandedStaminaCost, "Stamina", "fTwoHandedCost");
        clib_util::ini::get_value(a_ini, a_values.unarmedStaminaCost, "Stamina", "fUnarmedCost");
    }

    void ReadWeapons(CSimpleIniA& a_ini, Values& a_values) {
        clib_util::ini::get_value(
            a_ini,
            a_values.enableDaggers,
            "Weapons",
            "bDaggers",
            nullptr,
            clib_util::ini::bool_format::kNumeric
        );
        clib_util::ini::get_value(
            a_ini,
            a_values.enableSwords,
            "Weapons",
            "bSwords",
            nullptr,
            clib_util::ini::bool_format::kNumeric
        );
        clib_util::ini::get_value(
            a_ini,
            a_values.enableWarAxes,
            "Weapons",
            "bWarAxes",
            nullptr,
            clib_util::ini::bool_format::kNumeric
        );
        clib_util::ini::get_value(
            a_ini,
            a_values.enableMaces,
            "Weapons",
            "bMaces",
            nullptr,
            clib_util::ini::bool_format::kNumeric
        );
        clib_util::ini::get_value(
            a_ini,
            a_values.enableGreatswords,
            "Weapons",
            "bGreatswords",
            nullptr,
            clib_util::ini::bool_format::kNumeric
        );
        clib_util::ini::get_value(
            a_ini,
            a_values.enableBattleaxesAndWarhammers,
            "Weapons",
            "bBattleaxesAndWarhammers",
            nullptr,
            clib_util::ini::bool_format::kNumeric
        );
        clib_util::ini::get_value(
            a_ini,
            a_values.enableDualWield,
            "Weapons",
            "bDualWield",
            nullptr,
            clib_util::ini::bool_format::kNumeric
        );
        clib_util::ini::get_value(
            a_ini,
            a_values.enableUnarmed,
            "Weapons",
            "bUnarmed",
            nullptr,
            clib_util::ini::bool_format::kNumeric
        );
    }

    void ReadValues(CSimpleIniA& a_ini, Values& a_values) {
        ReadGeneral(a_ini, a_values);
        ReadTimingAndStamina(a_ini, a_values);
        ReadWeapons(a_ini, a_values);
    }

    void WriteLong(CSimpleIniA& a_user, const char* a_section, const char* a_key, long a_value);
    void WriteFloat(CSimpleIniA& a_user, const char* a_section, const char* a_key, float a_value);

    void ValidateValues(CSimpleIniA& a_user, Values& a_values) {
        const auto binding = std::to_underlying(a_values.binding);
        if (binding < std::to_underlying(Binding::kBlock) || binding > std::to_underlying(Binding::kCustom)) {
            SKSE::log::warn("Settings: invalid General.iBinding={}, using 0", binding);
            a_values.binding = Binding::kBlock;
            WriteLong(a_user, "General", "iBinding", std::to_underlying(a_values.binding));
        }
        const auto additionalBinding = std::to_underlying(a_values.additionalBinding);
        if (additionalBinding
            < std::to_underlying(AdditionalBinding::kDisabled)
            || additionalBinding
            > std::to_underlying(AdditionalBinding::kCustom)) {
            SKSE::log::warn("Settings: invalid General.iAdditionalBinding={}, using 0", additionalBinding);
            a_values.additionalBinding = AdditionalBinding::kDisabled;
            WriteLong(a_user, "General", "iAdditionalBinding", std::to_underlying(a_values.additionalBinding));
        }
        const auto behavior = std::to_underlying(a_values.behavior);
        if (behavior
            < std::to_underlying(Behavior::kCancelAndBlock)
            || behavior
            > std::to_underlying(Behavior::kCancelOnly)) {
            SKSE::log::warn("Settings: invalid General.iBehavior={}, using 0", behavior);
            a_values.behavior = Behavior::kCancelAndBlock;
            WriteLong(a_user, "General", "iBehavior", std::to_underlying(a_values.behavior));
        }
        const auto customKeyValid = a_values.customKey
                                    == -1
                                    || (a_values.customKey
                                        >= SKSE::InputMap::kMacro_KeyboardOffset
                                        && a_values.customKey
                                        < SKSE::InputMap::kMaxMacros);
        if (!customKeyValid) {
            SKSE::log::warn("Settings: invalid General.iCustomKey={}, using -1", a_values.customKey);
            a_values.customKey = -1;
            WriteLong(a_user, "General", "iCustomKey", a_values.customKey);
        }
        const auto additionalCustomKeyValid = a_values.additionalCustomKey
                                              == -1
                                              || (a_values.additionalCustomKey
                                                  >= SKSE::InputMap::kMacro_KeyboardOffset
                                                  && a_values.additionalCustomKey
                                                  < SKSE::InputMap::kMaxMacros);
        if (!additionalCustomKeyValid) {
            SKSE::log::warn(
                "Settings: invalid General.iAdditionalCustomKey={}, using -1",
                a_values.additionalCustomKey
            );
            a_values.additionalCustomKey = -1;
            WriteLong(a_user, "General", "iAdditionalCustomKey", a_values.additionalCustomKey);
        }

        const auto sanitizeNonNegative = [&a_user](float& a_value, const char* a_section, const char* a_key) {
            if (!std::isfinite(a_value) || a_value < 0.0F) {
                SKSE::log::warn("Settings: invalid {}.{}={}, using 0", a_section, a_key, a_value);
                a_value = 0.0F;
                WriteFloat(a_user, a_section, a_key, a_value);
            }
        };
        sanitizeNonNegative(a_values.windowStart, "Timing", "fWindowStart");
        sanitizeNonNegative(a_values.windowEnd, "Timing", "fWindowEnd");
        sanitizeNonNegative(a_values.oneHandedStaminaCost, "Stamina", "fOneHandedCost");
        sanitizeNonNegative(a_values.twoHandedStaminaCost, "Stamina", "fTwoHandedCost");
        sanitizeNonNegative(a_values.unarmedStaminaCost, "Stamina", "fUnarmedCost");
        if (a_values.windowEnd > 0.0F && a_values.windowEnd < a_values.windowStart) {
            SKSE::log::warn(
                "Settings: Timing.fWindowEnd={} is before fWindowStart={}, using {}",
                a_values.windowEnd,
                a_values.windowStart,
                a_values.windowStart
            );
            a_values.windowEnd = a_values.windowStart;
            WriteFloat(a_user, "Timing", "fWindowEnd", a_values.windowEnd);
        }
    }

    void WriteLong(CSimpleIniA& a_user, const char* a_section, const char* a_key, const long a_value) {
        if (a_user.SetLongValue(a_section, a_key, a_value) < 0) {
            SKSE::log::error("Settings: cannot repair {}.{}", a_section, a_key);
        }
    }

    void WriteFloat(CSimpleIniA& a_user, const char* a_section, const char* a_key, const float a_value) {
        if (a_user.SetDoubleValue(a_section, a_key, a_value) < 0) {
            SKSE::log::error("Settings: cannot repair {}.{}", a_section, a_key);
        }
    }

}

const Values& Get() {
    return current;
}

void Reload() {
    const auto initialValues = Values {};
    auto loaded = BMK::Settings::Load(
        {
            .defaults = L"Data/MCM/Config/CancelAttackSKSE/settings.ini",
            .user = L"Data/MCM/Settings/CancelAttackSKSE.ini",
        },
        initialValues,
        [](CSimpleIniA& a_defaults, CSimpleIniA& a_user, Values& a_candidate) {
            ReadValues(a_defaults, a_candidate);
            ReadValues(a_user, a_candidate);
            ValidateValues(a_user, a_candidate);
        }
    );
    if (!loaded) {
        SKSE::log::warn("Cannot load settings: {}", loaded.error().message);
        return;
    }
    if (loaded->saveFailure) {
        SKSE::log::warn("Cannot save settings: {}", loaded->saveFailure->message);
    }

    current = loaded->values;
    const auto defaultLogLevel = SKSE::InitInfo {}.logLevel;
    BMK::Settings::ApplyLogLevel(current.debugLogging, defaultLogLevel);
    SKSE::log::info(
        "Settings loaded: binding={} customKey={} additionalBinding={} additionalCustomKey={} behavior={} "
        "window={:.3f}-{:.3f} costs={:.1f}/{:.1f}/{:.1f}",
        std::to_underlying(current.binding),
        current.customKey,
        std::to_underlying(current.additionalBinding),
        current.additionalCustomKey,
        std::to_underlying(current.behavior),
        current.windowStart,
        current.windowEnd,
        current.oneHandedStaminaCost,
        current.twoHandedStaminaCost,
        current.unarmedStaminaCost
    );
}
}

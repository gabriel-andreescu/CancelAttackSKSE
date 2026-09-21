#pragma once

#include <cstdint>

namespace Settings {
enum class Binding : std::int32_t {
    kBlock = 0,
    kReadyWeapon = 1,
    kCustom = 2,
};

enum class AdditionalBinding : std::int32_t {
    kDisabled = 0,
    kBlock = 1,
    kReadyWeapon = 2,
    kCustom = 3,
};

enum class Behavior : std::int32_t {
    kCancelAndBlock = 0,
    kCancelOnly = 1,
};

struct Values {
    bool debugLogging = false;
    Binding binding = Binding::kBlock;
    std::int32_t customKey = -1;
    AdditionalBinding additionalBinding = AdditionalBinding::kDisabled;
    std::int32_t additionalCustomKey = -1;
    Behavior behavior = Behavior::kCancelAndBlock;
    float windowStart = 0.0F;
    float windowEnd = 0.0F;
    float oneHandedStaminaCost = 0.0F;
    float twoHandedStaminaCost = 0.0F;
    float unarmedStaminaCost = 0.0F;
    bool enableDaggers = true;
    bool enableSwords = true;
    bool enableWarAxes = true;
    bool enableMaces = true;
    bool enableGreatswords = true;
    bool enableBattleaxesAndWarhammers = true;
    bool enableDualWield = true;
    bool enableUnarmed = true;
};

const Values& Get();
void Reload();
}

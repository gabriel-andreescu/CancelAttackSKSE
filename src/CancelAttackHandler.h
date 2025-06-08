#pragma once

#include "RE/Skyrim.h"
#include <chrono>

class CancelAttackHandler final : public RE::BSTEventSink<RE::InputEvent*>
{
public:
    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event,
        RE::BSTEventSource<RE::InputEvent*>*) override;

private:
    static bool HasTwoHandedWeaponEquipped(const RE::PlayerCharacter* player);
    static bool IsRestrictedCancelWindow(const RE::PlayerCharacter* player);
    static void CancelAttack(RE::PlayerCharacter* player);
    static float GetStaminaCost(const RE::PlayerCharacter* player);
    static bool TryConsumeStamina(RE::PlayerCharacter* player);
    inline static std::chrono::steady_clock::time_point lastAttackStartTime{};
    inline static bool wasAttacking = false;
};

#pragma once

#include "RE/Skyrim.h"
#include <chrono>

class CancelAttackHandler final : public RE::BSTEventSink<RE::InputEvent*>
{
public:
    static CancelAttackHandler* GetSingleton();
    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event,
        RE::BSTEventSource<RE::InputEvent*>*) override;
    void UpdateBlockMappings();

private:
    static void CancelAttack(RE::PlayerCharacter* player);
    static bool TryConsumeStamina(RE::PlayerCharacter* player);
    static float GetStaminaCost(const RE::PlayerCharacter* player);
    static bool HasTwoHandedWeaponEquipped(const RE::PlayerCharacter* player);
    bool IsRestrictedCancelWindow(const RE::PlayerCharacter* player) const;
    [[nodiscard]] bool IsBlockInputKey(RE::INPUT_DEVICE device, std::uint32_t key) const;
    void UpdateAttackState(const RE::PlayerCharacter* player);

    std::chrono::steady_clock::time_point lastAttackStartTime{};
    bool wasAttacking = false;

    std::uint32_t blockMappingKeyboard = 255;
    std::uint32_t blockMappingMouse = 255;
    std::uint32_t blockMappingGamepad = 255;
    bool blockMapped = false;
};

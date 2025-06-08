#include "CancelAttackHandler.h"
#include "Config.h"

CancelAttackHandler* CancelAttackHandler::GetSingleton()
{
    static CancelAttackHandler instance;
    return &instance;
}

RE::BSEventNotifyControl CancelAttackHandler::ProcessEvent(
    RE::InputEvent* const* a_event,
    RE::BSTEventSource<RE::InputEvent*>*)
{
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player || !a_event || !*a_event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    UpdateAttackState(player);
    if (!player->IsAttacking()) {
        return RE::BSEventNotifyControl::kContinue;
    }

    const auto* rightHand = player->GetEquippedObject(false);
    const auto* rightWeapon = rightHand ? rightHand->As<RE::TESObjectWEAP>() : nullptr;
    if (!rightWeapon || !rightWeapon->IsMelee() || rightWeapon->IsHandToHandMelee()) {
        return RE::BSEventNotifyControl::kContinue;
    }

    if (!blockMapped) {
        UpdateBlockMappings();
    }

    for (auto input = *a_event; input; input = input->next) {
        auto* button = input->AsButtonEvent();
        if (!button || !button->IsPressed() || button->HeldDuration() > 0.0f) {
            continue;
        }

        if (!IsBlockInputKey(button->GetDevice(), button->GetIDCode())) {
            continue;
        }

        if (IsRestrictedCancelWindow(player) || !TryConsumeStamina(player)) {
            return RE::BSEventNotifyControl::kContinue;
        }

        CancelAttack(player);
    }

    return RE::BSEventNotifyControl::kContinue;
}

void CancelAttackHandler::UpdateAttackState(const RE::PlayerCharacter* player)
{
    const bool isAttacking = player->IsAttacking();
    if (isAttacking && !wasAttacking) {
        lastAttackStartTime = std::chrono::steady_clock::now();
    }
    wasAttacking = isAttacking;
}

void CancelAttackHandler::UpdateBlockMappings()
{
    auto* controlMap = RE::ControlMap::GetSingleton();
    const auto* userEvents = RE::UserEvents::GetSingleton();

    blockMappingKeyboard = controlMap->GetMappedKey(userEvents->leftAttack, RE::INPUT_DEVICE::kKeyboard);
    blockMappingMouse = controlMap->GetMappedKey(userEvents->leftAttack, RE::INPUT_DEVICE::kMouse);
    blockMappingGamepad = controlMap->GetMappedKey(userEvents->leftAttack, RE::INPUT_DEVICE::kGamepad);

    blockMapped = true;
}

bool CancelAttackHandler::IsBlockInputKey(const RE::INPUT_DEVICE device, const std::uint32_t key) const
{
    switch (device) {
        case RE::INPUT_DEVICE::kKeyboard:
            return key == blockMappingKeyboard;
        case RE::INPUT_DEVICE::kMouse:
            return key == blockMappingMouse;
        case RE::INPUT_DEVICE::kGamepad:
            return key == blockMappingGamepad;
        default:
            return false;
    }
}

bool CancelAttackHandler::HasTwoHandedWeaponEquipped(const RE::PlayerCharacter* player)
{
    const auto* rightHand = player->GetEquippedObject(false);
    const auto* rightWeapon = rightHand ? rightHand->As<RE::TESObjectWEAP>() : nullptr;
    return rightWeapon && (rightWeapon->IsTwoHandedAxe() || rightWeapon->IsTwoHandedSword());
}

bool CancelAttackHandler::IsRestrictedCancelWindow(const RE::PlayerCharacter* player) const
{
    if (!Config::restrictCancelWindow || (Config::cancelWindow1H <= 0 && Config::cancelWindow2H <= 0)) {
        return false;
    }

    if (lastAttackStartTime.time_since_epoch().count() == 0) {
        return true;
    }

    int cancelWindowMs = HasTwoHandedWeaponEquipped(player) ? Config::cancelWindow2H : Config::cancelWindow1H;
    return std::chrono::steady_clock::now() - lastAttackStartTime > std::chrono::milliseconds(cancelWindowMs);
}

void CancelAttackHandler::CancelAttack(RE::PlayerCharacter* player)
{
    player->NotifyAnimationGraph("attackStop");

    const auto* left = player->GetEquippedObject(true);
    const auto* shield = left ? left->As<RE::TESObjectARMO>() : nullptr;

    if (shield && shield->IsShield()) {
        player->NotifyAnimationGraph("blockStart");
    }
}

float CancelAttackHandler::GetStaminaCost(const RE::PlayerCharacter* player)
{
    return HasTwoHandedWeaponEquipped(player) ? Config::staminaCost2H : Config::staminaCost1H;
}

bool CancelAttackHandler::TryConsumeStamina(RE::PlayerCharacter* player)
{
    const float cost = GetStaminaCost(player);
    if (cost <= 0.0f) {
        return true;
    }

    if (player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kStamina) >= cost) {
        player->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kStamina, -cost);
        return true;
    }

    return false;
}

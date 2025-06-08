#include "CancelAttackHandler.h"
#include "Config.h"

RE::BSEventNotifyControl CancelAttackHandler::ProcessEvent(
    RE::InputEvent* const* a_event,
    RE::BSTEventSource<RE::InputEvent*>*)
{
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (!player || !a_event || !*a_event) {
        return RE::BSEventNotifyControl::kContinue;
    }

    bool isAttacking = player->IsAttacking();

    if (isAttacking && !wasAttacking) {
        lastAttackStartTime = std::chrono::steady_clock::now();
    }
    wasAttacking = isAttacking;

    if (!isAttacking) {
        return RE::BSEventNotifyControl::kContinue;
    }

    const auto* rightHand = player->GetEquippedObject(false);
    const auto* rightWeapon = rightHand ? rightHand->As<RE::TESObjectWEAP>() : nullptr;
    if (!rightWeapon || !rightWeapon->IsMelee() || rightWeapon->IsHandToHandMelee()) {
        return RE::BSEventNotifyControl::kContinue;
    }

    for (auto input = *a_event; input; input = input->next) {
        if (input->eventType != RE::INPUT_EVENT_TYPE::kButton)
            continue;

        auto* button = input->AsButtonEvent();
        if (!button || !button->IsPressed() || button->HeldDuration() > 0.0f)
            continue;

        const auto key = button->GetIDCode();
        const auto device = button->GetDevice();

        const bool isBlockInput =
            (device == RE::INPUT_DEVICE::kMouse && key == 0x01) || (device == RE::INPUT_DEVICE::kGamepad && key == 0x0F);

        if (!isBlockInput)
            continue;

        if (IsRestrictedCancelWindow(player)) {
            return RE::BSEventNotifyControl::kContinue;
        }

        if (!TryConsumeStamina(player)) {
            return RE::BSEventNotifyControl::kContinue;
        }

        CancelAttack(player);
    }

    return RE::BSEventNotifyControl::kContinue;
}

bool CancelAttackHandler::HasTwoHandedWeaponEquipped(const RE::PlayerCharacter* player)
{
    const auto* rightHand = player->GetEquippedObject(false);
    const auto* rightWeapon = rightHand ? rightHand->As<RE::TESObjectWEAP>() : nullptr;
    return rightWeapon && (rightWeapon->IsTwoHandedAxe() || rightWeapon->IsTwoHandedSword());
}

bool CancelAttackHandler::IsRestrictedCancelWindow(const RE::PlayerCharacter* player)
{
    if (!Config::restrictCancelWindow || (Config::cancelWindow1H <= 0 && Config::cancelWindow2H <= 0)) {
        return false;
    }

    if (lastAttackStartTime.time_since_epoch().count() == 0) {
        return true;
    }

    int cancelWindowMs = Config::cancelWindow1H;

    if (HasTwoHandedWeaponEquipped(player)) {
        cancelWindowMs = Config::cancelWindow2H;
    }

    auto now = std::chrono::steady_clock::now();
    return now - lastAttackStartTime > std::chrono::milliseconds(cancelWindowMs);
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
    if (Config::staminaCost1H <= 0.0f && Config::staminaCost2H <= 0.0f) {
        return 0.0f;
    }

    if (HasTwoHandedWeaponEquipped(player)) {
        return Config::staminaCost2H;
    }

    return Config::staminaCost1H;
}

bool CancelAttackHandler::TryConsumeStamina(RE::PlayerCharacter* player)
{
    float staminaCost = GetStaminaCost(player);
    if (staminaCost <= 0.0f) {
        return true;
    }

    float currentStamina = player->AsActorValueOwner()->GetActorValue(RE::ActorValue::kStamina);
    if (currentStamina >= staminaCost) {
        player->AsActorValueOwner()->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kStamina, -staminaCost);
        return true;
    }

    return false;
}

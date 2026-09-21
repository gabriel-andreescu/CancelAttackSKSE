#include "Combat.h"

#include "Settings.h"

#include <SKSE/InputMap.h>
#include <SKSE/SKSE.h>

#include <algorithm>
#include <utility>

namespace {
enum class WeaponClass : std::uint8_t {
    kNone,
    kDagger,
    kSword,
    kWarAxe,
    kMace,
    kGreatsword,
    kBattleaxe,
    kUnsupported,
};

[[nodiscard]] bool IsOneHanded(const RE::TESObjectWEAP& a_weapon) {
    return a_weapon.IsOneHandedDagger()
           || a_weapon.IsOneHandedSword()
           || a_weapon.IsOneHandedAxe()
           || a_weapon.IsOneHandedMace();
}

[[nodiscard]] WeaponClass ClassifyWeapon(const RE::TESForm* a_form, const RE::TESObjectWEAP* a_weapon) {
    if (a_form == nullptr) {
        return WeaponClass::kNone;
    }
    if (a_weapon == nullptr) {
        return WeaponClass::kUnsupported;
    }
    if (a_weapon->IsOneHandedDagger()) {
        return WeaponClass::kDagger;
    }
    if (a_weapon->IsOneHandedSword()) {
        return WeaponClass::kSword;
    }
    if (a_weapon->IsOneHandedAxe()) {
        return WeaponClass::kWarAxe;
    }
    if (a_weapon->IsOneHandedMace()) {
        return WeaponClass::kMace;
    }
    if (a_weapon->IsTwoHandedSword()) {
        return WeaponClass::kGreatsword;
    }
    if (a_weapon->IsTwoHandedAxe()) {
        return WeaponClass::kBattleaxe;
    }
    return WeaponClass::kUnsupported;
}

[[nodiscard]] WeaponClass ClassifyAttackWeapon(
    const RE::PlayerCharacter& a_player,
    const RE::TESForm* a_right,
    const RE::TESForm* a_left
) {
    if (a_right == nullptr && a_left == nullptr) {
        return WeaponClass::kNone;
    }

    const auto* attackingEntry = a_player.GetAttackingWeapon();
    const auto* attackingForm = attackingEntry != nullptr ? attackingEntry->object : nullptr;
    const auto* attackingWeapon = attackingForm != nullptr ? attackingForm->As<RE::TESObjectWEAP>() : nullptr;
    return ClassifyWeapon(attackingForm, attackingWeapon);
}

[[nodiscard]] bool IsEnabled(const WeaponClass a_weapon, const Settings::Values& a_settings) {
    switch (a_weapon) {
        case WeaponClass::kNone:        return a_settings.enableUnarmed;
        case WeaponClass::kDagger:      return a_settings.enableDaggers;
        case WeaponClass::kSword:       return a_settings.enableSwords;
        case WeaponClass::kWarAxe:      return a_settings.enableWarAxes;
        case WeaponClass::kMace:        return a_settings.enableMaces;
        case WeaponClass::kGreatsword:  return a_settings.enableGreatswords;
        case WeaponClass::kBattleaxe:   return a_settings.enableBattleaxesAndWarhammers;
        case WeaponClass::kUnsupported: return false;
    }
    return false;
}

[[nodiscard]] bool IsDualWielding(const RE::TESObjectWEAP* a_right, const RE::TESForm* a_left) {
    const auto* leftWeapon = a_left != nullptr ? a_left->As<RE::TESObjectWEAP>() : nullptr;
    if (a_right == nullptr || leftWeapon == nullptr) {
        return false;
    }
    return IsOneHanded(*a_right) && IsOneHanded(*leftWeapon);
}

[[nodiscard]] bool MappedBlockConflictsWithLeftHand(const RE::TESObjectWEAP* a_right, const RE::TESForm* a_left) {
    if (a_right != nullptr && (a_right->IsTwoHandedSword() || a_right->IsTwoHandedAxe())) {
        return false;
    }
    if (a_left == nullptr) {
        return a_right == nullptr;
    }
    if (a_left->As<RE::SpellItem>() != nullptr) {
        return true;
    }
    const auto* leftWeapon = a_left->As<RE::TESObjectWEAP>();
    return leftWeapon != nullptr && (leftWeapon->IsStaff() || IsOneHanded(*leftWeapon));
}

[[nodiscard]] bool SupportsVanillaBlock(const RE::TESObjectWEAP* a_right, const RE::TESForm* a_left) {
    if (a_right == nullptr) {
        return false;
    }
    if (a_right->IsTwoHandedSword() || a_right->IsTwoHandedAxe()) {
        return true;
    }
    if (!IsOneHanded(*a_right)) {
        return false;
    }
    if (a_left == nullptr) {
        return true;
    }
    const auto* armor = a_left->As<RE::TESObjectARMO>();
    return armor != nullptr && armor->IsShield();
}

[[nodiscard]] float StaminaCost(const WeaponClass a_weapon, const Settings::Values& a_settings) {
    switch (a_weapon) {
        case WeaponClass::kNone:       return a_settings.unarmedStaminaCost;
        case WeaponClass::kGreatsword:
        case WeaponClass::kBattleaxe:  return a_settings.twoHandedStaminaCost;
        default:                       return a_settings.oneHandedStaminaCost;
    }
}

[[nodiscard]] bool InCancelWindow(const float a_elapsed, const Settings::Values& a_settings) {
    if (a_elapsed < a_settings.windowStart) {
        return false;
    }
    return a_settings.windowEnd <= 0.0F || a_elapsed <= a_settings.windowEnd;
}

[[nodiscard]] bool MatchesCustomKey(const RE::ButtonEvent& a_button, const std::int32_t a_key) {
    if (a_key < 0) {
        return false;
    }

    std::uint32_t macro = SKSE::InputMap::kMaxMacros;
    switch (a_button.GetDevice()) {
        case RE::INPUT_DEVICE::kKeyboard: macro = a_button.GetIDCode(); break;
        case RE::INPUT_DEVICE::kMouse:   macro = SKSE::InputMap::kMacro_MouseButtonOffset + a_button.GetIDCode(); break;
        case RE::INPUT_DEVICE::kGamepad: macro = SKSE::InputMap::GamepadMaskToKeycode(a_button.GetIDCode()); break;
        default:
            if (!REL::Module::IsVR()
                || a_button.GetDevice()
                < RE::INPUT_DEVICE::kVivePrimary
                || a_button.GetDevice()
                > RE::INPUT_DEVICE::kWMRSecondary) {
                return false;
            }
            macro = a_button.GetIDCode();
            break;
    }
    return std::cmp_equal(macro, a_key);
}

[[nodiscard]] bool MatchesBinding(
    const RE::ButtonEvent& a_button,
    const Settings::Binding a_binding,
    const std::int32_t a_customKey
) {
    const auto* userEvents = RE::UserEvents::GetSingleton();
    if (userEvents == nullptr) {
        return false;
    }

    switch (a_binding) {
        case Settings::Binding::kBlock:       return a_button.QUserEvent() == userEvents->leftAttack;
        case Settings::Binding::kReadyWeapon: return a_button.QUserEvent() == userEvents->readyWeapon;
        case Settings::Binding::kCustom:      return MatchesCustomKey(a_button, a_customKey);
    }
    return false;
}

[[nodiscard]] bool MatchesAdditionalBinding(
    const RE::ButtonEvent& a_button,
    const Settings::AdditionalBinding a_binding,
    const std::int32_t a_customKey
) {
    const auto* userEvents = RE::UserEvents::GetSingleton();
    if (userEvents == nullptr) {
        return false;
    }

    switch (a_binding) {
        case Settings::AdditionalBinding::kDisabled:    return false;
        case Settings::AdditionalBinding::kBlock:       return a_button.QUserEvent() == userEvents->leftAttack;
        case Settings::AdditionalBinding::kReadyWeapon: return a_button.QUserEvent() == userEvents->readyWeapon;
        case Settings::AdditionalBinding::kCustom:      return MatchesCustomKey(a_button, a_customKey);
    }
    return false;
}

[[nodiscard]] bool MatchesAnyBinding(const RE::ButtonEvent& a_button, const Settings::Values& a_settings) {
    return MatchesBinding(a_button, a_settings.binding, a_settings.customKey)
           || MatchesAdditionalBinding(a_button, a_settings.additionalBinding, a_settings.additionalCustomKey);
}

[[nodiscard]] bool IsRightAttack(const RE::ButtonEvent& a_button) {
    const auto* userEvents = RE::UserEvents::GetSingleton();
    return userEvents != nullptr && a_button.QUserEvent() == userEvents->rightAttack;
}

[[nodiscard]] bool IsMappedBlock(const RE::ButtonEvent& a_button) {
    const auto* userEvents = RE::UserEvents::GetSingleton();
    return userEvents != nullptr && a_button.QUserEvent() == userEvents->leftAttack;
}

[[nodiscard]] bool IsGameplayInputEnabled() {
    auto* userInterface = RE::UI::GetSingleton();
    const auto* controls = RE::ControlMap::GetSingleton();
    if (userInterface == nullptr || controls == nullptr) {
        return false;
    }
    return !userInterface->GameIsPaused() && controls->IsFightingControlsEnabled();
}
}

void Combat::RegisterInput() {
    auto* self = GetSingleton();
    auto* input = RE::BSInputDeviceManager::GetSingleton();
    if (input == nullptr) {
        SKSE::log::critical("Combat input registration failed");
        return;
    }
    if (!self->inputRegistered_) {
        input->PrependEventSink(static_cast<RE::BSInputDeviceManager::Sink*>(self));
        self->inputRegistered_ = true;
        SKSE::log::info("Combat input registered");
    }
}

void Combat::RegisterLifecycleEvents() {
    auto* self = GetSingleton();
    if (!self->menuEventsRegistered_) {
        if (auto* userInterface = RE::UI::GetSingleton(); userInterface != nullptr) {
            userInterface->AddEventSink<RE::MenuOpenCloseEvent>(self);
            self->menuEventsRegistered_ = true;
            SKSE::log::info("Combat lifecycle events registered");
        }
    }
}

void Combat::PrepareForLoad() {
    auto* self = GetSingleton();
    self->Reset(false);
}

bool Combat::IsActiveMeleeAttack(const RE::ATTACK_STATE_ENUM a_state) {
    return a_state >= RE::ATTACK_STATE_ENUM::kDraw && a_state <= RE::ATTACK_STATE_ENUM::kFollowThrough;
}

bool Combat::IsCancellableAttack(const RE::ATTACK_STATE_ENUM a_state) {
    return a_state >= RE::ATTACK_STATE_ENUM::kDraw && a_state <= RE::ATTACK_STATE_ENUM::kNextAttack;
}

bool Combat::ShouldSuppressReadyWeapon(const RE::InputEvent* a_event) {
    if (a_event == nullptr) {
        return false;
    }
    const auto& settings = Settings::Get();
    const auto* userEvents = RE::UserEvents::GetSingleton();
    const auto* player = RE::PlayerCharacter::GetSingleton();
    if (userEvents == nullptr || player == nullptr) {
        return false;
    }
    const auto* button = a_event->AsButtonEvent();
    const auto isReadyEvent = a_event->QUserEvent()
                              == userEvents->readyWeapon
                              && ((settings.binding == Settings::Binding::kReadyWeapon)
                                  || (settings.additionalBinding == Settings::AdditionalBinding::kReadyWeapon)
                                  || (button != nullptr && MatchesAnyBinding(*button, settings)));
    const auto activeMeleeAttack = IsActiveMeleeAttack(player->AsActorState()->GetAttackState());
    return isReadyEvent && activeMeleeAttack;
}

void Combat::Update(RE::PlayerCharacter& a_player, const float a_delta) {
    if (auto* userInterface = RE::UI::GetSingleton(); userInterface != nullptr && userInterface->GameIsPaused()) {
        return;
    }

    const auto state = a_player.AsActorState()->GetAttackState();

    if (a_player.IsDead()) {
        Reset(true);
        return;
    }

    if ((ownsBlock_ || acceptedBlockPending_)) {
        const auto* rightForm = a_player.GetEquippedObject(false);
        const auto* leftForm = a_player.GetEquippedObject(true);
        const auto* rightWeapon = rightForm != nullptr ? rightForm->As<RE::TESObjectWEAP>() : nullptr;
        if (!SupportsVanillaBlock(rightWeapon, leftForm)) {
            acceptedBlockPending_ = false;
            heldBlockKey_.reset();
            EndOwnedBlock(a_player);
        }
    }

    if (state == RE::ATTACK_STATE_ENUM::kDraw && lastAttackState_ != RE::ATTACK_STATE_ENUM::kDraw) {
        attackElapsed_ = 0.0F;
        timingActive_ = true;
    } else if (IsActiveMeleeAttack(state)) {
        if (timingActive_) {
            attackElapsed_ += std::max(a_delta, 0.0F);
        }
    } else {
        attackElapsed_ = 0.0F;
        timingActive_ = false;
    }

    if (acceptedBlockPending_ && heldBlockKey_ && !IsActiveMeleeAttack(state)) {
        BeginBlock(a_player);
    }

    if (state != lastAttackState_) {
        SKSE::log::debug(
            "Attack state {} -> {} at {:.3f}s",
            std::to_underlying(lastAttackState_),
            std::to_underlying(state),
            attackElapsed_
        );
    }
    lastAttackState_ = state;
}

void Combat::Reset(const bool a_stopOwnedBlock) {
    ResetBlockState(a_stopOwnedBlock);
    attackElapsed_ = 0.0F;
    lastAttackState_ = RE::ATTACK_STATE_ENUM::kNone;
    timingActive_ = false;
}

void Combat::ReconcileSettings() {
    ResetBlockState(true);
}

void Combat::ResetBlockState(const bool a_stopOwnedBlock) {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (a_stopOwnedBlock && player != nullptr) {
        EndOwnedBlock(*player);
    }
    heldBlockKey_.reset();
    ownedBlockKey_.reset();
    vanillaBlockKey_.reset();
    acceptedBlockPending_ = false;
    ownsBlock_ = false;
}

RE::BSEventNotifyControl Combat::ProcessEvent(
    RE::InputEvent* const* a_events,
    [[maybe_unused]] RE::BSTEventSource<RE::InputEvent*>* a_eventSource
) {
    auto* player = RE::PlayerCharacter::GetSingleton();
    if (a_events == nullptr || player == nullptr) {
        return RE::BSEventNotifyControl::kContinue;
    }

    for (auto* event = *a_events; event != nullptr; event = event->next) {
        if (auto* button = event->AsButtonEvent()) {
            HandleButton(*button, *player);
        }
    }
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl Combat::ProcessEvent(
    const RE::MenuOpenCloseEvent* a_event,
    [[maybe_unused]] RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource
) {
    if (a_event != nullptr && a_event->opening) {
        auto* userInterface = RE::UI::GetSingleton();
        if (userInterface != nullptr && userInterface->GameIsPaused()) {
            ResetBlockState(true);
        }
    }
    return RE::BSEventNotifyControl::kContinue;
}

void Combat::HandleButton(RE::ButtonEvent& a_button, RE::PlayerCharacter& a_player) {
    const PhysicalKey key {a_button.GetDevice(), a_button.GetIDCode()};

    if (IsMappedBlock(a_button)) {
        if (a_button.IsDown()) {
            vanillaBlockKey_ = key;
        } else if (a_button.IsUp() && vanillaBlockKey_ && key == *vanillaBlockKey_) {
            vanillaBlockKey_.reset();
        }
    }

    if (a_button.IsUp() && heldBlockKey_ && key == *heldBlockKey_) {
        heldBlockKey_.reset();
        acceptedBlockPending_ = false;
        EndOwnedBlock(a_player);
    }

    if (a_button.IsDown() && IsRightAttack(a_button)) {
        acceptedBlockPending_ = false;
        EndOwnedBlock(a_player);
        heldBlockKey_.reset();
    }

    if (a_button.IsDown() && IsGameplayInputEnabled() && MatchesAnyBinding(a_button, Settings::Get())) {
        TryCancel(a_button, a_player);
    }
}

void Combat::TryCancel(RE::ButtonEvent& a_button, RE::PlayerCharacter& a_player) {
    const auto state = a_player.AsActorState()->GetAttackState();
    if (!IsCancellableAttack(state)) {
        return;
    }

    const auto& settings = Settings::Get();
    const auto timingConstrained = settings.windowStart > 0.0F || settings.windowEnd > 0.0F;
    if (timingConstrained && !timingActive_) {
        SKSE::log::debug("Cancellation denied because this attack has no observed start");
        return;
    }
    if (timingConstrained && !InCancelWindow(attackElapsed_, settings)) {
        SKSE::log::debug("Cancellation denied by timing at {:.3f}s", attackElapsed_);
        return;
    }

    const auto* rightForm = a_player.GetEquippedObject(false);
    const auto* leftForm = a_player.GetEquippedObject(true);
    const auto* rightWeapon = rightForm != nullptr ? rightForm->As<RE::TESObjectWEAP>() : nullptr;
    const auto weaponClass = ClassifyAttackWeapon(a_player, rightForm, leftForm);
    if (!IsEnabled(weaponClass, settings)) {
        return;
    }
    if (IsDualWielding(rightWeapon, leftForm) && !settings.enableDualWield) {
        return;
    }
    if (IsMappedBlock(a_button) && MappedBlockConflictsWithLeftHand(rightWeapon, leftForm)) {
        SKSE::log::debug("Mapped Block left-hand action preserved");
        return;
    }

    const auto cost = StaminaCost(weaponClass, settings);
    const auto stamina = a_player.AsActorValueOwner()->GetActorValue(RE::ActorValue::kStamina);
    if (!RE::PlayerCharacter::IsGodMode() && stamina < cost) {
        RE::HUDMenu::FlashMeter(RE::ActorValue::kStamina);
        SKSE::log::debug("Cancellation denied by stamina: need {:.1f}", cost);
        return;
    }

    const auto startBlock = settings.behavior
                            == Settings::Behavior::kCancelAndBlock
                            && SupportsVanillaBlock(rightWeapon, leftForm);
    const PhysicalKey key {a_button.GetDevice(), a_button.GetIDCode()};
    if (startBlock) {
        heldBlockKey_ = key;
    }

    if (!a_player.NotifyAnimationGraph("attackStop")) {
        SKSE::log::debug("Cancellation request rejected by animation graph");
        heldBlockKey_.reset();
        return;
    }
    CompleteCancellation(a_player, RE::PlayerCharacter::IsGodMode() ? 0.0F : cost, startBlock);
    SKSE::log::debug("Cancellation accepted in attack state {}", std::to_underlying(state));
}

void Combat::CompleteCancellation(RE::PlayerCharacter& a_player, const float a_staminaCost, const bool a_startBlock) {
    if (a_staminaCost > 0.0F) {
        a_player.AsActorValueOwner()->DamageActorValue(RE::ActorValue::kStamina, a_staminaCost);
    }
    SKSE::log::debug("Cancellation charged stamina cost {:.1f}", a_staminaCost);

    if (a_startBlock && heldBlockKey_) {
        acceptedBlockPending_ = true;
        if (!IsActiveMeleeAttack(a_player.AsActorState()->GetAttackState())) {
            BeginBlock(a_player);
        }
    }
}

void Combat::BeginBlock(RE::PlayerCharacter& a_player) {
    if (!acceptedBlockPending_ || !heldBlockKey_) {
        return;
    }
    acceptedBlockPending_ = false;
    const auto sameVanillaKey = vanillaBlockKey_ && heldBlockKey_ && *vanillaBlockKey_ == *heldBlockKey_;
    if (!sameVanillaKey
        && (a_player.IsBlocking() || a_player.AsActorState()->actorState2.wantBlocking != 0 || vanillaBlockKey_)) {
        heldBlockKey_.reset();
        return;
    }
    a_player.AsActorState()->actorState2.wantBlocking = 1;
    if (!a_player.IsBlocking()) {
        a_player.NotifyAnimationGraph("blockStart");
    }
    ownsBlock_ = true;
    ownedBlockKey_ = heldBlockKey_;
    SKSE::log::debug("Owned block started");
}

void Combat::EndOwnedBlock(RE::PlayerCharacter& a_player) {
    if (!ownsBlock_) {
        return;
    }
    const auto otherVanillaKey = vanillaBlockKey_ && (!ownedBlockKey_ || *vanillaBlockKey_ != *ownedBlockKey_);
    if (!otherVanillaKey) {
        a_player.AsActorState()->actorState2.wantBlocking = 0;
        if (a_player.IsBlocking()) {
            a_player.NotifyAnimationGraph("blockStop");
        }
    }
    ownsBlock_ = false;
    ownedBlockKey_.reset();
    SKSE::log::debug("Owned block stopped");
}

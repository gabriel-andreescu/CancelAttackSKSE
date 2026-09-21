#pragma once

#include <RE/Skyrim.h>

#include <REX/REX/Singleton.h>

#include <cstdint>
#include <optional>

class Combat final :
    public REX::Singleton<Combat>,
    public RE::BSInputDeviceManager::Sink,
    public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
public:
    static void RegisterInput();
    static void RegisterLifecycleEvents();
    static void PrepareForLoad();

    void Update(RE::PlayerCharacter& a_player, float a_delta);
    void Reset(bool a_stopOwnedBlock);
    void ReconcileSettings();
    [[nodiscard]] static bool ShouldSuppressReadyWeapon(const RE::InputEvent* a_event);

protected:
    RE::BSEventNotifyControl ProcessEvent(
        RE::InputEvent* const* a_events,
        RE::BSTEventSource<RE::InputEvent*>* a_eventSource
    ) override;
    RE::BSEventNotifyControl ProcessEvent(
        const RE::MenuOpenCloseEvent* a_event,
        RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource
    ) override;

private:
    class PhysicalKey {
    public:
        PhysicalKey(RE::INPUT_DEVICE a_device, std::uint32_t a_id)
            : device_(a_device)
            , id_(a_id) {}

        bool operator==(const PhysicalKey&) const = default;

    private:
        RE::INPUT_DEVICE device_;
        std::uint32_t id_;
    };

    void HandleButton(RE::ButtonEvent& a_button, RE::PlayerCharacter& a_player);
    void TryCancel(RE::ButtonEvent& a_button, RE::PlayerCharacter& a_player);
    void CompleteCancellation(RE::PlayerCharacter& a_player, float a_staminaCost, bool a_startBlock);
    void BeginBlock(RE::PlayerCharacter& a_player);
    void EndOwnedBlock(RE::PlayerCharacter& a_player);
    void ResetBlockState(bool a_stopOwnedBlock);

    [[nodiscard]] static bool IsActiveMeleeAttack(RE::ATTACK_STATE_ENUM a_state);
    [[nodiscard]] static bool IsCancellableAttack(RE::ATTACK_STATE_ENUM a_state);

    float attackElapsed_ = 0.0F;
    RE::ATTACK_STATE_ENUM lastAttackState_ = RE::ATTACK_STATE_ENUM::kNone;
    std::optional<PhysicalKey> heldBlockKey_;
    std::optional<PhysicalKey> ownedBlockKey_;
    std::optional<PhysicalKey> vanillaBlockKey_;
    bool acceptedBlockPending_ = false;
    bool ownsBlock_ = false;
    bool timingActive_ = false;
    bool inputRegistered_ = false;
    bool menuEventsRegistered_ = false;
};

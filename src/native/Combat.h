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
            : _device(a_device)
            , _id(a_id) {}

        bool operator==(const PhysicalKey&) const = default;

    private:
        RE::INPUT_DEVICE _device;
        std::uint32_t _id;
    };

    void HandleButton(RE::ButtonEvent& a_button, RE::PlayerCharacter& a_player);
    void TryCancel(RE::ButtonEvent& a_button, RE::PlayerCharacter& a_player);
    void CompleteCancellation(RE::PlayerCharacter& a_player, float a_staminaCost, bool a_startBlock);
    void BeginBlock(RE::PlayerCharacter& a_player);
    void EndOwnedBlock(RE::PlayerCharacter& a_player);
    void ResetBlockState(bool a_stopOwnedBlock);

    [[nodiscard]] static bool IsActiveMeleeAttack(RE::ATTACK_STATE_ENUM a_state);
    [[nodiscard]] static bool IsCancellableAttack(RE::ATTACK_STATE_ENUM a_state);

    float _attackElapsed = 0.0F;
    RE::ATTACK_STATE_ENUM _lastAttackState = RE::ATTACK_STATE_ENUM::kNone;
    std::optional<PhysicalKey> _heldBlockKey;
    std::optional<PhysicalKey> _ownedBlockKey;
    std::optional<PhysicalKey> _vanillaBlockKey;
    bool _acceptedBlockPending = false;
    bool _ownsBlock = false;
    bool _timingActive = false;
    bool _inputRegistered = false;
    bool _menuEventsRegistered = false;
};

#include "Hooks.h"

#include "Combat.h"

#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

namespace Hooks {
namespace {
    struct ReadyWeaponCanProcessHook {
        static bool Thunk(RE::ReadyWeaponHandler* a_handler, RE::InputEvent* a_event) {
            if (Combat::ShouldSuppressReadyWeapon(a_event)) {
                return false;
            }
            return func(a_handler, a_event);
        }

        static inline REL::Relocation<decltype(Thunk)> func;
    };

    struct PlayerUpdateHook {
        static void Thunk(RE::PlayerCharacter* a_player, const float a_delta) {
            func(a_player, a_delta);
            Combat::GetSingleton()->Update(*a_player, a_delta);
        }

        static inline REL::Relocation<decltype(Thunk)> func;
    };
}

void Install() {
    REL::Relocation<std::uintptr_t> readyWeaponVTable {RE::ReadyWeaponHandler::VTABLE[0]};
    ReadyWeaponCanProcessHook::func = readyWeaponVTable.write_vfunc(0x1, ReadyWeaponCanProcessHook::Thunk);

    REL::Relocation<std::uintptr_t> playerVTable {RE::PlayerCharacter::VTABLE[0]};
    const auto updateSlot = REL::Relocate<std::size_t>(0xAD, 0xAD, 0xAF);
    PlayerUpdateHook::func = playerVTable.write_vfunc(updateSlot, PlayerUpdateHook::Thunk);
    SKSE::log::info("Hooks installed: ReadyWeaponHandler[1], PlayerCharacter[{:X}]", updateSlot);
}
}

#include <RE/Skyrim.h>

#include <SKSE/SKSE.h>

#include "Combat.h"
#include "Hooks.h"
#include "Settings.h"

namespace {
void ReloadSettings(RE::StaticFunctionTag* /*a_base*/) {
    SKSE::GetTaskInterface()->AddTask([] {
        Settings::Reload();
        Combat::GetSingleton()->ReconcileSettings();
    });
}

bool RegisterPapyrus(RE::BSScript::IVirtualMachine* a_virtualMachine) {
    a_virtualMachine->RegisterFunction("ReloadSettings", "CancelAttackSKSENative", ReloadSettings);
    return true;
}

void MessageHandler(SKSE::MessagingInterface::Message* a_message) { // NOLINT(misc-const-correctness)
    switch (a_message->type) {
        case SKSE::MessagingInterface::kInputLoaded: Combat::RegisterInput(); break;
        case SKSE::MessagingInterface::kDataLoaded:  Combat::RegisterLifecycleEvents(); break;
        case SKSE::MessagingInterface::kPreLoadGame:
        case SKSE::MessagingInterface::kNewGame:     Combat::PrepareForLoad(); break;
        default:                                     break;
    }
}
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_extender) {
    SKSE::Init(a_extender, {.logPattern = "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v"});
    Settings::Reload();
    Hooks::Install();
    SKSE::GetPapyrusInterface()->Register(RegisterPapyrus);
    SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);
    return true;
}

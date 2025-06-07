class CancelAttackHandler final : public RE::BSTEventSink<RE::InputEvent*>
{
public:
    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event,
        RE::BSTEventSource<RE::InputEvent*>*) override
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player || !player->IsAttacking() || !a_event || !*a_event) {
            return RE::BSEventNotifyControl::kContinue;
        }

        const auto* rightHand = player->GetEquippedObject(false);
        const auto* rightWeapon = rightHand ? rightHand->As<RE::TESObjectWEAP>() : nullptr;
        if (rightWeapon && (rightWeapon->IsBow() || rightWeapon->IsCrossbow() || rightWeapon->IsHandToHandMelee())) {
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

            if ((device == RE::INPUT_DEVICE::kMouse && key == 0x01) || (device == RE::INPUT_DEVICE::kGamepad && key == 0x0F)) {
                player->NotifyAnimationGraph("attackStop");

                const auto* left = player->GetEquippedObject(true);
                if (const auto* armor = left ? left->As<RE::TESObjectARMO>() : nullptr; armor && armor->IsShield()) {
                    player->NotifyAnimationGraph("blockStart");
                }
            }
        }

        return RE::BSEventNotifyControl::kContinue;
    }
};

static CancelAttackHandler g_cancelHandler;

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
    if (a_message->type == SKSE::MessagingInterface::kDataLoaded) {
        if (auto* inputEventSource = RE::BSInputDeviceManager::GetSingleton()) {
            inputEventSource->AddEventSink(&g_cancelHandler);
            logger::info("CancelAttackHandler registered");
        } else {
            logger::warn("Failed to register CancelAttackHandler - no input source");
        }
    }
}

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() noexcept {
    SKSE::PluginVersionData v;
    v.PluginName(Plugin::NAME.data());
    v.PluginVersion(Plugin::VERSION);
    v.UsesAddressLibrary();
    v.UsesNoStructs();
    return v;
}();

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface*, SKSE::PluginInfo* pluginInfo)
{
    pluginInfo->name = SKSEPlugin_Version.pluginName;
    pluginInfo->infoVersion = SKSE::PluginInfo::kVersion;
    pluginInfo->version = SKSEPlugin_Version.pluginVersion;
    return true;
}

void InitializeLog()
{
    auto path = logger::log_directory();
    if (!path) {
        stl::report_and_fail("Failed to find standard logging directory");
    }

    *path /= Plugin::NAME;
    *path += ".log";
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
    auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));

    log->set_level(spdlog::level::info);
    log->flush_on(spdlog::level::info);
    set_default_logger(std::move(log));

    spdlog::set_pattern("[%H:%M:%S:%e] %v");
    logger::info(FMT_STRING("{} v{}"), Plugin::NAME, Plugin::VERSION);
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
    InitializeLog();
    logger::info("Game version : {}", a_skse->RuntimeVersion().string());

    Init(a_skse);
    SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);
    return true;
}

#include "CancelAttackHandler.h"
#include "Config.h"

class MenuWatcher final : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
    RE::BSEventNotifyControl ProcessEvent(
        const RE::MenuOpenCloseEvent* a_event,
        RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
    {
        if (!a_event || a_event->menuName.empty())
            return RE::BSEventNotifyControl::kContinue;

        if (a_event->menuName == RE::InterfaceStrings::GetSingleton()->journalMenu && !a_event->opening) {
            CancelAttackHandler::GetSingleton()->UpdateBlockMappings();
        }

        return RE::BSEventNotifyControl::kContinue;
    }
};

static MenuWatcher g_menuWatcher;

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void MessageHandler(SKSE::MessagingInterface::Message* a_message)
{
    if (a_message->type == SKSE::MessagingInterface::kDataLoaded) {
        if (auto* inputEventSource = RE::BSInputDeviceManager::GetSingleton()) {
            inputEventSource->AddEventSink(CancelAttackHandler::GetSingleton());
        } else {
            logger::warn("Failed to register CancelAttackHandler");
        }

        if (auto ui = RE::UI::GetSingleton()) {
            ui->AddEventSink(&g_menuWatcher);
        } else {
            logger::warn("Failed to register MenuWatcher");
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
    Config::Load();
    SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);
    return true;
}

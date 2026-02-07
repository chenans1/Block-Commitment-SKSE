#include "PCH.h"

#include <Windows.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

#include "ABHandler.h"
#include "settings.h"
#include "updateHook.h"
#include "altBlock.h"
#include "utils.h"
#include "bashHandler.h"
#include "AnimEventSink.h"
#include "notifyHook.h"

using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

namespace {
    void initialize_log() {
        auto path = log_directory();
        if (!path) {
            report_and_fail("SKSE logs directory not found");
        }
        *path /= PluginDeclaration::GetSingleton()->GetName();
        *path += L".log";

        std::shared_ptr<spdlog::logger> loggerPtr;

        if (IsDebuggerPresent()) {
            // share with debugger
            auto debugLoggerPtr = std::make_shared<spdlog::sinks::msvc_sink_mt>();
            auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
            spdlog::sinks_init_list loggers{std::move(fileLoggerPtr), std::move(debugLoggerPtr)};
            loggerPtr = std::make_shared<spdlog::logger>("log", loggers);
        } else {
            // If no debugger is attached, only log to the file.
            auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
            loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
        }

        loggerPtr->set_level(spdlog::level::info);
        loggerPtr->flush_on(spdlog::level::info);
        spdlog::set_default_logger(std::move(loggerPtr));
    }
}

static void MessageHandler(SKSE::MessagingInterface::Message* msg) {
    if (!msg) {
        return;
    }
    switch (msg->type) {
        case SKSE::MessagingInterface::kDataLoaded: {
            auto* inputMgr = RE::BSInputDeviceManager::GetSingleton();
            if (!inputMgr) {
                log::warn("BSInputDeviceManager Not Available");
                return;
            }
            inputMgr->AddEventSink(&altBlock::AltBlockInputSink::GetSingleton());
            utils::initKeyword();
            utils::init();
            auto* bashController = bash::bashController::GetSingleton();
            bashController->init();
            break;
        }
        case SKSE::MessagingInterface::kPostLoadGame: {
            if (auto* pc = RE::PlayerCharacter::GetSingleton()) {
                pc->AddAnimationGraphEventSink(&block::AnimEventSink::GetSingleton());
                log::info("registered anim event sink");
            }
            break;
        }
        default:
            break;
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    initialize_log();
    auto* plugin = PluginDeclaration::GetSingleton();
    auto version = plugin->GetVersion();
    log::info("{} {} is loading...", plugin->GetName(), version);
    SKSE::Init(skse);
    ABHook::Install();
    updateHook::PlayerUpdateHook::Install();
    settings::RegisterMenu();
    settings::load();
    SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);
    SKSE::AllocTrampoline(14);  // Allocate trampoline space for the hook
    notify::Install();
    log::info("{} has finished loading.", plugin->GetName());
    return true;
}
#include "PCH.h"
#include "ABHandler.h"
#include "settings.h"
#include "blockHandler.h"
#include "utils.h"
#include "blockCommit.h"
#include "bashHandler.h"

using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

//cache the left key -> this should make rebinding not require u to restart game
static inline RE::INPUT_DEVICE g_blockDevice = RE::INPUT_DEVICE::kKeyboard;
static inline std::uint32_t g_blockIDCode = 0;

using ProcessButton_t = void (*)(RE::AttackBlockHandler*, RE::ButtonEvent*, RE::PlayerControlsData*);
static inline ProcessButton_t _ProcessButton = nullptr;

static bool bashing = false;

static void ABHook_handler(RE::AttackBlockHandler* self, RE::ButtonEvent* ev, RE::PlayerControlsData* data) {
    if (!self || !ev || !data || !_ProcessButton) {
        log::warn("[ABHook]: missing self/ev/data/_ProcessButton");
        return;
    }
    //if it's not a valid equip combo -> return
    auto* pc = RE::PlayerCharacter::GetSingleton();
    if (!pc) {
        return _ProcessButton(self, ev, data);
    }
    if (settings::mageBlock() && utils::isRightHandCaster(pc)) {
        if (pc->IsBlocking()) {
            if (settings::log()) log::info("right hand caster, is blocking - swallowing input");
            return;
        }
        return _ProcessButton(self, ev, data);
    }
    
    if (ev->QUserEvent() == "Left Attack/Block") {
        if (!utils::isLeftKeyBlock(pc)) {
            return _ProcessButton(self, ev, data);
        }
        g_blockDevice = ev->GetDevice();
        g_blockIDCode = ev->GetIDCode();
        auto* blockController = blockCommit::Controller::GetSingleton();
        if (ev->IsDown()) {
            if (!settings::replaceLeftWBash()||pc->IsAttacking()) {
                blockController->beginLeftBlock();
                return _ProcessButton(self, ev, data);
            }
            /*auto* bashController = bash::bashController::GetSingleton();
            bashController->beginBash(pc);*/
            if (utils::tryBashStart(pc)) {
                bashing = true;
            }
            return _ProcessButton(self, ev, data);
        /*} else {*/
        } else if (ev->IsUp()) {
            if (settings::replaceLeftWBash() && bashing) {
                utils::tryBashRelease(pc);
                bashing = false;
            }
            if (ev->HeldDuration() < settings::getCommitDur()) {
                const bool swallowed = blockController->wantReleaseLeftBlock();
                if (swallowed) {
                    if (settings::log()) log::info("[ABHook]: denied left release");
                    return;
                }
                return _ProcessButton(self, ev, data);
            } else {
                if (pc->IsBlocking()) blockController->reset();
                return _ProcessButton(self, ev, data);
            }
            
        }
    }
    return _ProcessButton(self, ev, data);
}

static void InjectReleaseLeft() {
    if (!_ProcessButton) {
        return;
    }
    auto* controls = RE::PlayerControls::GetSingleton();
    if (!controls || !controls->attackBlockHandler) {
        return;
    }

    auto* pdata = std::addressof(controls->data);

    // dont bother injecting if not blocking maybe?
    auto* pc = RE::PlayerCharacter::GetSingleton();
    if (!pc) {
        return;
    }
    // create synthetic button release event
    RE::ButtonEvent* release = RE::ButtonEvent::Create(g_blockDevice, "Left Attack/Block", g_blockIDCode, 0.0f, 0.0f);
    if (!release) {
        return;
    }
    _ProcessButton(controls->attackBlockHandler, release, pdata);
    RE::free(release);
    if (settings::log()) log::info("[ABhook]: sucessfully injected release key");
    // send block stop and the key release
    //pc->NotifyAnimationGraph("blockStop");
}

void ABHook::Check() { 
    auto* bh = block::blockHandler::GetSingleton(); 
    if (bh->IsBlockHeld()) {
        if (settings::log()) log::info("Check: blockKey is held");
        return;
    }
    if (bh->consumeReleaseRequest()) {
        if (settings::log()) log::info("Consumed Release Request");
        InjectReleaseLeft();
    }
}

void ABHook::Install() {
    log::info("Attempting to install ABHook...");
    REL::Relocation<std::uintptr_t> vtbl{RE::VTABLE_AttackBlockHandler[0]};
    const std::uintptr_t orig = vtbl.write_vfunc(0x4, &ABHook_handler);
    _ProcessButton = reinterpret_cast<ProcessButton_t>(orig);
    log::info("Finished Installing ABhook");
};
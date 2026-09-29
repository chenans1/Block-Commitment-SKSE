#include "PCH.h"
#include "ABHandler.h"
#include "settings.h"
#include "utils.h"
#include "blockCommit.h"
#include "bashHandler.h"

using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

using ProcessButton_t = void (*)(RE::AttackBlockHandler*, RE::ButtonEvent*, RE::PlayerControlsData*);
static inline ProcessButton_t _ProcessButton = nullptr;

static bool releasedBash = false;

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
    const auto* userEvents = RE::UserEvents::GetSingleton();
    auto* bh = blockCommit::Controller::GetSingleton();
    // A block press must finish through the same handler even if the player's
    // equipment or bash settings changed before the key was released.
    if (userEvents && ev->QUserEvent() == userEvents->leftAttack && ev->IsUp() && bh->IsLeftBlockHeld()) {
        if (bh->OnLeftBlockUp(ev->HeldDuration())) {
            if (settings::log()) log::info("[ABHook]: denied left release");
            return;
        }
        _ProcessButton(self, ev, data);
        bh->OnLeftReleaseForwarded(pc);
        return;
    }
    if (!userEvents) return _ProcessButton(self, ev, data);
    /*removed checking setings to see if mage blocking is enabled to avoid bricking controls if user installs nemesis
        patch without turning on mageBlocking*/ 
    if (utils::isRightHandCaster(pc)) {
        if (pc->IsBlocking()) {
            if (settings::log()) log::info("right hand caster, is blocking - swallowing input");
            return;
        }
        return _ProcessButton(self, ev, data);
    }
    auto* st = pc->AsActorState();

    //for bashing if you've released block key but the block animation is still on
    // if (ev->QUserEvent() == userEvents->rightAttack) {
    //     if (st && pc->IsBlocking() && st->actorState2.wantBlocking == 0) {
    //         st->actorState2.wantBlocking = 1;
    //         _ProcessButton(self, ev, data);
    //         st->actorState2.wantBlocking = 0;
    //         return;
    //     }
    //     return _ProcessButton(self, ev, data);
    // }

    if (ev->QUserEvent() ==  userEvents->leftAttack) {
        if (!utils::isLeftKeyBlock(pc)) {
            return _ProcessButton(self, ev, data);
        }
        if (!st) return _ProcessButton(self, ev, data);
        const bool isDrawn = st->IsWeaponDrawn();
        if (settings::leftHandBash() && !isDrawn) {
            return _ProcessButton(self, ev, data);
        }
        // auto* bashHandler = bash::bashController::GetSingleton();
        if (ev->IsDown()) {
            if (settings::leftHandBash()){
                // releasedBash = false;
                if (st && !pc->IsAttacking()) {
                    
                    // pc->NotifyAnimationGraph("blockStart");
                    st->actorState2.wantBlocking = 1;
                    if (utils::tryBashStart(pc)) {
                        // st->actorState1.meleeAttackState = RE::ATTACK_STATE_ENUM::kBash;
                        releasedBash = false;
                        st->actorState2.wantBlocking = 0;
                    }
                    // utils::forceUpdateBashAttackData();
                    // if (pc->NotifyAnimationGraph("bashStart")) {
                    //     st->actorState1.meleeAttackState = RE::ATTACK_STATE_ENUM::kBash;
                    // }
                    // st->actorState2.wantBlocking = 0;
                }
                return;
            } else {
                bh->OnLeftBlockDown(ev);
            }
            return _ProcessButton(self, ev, data);
        // held down longer than power bash, power bash auto release?
        } else if (ev->IsPressed() && settings::leftHandBash() && !releasedBash) {
            if (st && ev->HeldDuration() >= settings::powerBashDelay()) {
                if (st->actorState1.meleeAttackState == RE::ATTACK_STATE_ENUM::kBash) {
                    if (ev->HeldDuration() >= settings::powerBashDelay()) {
                        utils::tryBashPowerStart(pc);
                        releasedBash = true;
                        st->actorState2.wantBlocking = 0;
                        // if (pc->IsBlocking()) {
                        //     pc->NotifyAnimationGraph("blockStop");
                        // }
                    }
                }
            }
            return;
        } else if (ev->IsUp()) {
            if (settings::leftHandBash()) {
                if (!releasedBash && st) {
                    if (st->actorState1.meleeAttackState == RE::ATTACK_STATE_ENUM::kBash) {
                        if (ev->HeldDuration() < settings::powerBashDelay()) {
                            utils::tryBashRelease(pc);
                        }
                        releasedBash = true;
                        st->actorState2.wantBlocking = 0;
                        // if (pc->IsBlocking()) {
                        //     pc->NotifyAnimationGraph("blockStop");
                        // }
                        
                    }
                }
                return;
            }
            return _ProcessButton(self, ev, data);
        }
    }
    return _ProcessButton(self, ev, data);
}

void ABHook::Check() { 
    auto* bh = blockCommit::Controller::GetSingleton();
    if (bh->IsLeftBlockHeld()) {
        if (settings::log()) log::info("Check: blockKey is held");
        return;
    }
    bh->TryInjectLeftRelease(_ProcessButton);
}

void ABHook::Install() {
    log::info("Attempting to install ABHook...");
    REL::Relocation<std::uintptr_t> vtbl{RE::VTABLE_AttackBlockHandler[0]};
    const std::uintptr_t orig = vtbl.write_vfunc(0x4, &ABHook_handler);
    _ProcessButton = reinterpret_cast<ProcessButton_t>(orig);
    log::info("Finished Installing ABhook");
};

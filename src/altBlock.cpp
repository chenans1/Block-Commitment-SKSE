#include "PCH.h"
#include "altBlock.h"
#include "settings.h"
#include "utils.h"
#include "blockCommit.h"

//largely adapted from Dual Wield Parrying SKSE
//https://github.com/DennisSoemers/DualWieldParryingSKSE/blob/main/src/InputEventHandler.cpp

static bool isUIClosed() { 
    const auto ui = RE::UI::GetSingleton();
    if (ui && !ui->GameIsPaused() && !ui->IsApplicationMenuOpen() && !ui->IsItemMenuOpen() 
        && !ui->IsMenuOpen(RE::InterfaceStrings::GetSingleton()->dialogueMenu)) {
        return true;
    }
    return false;
}

static bool validPlayerState(RE::PlayerCharacter* player) {
    auto playerAI = player->GetActorRuntimeData().currentProcess;
    const auto playerState = player->AsActorState();
    if (playerState && playerState->GetWeaponState() == RE::WEAPON_STATE::kDrawn &&
        playerState->GetSitSleepState() == RE::SIT_SLEEP_STATE::kNormal &&
        playerState->GetKnockState() == RE::KNOCK_STATE_ENUM::kNormal &&
        playerState->GetFlyState() == RE::FLY_STATE::kNone && !player->IsInKillMove() && playerAI) {
        return true;
    }
    return false;
}

static bool areControlsEnabled() {
    const auto controlMap = RE::ControlMap::GetSingleton();
    const auto playerControls = RE::PlayerControls::GetSingleton();
    if (controlMap->IsFightingControlsEnabled() && playerControls->attackBlockHandler->inputEventHandlingEnabled) {
        return true;
    }
    return false;
}

namespace altBlock {
    static bool modifierKeyHeld = false;
    static bool isBashing = false;

    static bool isModRequired() { 
        const int modifierKey = settings::getModifierKey();
        return (modifierKey > 0 && modifierKey < 300);
    }
    
    RE::BSEventNotifyControl AltBlockInputSink::ProcessEvent(
        RE::InputEvent* const* a_events, RE::BSTEventSource<RE::InputEvent*>*) {
        if (!a_events) {
            return RE::BSEventNotifyControl::kContinue;
        }
        //check if the game is paused, in ui, or else:
        const auto ui = RE::UI::GetSingleton();
        if (!isUIClosed() || !areControlsEnabled()) {
            return RE::BSEventNotifyControl::kContinue;
        }
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return RE::BSEventNotifyControl::kContinue;
        }
        if (!settings::mageBlock() && utils::isRightHandCaster(player)) {
            return RE::BSEventNotifyControl::kContinue;
        }
        //check here for enderal telescopes
        const auto* leftForm = player->GetEquippedObject(true);
        const auto* leftShield = leftForm ? leftForm->As<RE::TESObjectARMO>() : nullptr;
        if (leftShield) {
            // log::info("shield equipped");
            if (utils::hasTelescopeKeyword(leftShield)) {
                SKSE::log::info("shield has telescope keyword");
                return RE::BSEventNotifyControl::kContinue;
            }
        }

        if (!validPlayerState(player)) {
            return RE::BSEventNotifyControl::kContinue;
        }
        /*if (!utils::canAltBlock(player)) {
            return RE::BSEventNotifyControl::kContinue;
        }*/
        //disable the altblock if you don't need it to block
        if (settings::isDoubleBindDisabled() && utils::isLeftKeyBlock(player) && !settings::altBlockBash()) {
            if (settings::log()) SKSE::log::info("left key is block, altBash Disabled, no double binds - alt block denied");
            return RE::BSEventNotifyControl::kContinue;
        }
        const int bind = settings::getAltBlock();
        if (bind <= 0) {
            return RE::BSEventNotifyControl::kContinue;
        }
        const int modifierKey = settings::getModifierKey();
        const bool needsModifier = isModRequired();

        //afaik this is a linkedlist so we gotta traverse and check
        for (auto ev = *a_events; ev != nullptr; ev = ev->next) {
            auto* btn = ev->AsButtonEvent();
            if (!btn) continue;

            const int macro = settings::toKeyCode(*btn);

            //check if it's mod key
            if (needsModifier && macro == modifierKey) {
                if (btn->IsDown() || btn->IsHeld()) {
                    if (settings::log()) SKSE::log::info("Mod={} held", modifierKey);
                    modifierKeyHeld = true;
                } else if (btn->IsUp()) {
                    if (settings::log()) SKSE::log::info("Mod={} released", modifierKey);
                    modifierKeyHeld = false;
                }
            }
            if (macro != bind) continue;

            auto* st = player->AsActorState();
            if (!st) { return RE::BSEventNotifyControl::kContinue; }
            auto* blockController = blockCommit::Controller::GetSingleton();
            const bool bashInstead = settings::altBlockBash() && utils::isLeftKeyBlock(player);
            if (btn->IsDown()) {
                if (needsModifier && !modifierKeyHeld) {
                    if (settings::log()) SKSE::log::info("Mod={} not held, no block", modifierKey);
                    return RE::BSEventNotifyControl::kContinue;
                }               
                //st->actorState2.wantBlocking = 1;
                //do nothing due to attack data won't be updated properly, need to fix this separately some other time
                if (bashInstead && player->IsAttacking()) {
                    return RE::BSEventNotifyControl::kContinue;
                }
                if (!bashInstead) blockController->beginAltBlock();
                if (utils::tryBlockIdle(player)) {
                    if (settings::log()) SKSE::log::info("[altBlock] tryBlockIdle Sucessful");
                    st->actorState2.wantBlocking = 1;
                    if (bashInstead) {
                        if (utils::tryBashStart(player)) {
                            isBashing = true;
                        }
                    } /*else {
                        blockController->beginAltBlock();
                    }*/
                }
                return RE::BSEventNotifyControl::kContinue;
            } else if (btn->IsPressed() && btn->HeldDuration() >= settings::powerBashDelay()) {
                if (bashInstead && isBashing) {
                    utils::tryBashPowerStart(player);
                    player->NotifyAnimationGraph("blockStop");
                    st->actorState2.wantBlocking = 0;
                    isBashing = false;
                }
            } else if (btn->IsUp()) {
                if (!bashInstead) {
                    blockController->wantReleaseAltBlock();
                    isBashing = false;
                    //return RE::BSEventNotifyControl::kContinue;
                } else {
                    if (isBashing) {
                        if (btn->HeldDuration() < settings::powerBashDelay()) {
                            utils::tryBashRelease(player);
                        }
                    }
                    if (player->IsBlocking()) {
                        player->NotifyAnimationGraph("blockStop");
                    }
                    st->actorState2.wantBlocking = 0;
                }
                isBashing = false;
            }
            //return RE::BSEventNotifyControl::kContinue;
        }
        return RE::BSEventNotifyControl::kContinue;
    }
}
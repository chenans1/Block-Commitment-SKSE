#include "PCH.h"
#include "altBlock.h"
#include "settings.h"
#include "utils.h"

//largely adapted from Dual Wield Parrying SKSE
//https://github.com/DennisSoemers/DualWieldParryingSKSE/blob/main/src/InputEventHandler.cpp

bool isUIClosed(RE::UI* ui) { 
    if (ui && !ui->GameIsPaused() && !ui->IsApplicationMenuOpen() && !ui->IsItemMenuOpen() 
        && !ui->IsMenuOpen(RE::InterfaceStrings::GetSingleton()->dialogueMenu)) {
        return true;
    }
    return false;
}

bool validPlayerState(RE::PlayerCharacter* player) {
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

bool areControlsEnabled() {
    const auto controlMap = RE::ControlMap::GetSingleton();
    const auto playerControls = RE::PlayerControls::GetSingleton();
    if (controlMap->IsFightingControlsEnabled() && playerControls->attackBlockHandler->inputEventHandlingEnabled) {
        return true;
    }
    return false;
}

namespace altBlock {
    //start set the want block flag, if not already blocking we start the block
    static void StartBlock(RE::PlayerCharacter* player, RE::ActorState* st, bool isBlocking) {
        //st->actorState2.wantBlocking = 1;
        if (!isBlocking) {
            SKSE::log::info("Starting AltBlock");
            player->NotifyAnimationGraph("blockStart");
        }
    }

    static void StopBlock(RE::PlayerCharacter* player, RE::ActorState* st, bool isBlocking) {
        //st->actorState2.wantBlocking = 0;
        if (isBlocking) {
            SKSE::log::info("Stopping AltBlock");
            player->NotifyAnimationGraph("blockStop");
        }
    }

    RE::BSEventNotifyControl AltBlockInputSink::ProcessEvent(
        RE::InputEvent* const* a_events, RE::BSTEventSource<RE::InputEvent*>*) {
        if (!a_events) {
            return RE::BSEventNotifyControl::kContinue;
        }
        //check if the game is paused, in ui, or else:
        const auto ui = RE::UI::GetSingleton();
        if (!isUIClosed(ui) || !areControlsEnabled()) {
            return RE::BSEventNotifyControl::kContinue;
        }
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return RE::BSEventNotifyControl::kContinue;
        }
        if (!validPlayerState(player) || !utils::canAltBlock(player)) {
            return RE::BSEventNotifyControl::kContinue;
        }

        const int bind = settings::getAltBlock();
        if (bind <= 0) {
            return RE::BSEventNotifyControl::kContinue;
        }
        //afaik this is a linkedlist so we gotta traverse and check
        for (auto ev = *a_events; ev != nullptr; ev = ev->next) {
            auto* btn = ev->AsButtonEvent();
            if (!btn) continue;

            const int macro = settings::toKeyCode(*btn);
            if (macro != bind) continue;
            
            //if not blocking, then start blocking
            // if fail to fetch the variable dont process
            bool isBlocking = false;
            if (!player->GetGraphVariableBool("IsBlocking", isBlocking)) {
                return RE::BSEventNotifyControl::kContinue;
            }
            // now we finally actually check if we are blocking.
            auto* st = player->AsActorState();
            if (!st) {
                return RE::BSEventNotifyControl::kContinue;
            }
            if (btn->IsDown()) {
                StartBlock(player, st, isBlocking);
                return RE::BSEventNotifyControl::kContinue;
            }

            if (btn->IsUp()) {
                StopBlock(player, st, isBlocking);
                return RE::BSEventNotifyControl::kContinue;
            }
        }
        return RE::BSEventNotifyControl::kContinue;
    }
}
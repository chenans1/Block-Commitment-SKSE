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

namespace altBlock {
    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_events, RE::BSTEventSource<RE::InputEvent*>*) {
        if (!a_events) {
            return RE::BSEventNotifyControl::kContinue;
        }
        //check if the game is paused, in ui, or else:
        const auto ui = RE::UI::GetSingleton();
        if (!isUIClosed(ui)) {
            return RE::BSEventNotifyControl::kContinue;
        }
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return RE::BSEventNotifyControl::kContinue;
        }
        if (!validPlayerState(player)) {
            return RE::BSEventNotifyControl::kContinue;
        }
        if (!utils::canAltBlock(player)) {
            return RE::BSEventNotifyControl::kContinue;
        }

        //now we finally actually check if we are blocking.
        //if fail to fetch the variable or we are already blocking: don't do anything.
        bool isBlocking = false;
        if (!player->GetGraphVariableBool("IsBlocking", isBlocking) || !isBlocking) {
            return RE::BSEventNotifyControl::kContinue;
        }

        return RE::BSEventNotifyControl::kContinue;
    }


}
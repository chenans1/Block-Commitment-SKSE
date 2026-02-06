#include "PCH.h"
#include "notifyHook.h"
#include "blockCommit.h"
#include "settings.h"
#include "utils.h"

namespace notify {
    void consumeStamina(RE::PlayerCharacter* player) {
        if (!player) return;
        auto* actorAV = player->AsActorValueOwner();
        if (actorAV) {
            actorAV->DamageActorValue(RE::ActorValue::kStamina, settings::blockCancelCost());
            SKSE::log::info("Damaged stamina={}", settings::blockCancelCost());
        }
    }

    // handles consuming stamina for block canceling
    void resolveBlockCancel(RE::PlayerCharacter* player) {
        if (!player) return;
        const bool recoveryAllowed = settings::allowMCORecovery();
        // if we are in recovery, allow block cancel free of cost
        if (recoveryAllowed) {
            bool MCO_recovery = false;
            if (player->GetGraphVariableBool("MCO_IsInRecovery", MCO_recovery)) {
                if (MCO_recovery) {
                    SKSE::log::info("Player is in MCO_recovery, allow block");
                    return;
                }
            }
        }
        // const bool blockCancelEnabled = settings::isBlockCancelEnabled();
        //  now we need to check if attacking
        auto* playerState = player->AsActorState();
        if (playerState) {
            // Check if player is attacking - bit less restrictive than checking for isAttacking()
            /* auto attackState = playerState->GetAttackState();
             bool isAttacking = (attackState == RE::ATTACK_STATE_ENUM::kSwing || attackState ==
             RE::ATTACK_STATE_ENUM::kHit); SKSE::log::info("player attack state = {}",
             std::to_underlying(attackState));*/
            const bool isAttacking = player->IsAttacking();
            if (isAttacking) {
                consumeStamina(player);
            }
            return;
        }
    }

    bool PC_NotifyAnimationGraph(RE::IAnimationGraphManagerHolder* a_this, const RE::BSFixedString& a_eventName) {
        const bool result = _PC_NotifyAnimationGraph(a_this, a_eventName);

        if (a_eventName == "blockStart" || a_eventName == "blockStop") {
            // Handle your event here
            // a_this is the player character
            //auto player = static_cast<RE::PlayerCharacter*>(a_this);

            if (a_eventName == "blockStart") {
                blockCommit::Controller::GetSingleton()->beginAltBlock();
                if (settings::isBlockCancelEnabled()) {
                    if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                        resolveBlockCancel(player);
                    }
                }
            } else if (a_eventName == "blockStop") {
                if (settings::log()) SKSE::log::info("block exit invoke reset()");
                blockCommit::Controller::GetSingleton()->reset();
            }
        }

        return result;
    }

    void Install() {
        SKSE::log::info("Installing PlayerCharacter animation graph hook...");

        REL::Relocation<uintptr_t> PlayerCharacter_IAnimationGraphManagerHolderVtbl{RE::VTABLE_PlayerCharacter[3]};
        _PC_NotifyAnimationGraph =
            PlayerCharacter_IAnimationGraphManagerHolderVtbl.write_vfunc(0x1, PC_NotifyAnimationGraph);

        SKSE::log::info("PlayerCharacter animation graph hook installed successfully");
    }

}
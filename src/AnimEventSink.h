#pragma once

#include "blockCommit.h"
#include "utils.h"
#include "settings.h"

void consumeStamina(RE::PlayerCharacter* player) {
    if (!player) return;
    auto* actorAV = player->AsActorValueOwner();
    if (actorAV) {
        actorAV->DamageActorValue(RE::ActorValue::kStamina, settings::blockCancelCost());
        SKSE::log::info("Damaged stamina={}", settings::blockCancelCost());
    }
}

//handles consuming stamina for block canceling
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
    //const bool blockCancelEnabled = settings::isBlockCancelEnabled();
    // now we need to check if attacking
    auto* playerState = player->AsActorState();
    if (playerState) {
        // Check if player is attacking - bit less restrictive than checking for isAttacking()
       /* auto attackState = playerState->GetAttackState();
        bool isAttacking = (attackState == RE::ATTACK_STATE_ENUM::kSwing || attackState == RE::ATTACK_STATE_ENUM::kHit);
        SKSE::log::info("player attack state = {}", std::to_underlying(attackState));*/
        const bool isAttacking = player->IsAttacking();
        if (isAttacking) {
            consumeStamina(player);
        }
        return;
    }
}

namespace block {
    class AnimEventSink : public RE::BSTEventSink<RE::BSAnimationGraphEvent> {
    public:
        static AnimEventSink& GetSingleton() {
            static AnimEventSink inst;
            return inst;
        }

        RE::BSEventNotifyControl ProcessEvent(
            const RE::BSAnimationGraphEvent* a_event,
            RE::BSTEventSource<RE::BSAnimationGraphEvent>*) {
            if (!a_event || !a_event->holder) {
                return RE::BSEventNotifyControl::kContinue;
            }

            if (a_event->tag == "SBF_BlockStart"sv) {
                blockCommit::Controller::GetSingleton()->beginAltBlock();
                if (settings::isBlockCancelEnabled()) {
                    if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                        resolveBlockCancel(player);
                    }
                }
                return RE::BSEventNotifyControl::kContinue;
            }

            if (a_event->tag == "SBF_BlockStart"sv) {
                if (settings::log()) SKSE::log::info("block exit invoke reset()");
                blockCommit::Controller::GetSingleton()->reset();
                return RE::BSEventNotifyControl::kContinue;
            }

            return RE::BSEventNotifyControl::kContinue;
        }

    private:
        // constructors and destructors
        AnimEventSink() = default;
        ~AnimEventSink() = default;
        AnimEventSink(const AnimEventSink&) = delete;
        AnimEventSink(AnimEventSink&&) = delete;
        AnimEventSink& operator=(const AnimEventSink&) = delete;
        AnimEventSink& operator=(AnimEventSink&&) = delete;
    };
}
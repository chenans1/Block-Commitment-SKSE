#include "PCH.h"

#include "settings.h"

using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

namespace utils {
    // check for equip. Assume mco-style left clicks
    // thus if left hand is weapon, yes, also useful as a check for 1h+spell
    bool isLeftKeyBlock(RE::PlayerCharacter* player) {
        if (!player) return false;
        const auto* rightForm = player->GetEquippedObject(false);
        const auto* leftForm = player->GetEquippedObject(true);
        const auto* rightWeap = rightForm ? rightForm->As<RE::TESObjectWEAP>() : nullptr;
        const auto* leftWeap = leftForm ? leftForm->As<RE::TESObjectWEAP>() : nullptr;
        const auto* leftShield = leftForm ? leftForm->As<RE::TESObjectARMO>() : nullptr;

        //shield: always can block
        if (leftShield) {
            // log::info("shield equipped");
            return true;
        }

        // you can't block without a weapon in the right hand
        if (!rightWeap) {
            //log::info("no right handed weapon");
            return false;
        }

        const auto rType = rightWeap->GetWeaponType();

        // 1: check if two handed
        if (rType >= RE::WeaponTypes::kTwoHandSword && rType <= RE::WeaponTypes::kTwoHandAxe) {
            //log::info("two handed weapon");
            return true;
        }

        // 2: if right hand isn't 1h and isn't 2h then we return false;
        const bool validRight = (rType >= RE::WeaponTypes::kHandToHandMelee && rType <= RE::WeaponTypes::kOneHandMace);
        if (!validRight) {
            //log::info("invalid right hand type");
            return false;
        }
        

        // now check left, make sure it's either unarmed or one handed
        if (leftWeap) {
            const auto lType = leftWeap->GetWeaponType();
            // if left = attack and right hand is 1h weapon then left hand must be empty
            if (settings::leftAttack() && lType != RE::WeaponTypes::kHandToHandMelee) {
                //log::info("left = attack, left hand is not unarmed/shield");
                return false;
            }
            if (lType >= RE::WeaponTypes::kHandToHandMelee && lType <= RE::WeaponTypes::kOneHandMace) {
                //log::info("valid right hand, valid left hand");
                return true;
            }
        }
        // at this point if there is no left weapon type then we know it's probably a spell in the left hand
        return false;
    }

    //need to check if right hand allows for blocking only - must not be bow
    bool canAltBlock(RE::PlayerCharacter* player) {
        if (!player) return false;
        const auto* rightForm = player->GetEquippedObject(false);
        const auto* rightWeap = rightForm ? rightForm->As<RE::TESObjectWEAP>() : nullptr;

        const auto* leftForm = player->GetEquippedObject(true);
        const auto* leftShield = leftForm ? leftForm->As<RE::TESObjectARMO>() : nullptr;

        //if you have left shield always can block, but you don't need alt block key in this case so uhhh?
        if (leftShield) {
            // log::info("shield equipped");
            return true;
        }

        // you can't block without a weapon in the right hand
        if (!rightWeap) {
            //log::info("no right handed weapon");
            return false;
        }

        const auto rType = rightWeap->GetWeaponType();

        //just check if it's not a ranged weapon/staff
        if (rType >= RE::WeaponTypes::kHandToHandMelee && rType <= RE::WeaponTypes::kTwoHandAxe) {
            //log::info("two handed weapon");
            return true;
        }
        return false;
    }

    bool isPlayerAttacking(RE::PlayerCharacter* player) { 
        if (!player) return false;
        if (player->IsAttacking() || player->IsPowerAttacking()) {
            log::info("Player is Attacking");
            return true;
        }
        return false;
    }

    //returns true if block is allowed, false if not. 
    bool resolveBlockCancel(RE::PlayerCharacter* player) { 
        if (!player) return false;
        const bool recoveryAllowed = settings::allowMCORecovery();
        //if we are in recovery, allow block cancel free of cost
        if (recoveryAllowed) {
            bool MCO_recovery = false;
            if (player->GetGraphVariableBool("MCO_IsInRecovery", MCO_recovery)) {
                if (MCO_recovery) {
                    log::info("Player is in MCO_recovery, allow block");
                    return true;
                }
            }
        }
        const bool blockCancelEnabled = settings::isBlockCancelEnabled();
        //now we need to check if attacking
        auto* playerState = player->AsActorState();
        if (playerState) {
            auto attackState = playerState->GetAttackState();

            // Check if player is attacking
            bool isAttacking =
                (attackState == RE::ATTACK_STATE_ENUM::kSwing || attackState == RE::ATTACK_STATE_ENUM::kHit);
            log::info("player attack state = {}", std::to_underlying(attackState));
            if (isAttacking) {
                if (blockCancelEnabled) {
                    return true;
                } else {
                    return false;
                }
            }
            return true;
        }

        return false;
    }

    //forcefully bashes
    void forceBashAttack(RE::PlayerCharacter* player) {
        if (player) {
            auto* st = player->AsActorState();
            if (st) {
                // Set the attack state to bash
                st->actorState1.meleeAttackState = RE::ATTACK_STATE_ENUM::kBash;
                player->NotifyAnimationGraph("bashStart");
                log::info("player forced bash attack");
            }
        }
    }

    static RE::BGSAction* actionBash = nullptr;
    static const SKSE::TaskInterface* g_taskInterface = nullptr;

    //gonna try to force bash, then do bash release action?
    void init() {
        actionBash = (RE::BGSAction*)RE::TESForm::LookupByID(0x13454);
        g_taskInterface = SKSE::GetTaskInterface();
        log::info("bashstuff: init....");
        if (!actionBash) {
            SKSE::log::error("Failed to find bash action!");
        }
        if (!g_taskInterface) {
            SKSE::log::error("Failed to get task interface!");
        }
    }

    //similar to OCPA - gonna do some replace l with bash trickery
    void PerformBash(RE::Actor* a) {
        if (!actionBash) {
            SKSE::log::error("Bash action not initialized!");
            return;
        }
        if (!g_taskInterface) {
            SKSE::log::error("Task interface not available!");
            return;
        }
        g_taskInterface->AddTask([a]() {
            std::unique_ptr<RE::TESActionData> data(RE::TESActionData::Create());
            data->source = RE::NiPointer<RE::TESObjectREFR>(a);
            data->action = actionBash;

            typedef bool func_t(RE::TESActionData*);
            REL::Relocation<func_t> func{RELOCATION_ID(40551, 41557)};
            bool success = func(data.get());

            if (settings::log()) {
                SKSE::log::info("Bash trigger: {}", success ? "success" : "failed");
            }
        });
    }
}
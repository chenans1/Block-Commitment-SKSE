#include "PCH.h"

#include "settings.h"

using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

namespace utils {
    inline RE::BGSKeyword* telescopeKYWD = nullptr;
    inline std::atomic_bool kywdCache{false};
    inline RE::TESIdleForm* blockingStartIdle;
    inline RE::TESIdleForm* bashStartIdle;
    inline RE::TESIdleForm* bashReleaseIdle;
    inline RE::TESIdleForm* bashPowerStart;

    void initKeyword() { 
        bool expected = false;
        if (!kywdCache.compare_exchange_strong(expected, true)) {
            return;
        }
        auto* form = RE::TESForm::LookupByEditorID("apo_key_telescope");
        telescopeKYWD = form ? form->As<RE::BGSKeyword>() : nullptr;
        if (telescopeKYWD) {
            SKSE::log::info("[utils] Cached keyword apo_key_telescope ({:08X})", telescopeKYWD->GetFormID());
        } else {
            SKSE::log::info("[utils] Keyword apo_key_telescope not found");
        }
    }

    void init() {
        blockingStartIdle = RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESIdleForm>(0x00013217, "Skyrim.esm");
        bashStartIdle = RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESIdleForm>(0x0001B417, "Skyrim.esm");
        bashReleaseIdle = RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESIdleForm>(0x0001457A, "Skyrim.esm");
        bashPowerStart = RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESIdleForm>(0x000E8452, "Skyrim.esm");
        if (blockingStartIdle) {
            SKSE::log::info("[utils] Cached blockingStart ({:08X})", blockingStartIdle->GetFormID());
        } else {
            SKSE::log::info("[utils] failed to find blockingStart");
        }
        if (bashStartIdle) {
            SKSE::log::info("[utils] Cached bashStart ({:08X})", bashStartIdle->GetFormID());
        } else {
            SKSE::log::info("[utils] failed to find bashStart");
        }
        if (bashReleaseIdle) {
            SKSE::log::info("[utils] Cached bashRelease ({:08X})", bashReleaseIdle->GetFormID());
        } else {
            SKSE::log::info("[utils] failed to find bashRelease");
        }
        if (bashPowerStart) {
            SKSE::log::info("[utils] Cached bashPowerStart ({:08X})", bashPowerStart->GetFormID());
        } else {
            SKSE::log::info("[utils] failed to find bashPowerStart");
        }
    }

    bool hasTelescopeKeyword(const RE::TESObjectARMO* shield) {
        if (!shield) {
            return false;
        }
        if (!kywdCache.load(std::memory_order_acquire) || !telescopeKYWD) {
            return false;
        }

        return shield->HasKeyword(telescopeKYWD);
    }

    bool isLeftKeyBlock(RE::PlayerCharacter* player) {
        if (!player) return false;
        const auto* rightForm = player->GetEquippedObject(false);
        const auto* leftForm = player->GetEquippedObject(true);
        const auto* rightWeap = rightForm ? rightForm->As<RE::TESObjectWEAP>() : nullptr;
        const auto* leftWeap = leftForm ? leftForm->As<RE::TESObjectWEAP>() : nullptr;
        const auto* leftShield = leftForm ? leftForm->As<RE::TESObjectARMO>() : nullptr;

        // shield: always can block
        if (leftShield) {
            // log::info("shield equipped");
            if (hasTelescopeKeyword(leftShield)) {
                log::info("shield has telescope keyword");
                return false;
            }
            return true;
        }
        
        // you can't block without a weapon in the right hand
        if (!rightWeap) {
            // log::info("no right handed weapon");
            //verify it's unarmed, if MCO/BFCO we can block, if not:
            if (!rightForm && !settings::leftAttack()) {
                return true;
            }
            return false;
        }
        const auto rType = rightWeap->GetWeaponType();

        // 1: check if two handed
        if (rType >= RE::WeaponTypes::kTwoHandSword && rType <= RE::WeaponTypes::kTwoHandAxe) {
            // log::info("two handed weapon");
            return true;
        }

        // 2: if right hand isn't 1h and isn't 2h then we return false;
        const bool validRight = (rType >= RE::WeaponTypes::kHandToHandMelee && rType <= RE::WeaponTypes::kOneHandMace);
        if (!validRight) {
            // log::info("invalid right hand type");
            return false;
        }
        //so we haev a 1h weapon and empty left hand: return true
        if (!leftForm) {
            return true;
        }
        // now check left, make sure it's either unarmed or one handed
        if (leftWeap) {
            const auto lType = leftWeap->GetWeaponType();
            // if left = attack and right hand is 1h weapon then left hand must be empty
            //if (settings::leftAttack() && (lType != RE::WeaponTypes::kHandToHandMelee || !leftForm)) {
            if (settings::leftAttack()) {
                // log::info("left = attack, left hand is not unarmed/shield");
                return false;
            }
            if (!leftForm || (lType >= RE::WeaponTypes::kHandToHandMelee && lType <= RE::WeaponTypes::kOneHandMace)) {
                // log::info("valid right hand, valid left hand");
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
            if (hasTelescopeKeyword(leftShield)) {
                log::info("shield has telescope keyword");
                return false;  // telescope shields CANNOT alt-block
            }
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

    void consumeStamina(RE::PlayerCharacter* player, float amount) {
        if (!player) return;
        auto* actorAV = player->AsActorValueOwner();
        if (actorAV) {
            actorAV->DamageActorValue(RE::ActorValue::kStamina, amount);
            if (settings::log()) SKSE::log::info("Damaged stamina={}", amount);
        }
    }

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
        //  now we need to check if attacking
        auto* playerState = player->AsActorState();
        if (playerState) {
            const bool isPowerAttacking = player->IsPowerAttacking();
            if (isPowerAttacking) {
                consumeStamina(player, settings::PACancelCost());
                return;
            }
            const bool isAttacking = player->IsAttacking();
            if (isAttacking) {
                consumeStamina(player, settings::blockCancelCost());
                return;
            }
        }
    }

    //dont ask me wtf this is lmao
    //adapted from: https://github.com/jarari/DynamicKeyActionFramework
    bool tryIdle(RE::TESIdleForm* idle, RE::Actor* actor, RE::DEFAULT_OBJECT action = RE::DEFAULT_OBJECT::kActionIdle,
        RE::Actor* target = nullptr) {
        if (!idle || !actor) {
            return false;
        }

        auto* proc = actor->GetActorRuntimeData().currentProcess;
        if (!proc) {
            return false;
        }

        using func_t =
            bool (*)(RE::AIProcess*, RE::Actor*, RE::DEFAULT_OBJECT, RE::TESIdleForm*, bool, bool, RE::Actor*);
        static REL::Relocation<func_t> func{RELOCATION_ID(38290, 39256)};

        return func(proc, actor, action, idle, true, true, target);
    }

    bool tryBlockIdle(RE::PlayerCharacter* pc) {
        if (!pc || !blockingStartIdle) {
            return false;
        }
        return tryIdle(blockingStartIdle, pc);
    }

    bool tryBashStart(RE::PlayerCharacter* pc) {
        if (!pc || !bashStartIdle) {
            return false;
        }
        /*if (auto* st = pc->AsActorState()) {
            pc->NotifyAnimationGraph("blockStart");
            st->actorState2.wantBlocking = 1;
        }*/
        const bool success = tryIdle(bashStartIdle, pc);
        if (settings::log()) {
            if (success) {
                SKSE::log::info("[utils] bashStart Successful");
            } else {
                SKSE::log::info("[utils] bashStart Failed");
            }
        }
        return success;
    }

    bool tryBashRelease(RE::PlayerCharacter* pc) {
        if (!pc || !bashReleaseIdle) {
            return false;
        }
        //force IsAttacking boolean to false
        const bool success = tryIdle(bashReleaseIdle, pc);
        if (settings::log()) {
            if (success) {
                SKSE::log::info("[utils] bashRelease Successful");
            } else {
                SKSE::log::info("[utils] bashRelease Failed");
            }
        }
        return success;
    }

    bool tryBashPowerStart(RE::PlayerCharacter* pc) {
        if (!pc || !bashPowerStart) {
            return false;
        }
        // force IsAttacking boolean to false
        const bool success = tryIdle(bashPowerStart, pc);
        if (settings::log()) {
            if (success) {
                SKSE::log::info("[utils] bashPowerStart Successful");
            } else {
                SKSE::log::info("[utils] bashPowerStart Failed");
            }
        }
        return success;
    }

    //prioritize false, for more permissive release and stop blocking i think
    bool isPlayerBlocking() {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return false;
        }

        const bool stateSaysBlocking = player->IsBlocking();

        bool graphSaysBlocking = true;
        bool gotGraph = player->GetGraphVariableBool("IsBlocking", graphSaysBlocking);

        if (!stateSaysBlocking) {
            return false;
        }
        if (gotGraph && !graphSaysBlocking) {
            return false;
        }
        return true;
    }

    //more accurately - checks if you're allowed to block. shield = always so 
    bool isRightHandCaster(RE::PlayerCharacter* player) {
        if (!player) {
            return false;
        }

        const RE::TESForm* rightForm = player->GetEquippedObject(false);
        const auto* leftForm = player->GetEquippedObject(true);
        const auto* leftShield = leftForm ? leftForm->As<RE::TESObjectARMO>() : nullptr;

        // if you have left shield always can block, but you don't need alt block key in this case so uhhh?
        if (leftShield) {
            //if (hasTelescopeKeyword(leftShield)) {
            //    log::info("shield has telescope keyword");
            //    return true;  // telescope shields CANNOT alt-block
            //}
            // log::info("shield equipped");
            return false;
        }

        if (!rightForm) {
            return false;
        }
        if (auto* spell = rightForm->As<RE::MagicItem>()) {
            return true;
        }

        if (auto* weapon = rightForm->As<RE::TESObjectWEAP>()) {
            return weapon->GetWeaponType() == RE::WeaponTypes::WEAPON_TYPE::kStaff;
        }
        return false;
    }
}
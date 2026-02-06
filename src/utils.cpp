#include "PCH.h"

#include "settings.h"

using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

namespace utils {
    inline RE::BGSKeyword* telescopeKYWD = nullptr;
    inline std::atomic_bool kywdCache{false};
    inline RE::TESIdleForm* blockingStartIdle;
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
        if (blockingStartIdle) {
            SKSE::log::info("[utils] Cached blockingStart ({:08X})", blockingStartIdle->GetFormID());
        } else {
            SKSE::log::info("[utils] failed to find blockingStart");
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

        // now check left, make sure it's either unarmed or one handed
        if (leftWeap) {
            const auto lType = leftWeap->GetWeaponType();
            // if left = attack and right hand is 1h weapon then left hand must be empty
            if (settings::leftAttack() && lType != RE::WeaponTypes::kHandToHandMelee) {
                // log::info("left = attack, left hand is not unarmed/shield");
                return false;
            }
            if (lType >= RE::WeaponTypes::kHandToHandMelee && lType <= RE::WeaponTypes::kOneHandMace) {
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

    void consumeStamina(RE::PlayerCharacter* player) {
        if (!player) return;
        auto* actorAV = player->AsActorValueOwner();
        if (actorAV) {
            actorAV->DamageActorValue(RE::ActorValue::kStamina, settings::blockCancelCost());
            SKSE::log::info("Damaged stamina={}", settings::blockCancelCost());
        }
    }

    //returns true if block is allowed, false if not. 
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
            const bool isAttacking = player->IsAttacking();
            if (isAttacking) {
                consumeStamina(player);
            }
            return;
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

    bool isRightHandCaster(RE::PlayerCharacter* player) {
        if (!player) {
            return false;
        }

        const RE::TESForm* rightForm = player->GetEquippedObject(false);

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
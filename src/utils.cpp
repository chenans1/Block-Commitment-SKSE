#include "PCH.h"

#include "settings.h"

using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

namespace utils {
    //check for equip. Assume mco-style left clicks
    //thus if left hand is weapon, yes, also useful as a check for 1h+spell
    bool isLeftKeyBlock(RE::PlayerCharacter* player) {
        if (!player) return false;
        const auto* rightForm = player->GetEquippedObject(false);
        const auto* leftForm = player->GetEquippedObject(true);
        const auto* rightWeap = rightForm ? rightForm->As<RE::TESObjectWEAP>() : nullptr;
        const auto* leftWeap = leftForm ? leftForm->As<RE::TESObjectWEAP>() : nullptr;
        const auto* leftShield = leftForm ? leftForm->As<RE::TESObjectARMO>() : nullptr;

        //you can't block without a weapon in the right hand
        if (!rightWeap) {
            log::info("no right handed weapon");
            return false;
        }
        
        const auto rType = rightWeap->GetWeaponType();

        //1: check if two handed
        if (rType >= RE::WeaponTypes::kTwoHandSword && rType <= RE::WeaponTypes::kTwoHandAxe) {
            log::info("two handed weapon");
            return true;
        }

        //2: if right hand isn't 1h and isn't 2h then we return false;
        const bool validRight = (rType >= RE::WeaponTypes::kHandToHandMelee && rType <= RE::WeaponTypes::kOneHandMace);
        if (!validRight) {
            log::info("invalid right hand type");
            return false;
        }
        //3: now we always have a valid right hand weapon in this case
        if (leftShield) {
            log::info("shield equipped");
            return true;
        }

        //now check left, make sure it's either unarmed or one handed
        if (leftWeap) {
            const auto lType = leftWeap->GetWeaponType();
            //if left = attack and right hand is 1h weapon then left hand must be empty
            if (settings::leftAttack() && lType != RE::WeaponTypes::kHandToHandMelee) {
                log::info("left = attack, left hand is not unarmed/shield");
                return false;
            }
            if (lType >= RE::WeaponTypes::kHandToHandMelee && lType <= RE::WeaponTypes::kOneHandMace) {
                log::info("valid right hand, valid left hand");
                return true;
            }
        }
        //at this point if there is no left weapon type then we know it's probably a spell in the left hand
        return false;
    }
}
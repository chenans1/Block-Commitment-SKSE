#pragma once

#include "soundHandler.h"

namespace utils {

    //hooks, addresses are adapted from valhalla combat
    inline REL::Relocation<float*> g_deltaTime{RELOCATION_ID(523660, 410199)};
    inline REL::Relocation<float*> g_deltaTimeRealTime{RELOCATION_ID(523661, 410200)};

    inline float DeltaTime() {
        auto* p = g_deltaTime.get();
        return p ? *p : 0.0f;
    }

    inline float RealTimeDelta() {
        auto* p = g_deltaTimeRealTime.get();
        return p ? *p : 0.0f;
    }
    bool hasTelescopeKeyword(const RE::TESObjectARMO* shield);
    bool isLeftKeyBlock(RE::PlayerCharacter* player);
    bool canAltBlock(RE::PlayerCharacter* player);

    //check if player attacking test
    bool isPlayerAttacking(RE::PlayerCharacter* player);
    void resolveBlockCancel(RE::PlayerCharacter* player);

    void initKeyword();
    void init();

    bool tryIdle(RE::TESIdleForm* idle, RE::Actor* actor, RE::DEFAULT_OBJECT action = RE::DEFAULT_OBJECT::kActionIdle,RE::Actor* target = nullptr);
    
    bool tryBlockIdle(RE::PlayerCharacter* pc);
    bool tryBashStart(RE::PlayerCharacter* pc);
    bool tryBashRelease(RE::PlayerCharacter* pc);

    bool isPlayerBlocking();
    
    bool isRightHandCaster(RE::PlayerCharacter* player);

    inline void consumeStamina(RE::PlayerCharacter* player);

    inline bool isWard(const RE::SpellItem* spell) {
        if (!spell) {
            return false;
        }
        static const auto* wardKYWD = RE::TESForm::LookupByID<RE::BGSKeyword>(0x0001EA69);
        if (!wardKYWD) {
            SKSE::log::info("Warning: No MagicWard Keyword [0x0001EA69] found!");
            return false;
        }
        for (auto* effect : spell->effects) {
            if (!effect) continue;
            auto* mgef = effect->baseEffect;
            if (!mgef) continue; 
            if (mgef->HasKeyword(wardKYWD)) return true;
        }
        return false;
    }

    //only checks for the first favorite ward, dgaf about everything else
    inline RE::SpellItem* findFavoriteWard() {
        auto* fav = RE::MagicFavorites::GetSingleton();
        if (!fav) return nullptr;
        for (auto* form : fav->spells) {
            if (!form) continue;
            auto* spell = form->As<RE::SpellItem>();
            if (!spell) continue;
            if (isWard(spell)) return spell;
        }
        return nullptr;
    }

    inline void castWardSpell(RE::PlayerCharacter* player) {
        auto* spell = findFavoriteWard();
        //test something weird: attempt to cast it as a fire and forget spell
        if (!spell) {
            RE::SendHUDMessage::ShowHUDMessage("No favorited ward spell found.", nullptr, true);
            return;
        }

        auto* caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kLeftHand);
        if (!caster) {
            return;
        }
        //check if the ward is fire and forget or not - in case someone uses a mod that does this
        const auto wardType = spell->GetCastingType();
        if (wardType == RE::MagicSystem::CastingType::kFireAndForget) {
            //if fire and forget ward we need to deduct magicka cost manually
            auto* actorAV = player->AsActorValueOwner();
            if (actorAV) {
                const float spellcost = spell->CalculateMagickaCost(player);
                if (spellcost >= actorAV->GetActorValue(RE::ActorValue::kMagicka)) {
                    RE::HUDMenu::FlashMeter(RE::ActorValue::kMagicka);
                    RE::SendHUDMessage::ShowHUDMessage("Not Enough Magicka To Cast Ward", nullptr, true);
                    return;
                }
                actorAV->DamageActorValue(RE::ActorValue::kMagicka, spell->CalculateMagickaCost(player));
            }
        }

        caster->CastSpellImmediate(spell, false, player, 1.0f, false, 1.0f, player);
        // send spell caster event sink
        if (auto ScriptEventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton()) {
            if (auto RefHandle = player->CreateRefHandle()) {
                ScriptEventSourceHolder->SendSpellCastEvent(RefHandle.get(), spell->formID);
            }
        }
        const auto wardSound = sound::GetMGEFSound(spell, RE::MagicSystem::SoundID::kRelease);
        sound::play_sound(player, wardSound);
    }
}
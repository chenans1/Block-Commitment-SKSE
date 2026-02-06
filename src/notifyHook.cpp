#include "PCH.h"
#include "notifyHook.h"
#include "blockCommit.h"
#include "settings.h"
#include "utils.h"

namespace notify {
    void castWardSpell(RE::PlayerCharacter* player) { 
        auto* spell = utils::findFavoriteWard();
        if (!spell) {
            RE::SendHUDMessage::ShowHUDMessage("No favorited ward spell found.", nullptr, true);
            return;
        }

        auto* caster = player->GetMagicCaster(RE::MagicSystem::CastingSource::kLeftHand);
        if (!caster) {
            RE::SendHUDMessage::ShowHUDMessage("Error: No MagicCaster in left hand", nullptr, true);
            return;
        }
        caster->CastSpellImmediate(spell, true, player, 1.0f, false, 0.0f, player);

        std::string msg = std::string("Casting ward: ") + (spell->GetName() ? spell->GetName() : "<unnamed>");
        RE::SendHUDMessage::ShowHUDMessage(msg.c_str(), nullptr, true);
    }

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
            if (a_eventName == "blockStart") {
                blockCommit::Controller::GetSingleton()->beginAltBlock();
                if (settings::isBlockCancelEnabled()) {
                    if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                        resolveBlockCancel(player);
                        //ward casting test
                        if (utils::isRightHandCaster(player)) {
                            castWardSpell(player);
                        }
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
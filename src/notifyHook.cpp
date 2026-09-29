#include "PCH.h"
#include "notifyHook.h"
#include "blockCommit.h"
#include "settings.h"
#include "utils.h"

//ADXP_MCO_DXP.esp ~ 0x80D
bool IsMCOBlockCancelEnabled() {
    auto* setting = RE::TESForm::LookupByEditorID<RE::TESGlobal>("MCO_bEnableBlockCancel");
    return setting && setting->value != 0.0f;
}

namespace notify {
    static bool alreadyConsumed = false;
    bool PC_NotifyAnimationGraph(RE::IAnimationGraphManagerHolder* a_this, const RE::BSFixedString& a_eventName) {
        static const RE::BSFixedString blockStart{ "blockStart" }; 
        static const RE::BSFixedString blockStop{ "blockStop" }; 
        static const RE::BSFixedString bashStart{ "bashStart" }; 
        static auto* const player = RE::PlayerCharacter::GetSingleton();

        // force the mco block cancel variable to true and then just check if we are allowed to process the event
        if (settings::fixMCOAttackCancel() && a_eventName == blockStart) {
            if (player->SetGraphVariableBool("MCO_bEnableBlockCancel", true)) {
                // bool MCO_IsInRecovery = false;
                bool inRecovery = false;
                const bool hasRecoveryVariable = player->GetGraphVariableBool("MCO_IsInRecovery", inRecovery);
                if (player->IsAttacking() && hasRecoveryVariable && !inRecovery) {
                    if (!IsMCOBlockCancelEnabled()) {
                        SKSE::log::info("[BlockCancelFix]: denying blockStart");
                        return false;
                    }
                }
            }
        }

        const bool result = _PC_NotifyAnimationGraph(a_this, a_eventName);
        if (!result) return result;

        if (a_eventName == blockStart) {
            blockCommit::Controller::GetSingleton()->onBlockStart();
            if (settings::isBlockCancelEnabled() && player->IsBlocking()) {
                utils::resolveBlockCancel(player);
                // if (!alreadyConsumed) {
                //     utils::resolveBlockCancel(player);
                //     alreadyConsumed = true;
                // }
            }
            
        } else if (a_eventName == blockStop) {      
            // if (player->IsBlocking()) {
            //     alreadyConsumed = false;
            // }
            blockCommit::Controller::GetSingleton()->reset();
            if (settings::mageBlock() && settings::mageWard() && utils::isRightHandCaster(player)) {
                player->InterruptCast(true);
            }
        } 
        // else if (a_eventName == bashStart) {
        //     if (auto* st = player->AsActorState()) {
        //         st->actorState2.wantBlocking = 0; 
        //     }
        //     player->NotifyAnimationGraph("blockStop");
        // }
        
        return result;
    }

    void Install() {
        SKSE::log::info("Installing PlayerCharacter animation graph hook...");

        REL::Relocation<uintptr_t> PlayerCharacter_IAnimationGraphManagerHolderVtbl{RE::VTABLE_PlayerCharacter[3]};
        _PC_NotifyAnimationGraph = PlayerCharacter_IAnimationGraphManagerHolderVtbl.write_vfunc(0x1, PC_NotifyAnimationGraph);

        SKSE::log::info("PlayerCharacter animation graph hook installed successfully");
    }

}

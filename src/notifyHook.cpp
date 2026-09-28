#include "PCH.h"
#include "notifyHook.h"
#include "blockCommit.h"
#include "settings.h"
#include "utils.h"

namespace notify {
    bool PC_NotifyAnimationGraph(RE::IAnimationGraphManagerHolder* a_this, const RE::BSFixedString& a_eventName) {
        const bool result = _PC_NotifyAnimationGraph(a_this, a_eventName);
        static const RE::BSFixedString blockStart{ "blockStart" }; 
        static const RE::BSFixedString blockStop{ "blockStop" }; 
        static const RE::BSFixedString bashStart{ "bashStart" }; 
        static auto* const player = RE::PlayerCharacter::GetSingleton();
        if (!result) return result;
        if (a_eventName == blockStart) {
            blockCommit::Controller::GetSingleton()->beginAltBlock();
            if (settings::isBlockCancelEnabled()) {
                utils::resolveBlockCancel(player);
            }
            
        } else if (a_eventName == blockStop) {            
            blockCommit::Controller::GetSingleton()->reset();
            if (settings::mageBlock() && settings::mageWard() && utils::isRightHandCaster(player)) {
                player->InterruptCast(true);
            }
        } else if (a_eventName == bashStart) {
            if (auto* st = player->AsActorState()) {
                st->actorState2.wantBlocking = 0; 
            }
            player->NotifyAnimationGraph("blockStop");

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
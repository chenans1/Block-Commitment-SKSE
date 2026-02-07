#include "PCH.h"
#include "notifyHook.h"
#include "blockCommit.h"
#include "settings.h"
#include "utils.h"

namespace notify {
    bool PC_NotifyAnimationGraph(RE::IAnimationGraphManagerHolder* a_this, const RE::BSFixedString& a_eventName) {
        const bool result = _PC_NotifyAnimationGraph(a_this, a_eventName);

        if (a_eventName == "blockStart") {
            blockCommit::Controller::GetSingleton()->beginAltBlock();
            if (settings::isBlockCancelEnabled()) {
                if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                    utils::resolveBlockCancel(player);
                }
            }
        } else if (a_eventName == "blockStop") {            
            if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                bool isBlocking = true;
                if (player->GetGraphVariableBool("IsBlocking", isBlocking) && !isBlocking) {
                    blockCommit::Controller::GetSingleton()->reset();
                }
                if (settings::mageBlock() && utils::isRightHandCaster(player)) {
                    player->InterruptCast(true);
                }
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
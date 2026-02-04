#include "PCH.h"

#include "altBlockCommit.h"
#include "altBlock.h"
#include "settings.h"

namespace altCommit {
    static void stopBlocking() {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return;
        }

        // if fail to fetch the variable dont process, if already not blocking don't send
        bool isBlocking = false;
        if (!player->GetGraphVariableBool("IsBlocking", isBlocking)) {
            return;
        }

        if (isBlocking) {
            if (auto* st = player->AsActorState()) st->actorState2.wantBlocking = 0;
            if (settings::log()) SKSE::log::info("Delayed AltBlock Stopped");
            player->NotifyAnimationGraph("blockStop");
        }
        return;
    }

    altController* altController::GetSingleton() {
        static altController singleton;
        return std::addressof(singleton);
    }

	void altController::delayedBlockStop(float duration) {
        //set our remaining to duration
        _pending.wantStop = true;
        _pending.remaining = duration;
	}

    void altController::Update(float a_delta) {
        SKSE::log::info("[altController] Update()");
        //only start decrementing if we want to stop, otherwise don't do anything
        if (!_pending.wantStop) {
            return;
        }
        //substract the timer
        _pending.remaining -= a_delta;
        if (_pending.remaining > 0.0f) {
            return;
        }

        //now this means we can just unblock 
        _pending.wantStop = false;
        stopBlocking();
        return;
   }
}
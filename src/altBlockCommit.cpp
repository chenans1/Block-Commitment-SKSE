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
            if (settings::log()) SKSE::log::info("Delayed alt blockStop fired");
            player->NotifyAnimationGraph("blockStop");
        }
        return;
    }

    altController* altController::GetSingleton() {
        static altController singleton;
        return std::addressof(singleton);
    }

    void altController::beginAltBlock() { 
        _pending.wantStop = false;
        _pending.blockDuration = 0.0f;
    }

	void altController::wantReleaseBlock() {
        //set our remaining to duration
        _pending.wantStop = true;
	}

    void altController::Update(float a_delta) {
        //SKSE::log::info("[altController] Update()");
        _pending.blockDuration += a_delta;
        //if we don't want to stop blocking, increment the counter
        if (!_pending.wantStop) {
            return;
        }
        //if we want to stop blocking: check if blockduration > commit duration
        //if true allow unblock, if not do nothing
        if (_pending.blockDuration >= settings::getCommitDur()) {
            _pending.wantStop = false;
            stopBlocking();
        }
        return;
   }
}
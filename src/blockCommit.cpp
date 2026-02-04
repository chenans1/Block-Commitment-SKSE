#include "PCH.h"

#include "BlockCommit.h"
#include "altBlock.h"
#include "settings.h"

namespace blockCommit {
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

    Controller* Controller::GetSingleton() {
        static Controller singleton;
        return std::addressof(singleton);
    }

    void Controller::beginAltBlock() { 
        _state.altBlockMode = true;
        _state.wantStop = false;
        _state.blockDuration = 0.0f;
    }

	void Controller::wantReleaseAltBlock() {
        //set our remaining to duration
        _state.wantStop = true;
	}

    //use pending altblock mode to prevent mixing up block input
    //fetch the input key types here
    void Controller::beginLeftBlock() {
        _state.altBlockMode = false;
        _state.wantStop = false;
        _state.blockDuration = 0.0f;
    }

    //returns true/false to decide if we swallow input
    bool Controller::wantReleaseLeftBlock() { 
        //check blockduration >= blockcommit. if true reset state machine and continue
        if (_state.blockDuration >= settings::getCommitDur()) {
            _state.blockDuration = 0.0f;
            return false;
        }
        //if we haven't held long enough, just set the flag to true
        _state.wantStop = true;
        return true;
    }

    void Controller::Update(float a_delta) {
        //SKSE::log::info("[altController] Update()");
        _state.blockDuration += a_delta;
        //if we don't want to stop blocking, increment the counter
        if (!_state.wantStop) {
            return;
        }
        //first check which mode we are in:
        if (_state.altBlockMode) {
            // if we want to stop blocking: check if blockduration > commit duration
            // if true allow unblock, if not do nothing
            if (_state.blockDuration >= settings::getCommitDur()) {
                _state.wantStop = false;
                stopBlocking();
            }
            return;
        }
        return;
   }
}
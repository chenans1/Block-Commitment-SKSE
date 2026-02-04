#include "PCH.h"

#include "blockCommit.h"
#include "altBlock.h"
#include "settings.h"

namespace blockCommit {
    static void stopBlocking() {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return;
        }

        if (auto* st = player->AsActorState()) st->actorState2.wantBlocking = 0;
        if (settings::log()) SKSE::log::info("[blockCommit]: delayed blockStop Fired");
        player->NotifyAnimationGraph("blockStop");
        return;
    }

    Controller* Controller::GetSingleton() {
        static Controller singleton;
        return std::addressof(singleton);
    }

    void Controller::beginAltBlock() {
        if (settings::log()) SKSE::log::info("[blockCommit]: beginAltBlock");
        _state.altBlockMode = true;
        _state.wantStop = false;
        _state.blockDuration = 0.0f;
    }

	void Controller::wantReleaseAltBlock() {
        if (_state.blockDuration >= settings::getCommitDur()) {
            if (settings::log()) {
                SKSE::log::info("[wantReleaseAltBlock] allow release: block duration={}", _state.blockDuration);
            }
            _state.blockDuration = 0.0f;
            return;
        }
        _state.wantStop = true;
        if (settings::log()) {
            SKSE::log::info("[wantReleaseAltBlock]: block duration={}, remaining={}", 
                _state.blockDuration, (settings::getCommitDur() - _state.blockDuration));
        }
	}

    //use pending altblock mode to prevent mixing up block input
    //fetch the input key types here
    void Controller::beginLeftBlock() {
        if (settings::log()) SKSE::log::info("[blockCommit]: beginLeftBlock");
        _state.altBlockMode = false;
        _state.wantStop = false;
        _state.blockDuration = 0.0f;
    }

    //returns true/false to decide if we swallow input
    bool Controller::wantReleaseLeftBlock() { 
        //check blockduration >= blockcommit. if true reset state machine and continue
        if (_state.blockDuration >= settings::getCommitDur()) {
            if (settings::log()) {
                SKSE::log::info("[wantReleaseLeftBlock] allow release: block duration={}", _state.blockDuration);
            }
            _state.blockDuration = 0.0f;
            return false;
        }
        //if we haven't held long enough, just set the flag to true
        _state.wantStop = true;
        if (settings::log()) {
            SKSE::log::info("[wantReleaseLeftBlock]: block duration={}, remaining={}", 
                _state.blockDuration, (settings::getCommitDur() - _state.blockDuration));
        }
        return true;
    }

    //invoked from the player update hook
    void Controller::Update(float a_delta) {
        //SKSE::log::info("[altController] Update()");
        _state.blockDuration += a_delta;
        //if we don't want to stop blocking, increment duration and do nothing.
        if (!_state.wantStop) {
            return;
        }
        
        // if we want to stop blocking: check if blockduration > commit duration
        // if true allow unblock, if not do nothing
        if (_state.blockDuration >= settings::getCommitDur()) {
            _state.wantStop = false;
            stopBlocking();
            return;
        }
   }
}
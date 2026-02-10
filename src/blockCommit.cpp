#include "PCH.h"

#include "blockCommit.h"
#include "altBlock.h"
#include "settings.h"
#include "utils.h"

namespace blockCommit {
    void Controller::stopBlocking() {
        _state.wantStop = false;
        if (auto* player = RE::PlayerCharacter::GetSingleton()) {
            if (auto* st = player->AsActorState()) {
                if (player->IsBlocking()) {
                    player->NotifyAnimationGraph("blockStop");
                    st->actorState2.wantBlocking = 0;
                    _state.blockDuration = 0.0f;
                    if (settings::log()) SKSE::log::info("[blockCommit]: delayed blockStop Fired");
                    return;
                }
            }
        }
        return;
    }

    Controller* Controller::GetSingleton() {
        static Controller singleton;
        return std::addressof(singleton);
    }

    void Controller::beginAltBlock() {
        if (!settings::blockCommitOn()) {
            if (settings::log()) SKSE::log::info("[blockCommit]: beginAltBlock do nothing, block commit off");
            return;
        }
        if (settings::log()) SKSE::log::info("[blockCommit]: beginAltBlock");
        _state.altBlockMode = true;
        _state.wantStop = false;
        _state.blockDuration = 0.0f;
    }

    // use pending altblock mode to prevent mixing up block input
    // fetch the input key types here
    void Controller::beginLeftBlock() {
        if (!settings::blockCommitOn()) {
            if (settings::log()) SKSE::log::info("[blockCommit]: beginLeftBlock do nothing, block commit off");
            return;
        }
        if (settings::log()) SKSE::log::info("[blockCommit]: beginLeftBlock");
        _state.altBlockMode = false;
        _state.wantStop = false;
        _state.blockDuration = 0.0f;
    }

	void Controller::wantReleaseAltBlock() {
        //just release if blockCommit is OFF
        if (_state.blockDuration >= settings::getCommitDur() || !settings::blockCommitOn()) {
            if (settings::log()) {
                if (settings::blockCommitOn()) {
                    SKSE::log::info("[wantReleaseAltBlock] allow release: block duration={}", _state.blockDuration);
                } else {
                    SKSE::log::info("[wantReleaseAltBlock] allow release: block commit is off");
                }
            }
            _state.wantStop = false;
            _state.blockDuration = 0.0f;
            stopBlocking();
            return;
        }
        _state.wantStop = true;
        if (settings::log()) {
            SKSE::log::info("[wantReleaseAltBlock]: block duration={}, remaining={}", 
                _state.blockDuration, (settings::getCommitDur() - _state.blockDuration));
        }
	}

    //returns true/false to decide if we swallow input
    bool Controller::wantReleaseLeftBlock() { 
        //if block commit off: always return false
        if (!settings::blockCommitOn()) {
            if (settings::log()) {
                SKSE::log::info("[wantReleaseLeftBlock] allow release: block commit is off");
            }
            _state.wantStop = false;
            _state.blockDuration = 0.0f;
            return false;
        }
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (player && !player->IsBlocking()) {
            if (settings::log()) {
                SKSE::log::info("[wantReleaseLeftBlock] Player is not blocking");
            }
            _state.wantStop = false;
            _state.blockDuration = 0.0f;
            return false;
        }
        if (_state.blockDuration >= settings::getCommitDur()) {
            if (settings::log()) {
                SKSE::log::info("[wantReleaseLeftBlock] allow release: block duration={}", _state.blockDuration);
            }
            _state.wantStop = false;
            _state.blockDuration = 0.0f;
            return false;
        }
        //if we haven't held long enough, just set the flag to true
        if (settings::log()) {
            SKSE::log::info("[wantReleaseLeftBlock]: block duration={}, remaining={}", 
                _state.blockDuration, (settings::getCommitDur() - _state.blockDuration));
        }
        _state.wantStop = true;
        return true;
    }

    void Controller::reset() {
        /*if (!utils::isPlayerBlocking)*/
        _state.wantStop = false;
        _state.blockDuration = 0.0f;
    }

    //invoked from the player update hook
    void Controller::Update(float a_delta) {
        //SKSE::log::info("[altController] Update()");
        const auto* player = RE::PlayerCharacter::GetSingleton();
        if (player->IsBlocking()) {
            _state.blockDuration += a_delta;
        }
        
        // if we want to stop blocking: check if blockduration > commit duration
        // if true allow unblock, if not do nothing
        if (_state.wantStop && _state.blockDuration >= settings::getCommitDur()) {
            stopBlocking();
            return;
        }
   }
}
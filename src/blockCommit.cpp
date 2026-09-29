#include "PCH.h"

#include "blockCommit.h"
#include "settings.h"

namespace blockCommit {
    Controller* Controller::GetSingleton() {
        static Controller singleton;
        return std::addressof(singleton);
    }

    bool Controller::releaseReady(const Commitment& commitment) const {
        return !settings::blockCommitOn() || commitment.elapsed >= settings::getCommitDur();
    }

    void Controller::updateWantBlocking(RE::PlayerCharacter* player) {
        // Released alt block keeps bash input active until the blocking graph exits.
        if (player) {
            if (auto* state = player->AsActorState()) {
                state->actorState2.wantBlocking = (_leftKeyHeld || _altKeyHeld || _altWaitingForBlockEnd) ? 1 : 0;
            }
        }
    }

    void Controller::OnLeftBlockDown(RE::ButtonEvent* event) {
        if (!event) return;
        _leftDevice = event->GetDevice();
        _leftIdCode = event->GetIDCode();
        _leftKeyHeld = true;
        _leftReleaseRequested = false;
        _left = {};
        if (settings::log()) SKSE::log::info("[blockCommit]: left/block key pressed");
    }

    bool Controller::OnLeftBlockUp(float heldDuration) {
        if (!_leftKeyHeld) return false;
        _leftKeyHeld = false;
        _left = {};
        _leftReleaseRequested = false;
        auto* player = RE::PlayerCharacter::GetSingleton();
        const bool wasBlocking = player && player->IsBlocking();
        // The native handler still needs wantBlocking when it receives the real or delayed key-up.

        _left.elapsed = heldDuration;
        if (releaseReady(_left)) {
            return false;
        }
        if (!wasBlocking) {
            return false;
        }
        _left.releasePending = true;
        if (settings::log()) {
            SKSE::log::info("[blockCommit]: left release pending, remaining={}", settings::getCommitDur() - heldDuration);
        }
        return true;
    }

    bool Controller::TryInjectLeftRelease(ProcessButton processButton) {
        if (!_leftReleaseRequested || _leftKeyHeld || !processButton) return false;
        auto* controls = RE::PlayerControls::GetSingleton();
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!controls || !controls->attackBlockHandler || !player) return false;

        auto* release = RE::ButtonEvent::Create(_leftDevice, "Left Attack/Block", _leftIdCode, 0.0f, 0.0f);
        if (!release) return false;

        processButton(controls->attackBlockHandler, release, std::addressof(controls->data));
        OnLeftReleaseForwarded(player);
        RE::free(release);
        _leftReleaseRequested = false;
        if (settings::log()) SKSE::log::info("[blockCommit]: injected delayed left/block release");
        return true;
    }

    void Controller::OnLeftReleaseForwarded(RE::PlayerCharacter* player) {
        if (!player || _leftKeyHeld) return;
        updateWantBlocking(player);
        if (settings::log()) {
            SKSE::log::info("[blockCommit]: left release forwarded, graph blocking={}, wantBlocking={}",
                player->IsBlocking(), _altKeyHeld || _altWaitingForBlockEnd);
        }
    }

    void Controller::beginAltBlock() {
        auto* player = RE::PlayerCharacter::GetSingleton();
        const bool wasBlocking = player && player->IsBlocking();
        bool wantedBlocking = false;
        if (player) {
            if (auto* state = player->AsActorState()) {
                wantedBlocking = state->actorState2.wantBlocking != 0;
                // Start a new alt press from a clean block request. The input
                // handler sets this again only after starting the new block.
                state->actorState2.wantBlocking = 0;
            }
        }
        const bool hadPendingRelease = _alt.releasePending || _altWaitingForBlockEnd;
        _altKeyHeld = true;
        _altWaitingForBlockEnd = false;
        _alt = {};
        if (settings::log()) {
            SKSE::log::info("[blockCommit]: beginAltBlock, prior graph blocking={}, wantBlocking={}, pending release={}",
                wasBlocking, wantedBlocking, hadPendingRelease);
        }
    }

    void Controller::onBlockStart() {
        if (_altKeyHeld && !_alt.releasePending) {
            _alt.elapsed = 0.0f;
        }
    }

    void Controller::wantReleaseAltBlock() {
        _altKeyHeld = false;
        auto* player = RE::PlayerCharacter::GetSingleton();
        const bool wasBlocking = player && player->IsBlocking();
        _altWaitingForBlockEnd = wasBlocking;
        updateWantBlocking(player);
        if (settings::log()) {
            SKSE::log::info("[blockCommit]: alt key released, graph blocking={}", wasBlocking);
        }
        if (!wasBlocking || releaseReady(_alt)) {
            stopAltBlocking();
            return;
        }

        _alt.releasePending = true;
        if (settings::log()) {
            SKSE::log::info("[blockCommit]: alt release pending, remaining={}", settings::getCommitDur() - _alt.elapsed);
        }
    }

    void Controller::stopAltBlocking() {
        _alt = {};
        _altKeyHeld = false;
        auto* player = RE::PlayerCharacter::GetSingleton();
        const bool wasBlocking = player && player->IsBlocking();
        if (player) {
            if (auto* state = player->AsActorState()) {
                if (wasBlocking) {
                    player->NotifyAnimationGraph("blockStop");
                    if (settings::log()) SKSE::log::info("[blockCommit]: delayed blockStop fired");
                }
            }
        }
        _altWaitingForBlockEnd = false;
        updateWantBlocking(player);
        if (settings::log() && !wasBlocking) {
            SKSE::log::info("[blockCommit]: alt release completed after graph exit");
        }
    }

    void Controller::reset() {
        // A bash or another animation may stop blocking before release is due.
        if (_altWaitingForBlockEnd) {
            _altWaitingForBlockEnd = false;
            updateWantBlocking(RE::PlayerCharacter::GetSingleton());
            if (settings::log()) SKSE::log::info("[blockCommit]: alt release state cleared on blockStop");
        }
        _alt = {};
    }

    void Controller::Update(float a_delta) {
        auto* player = RE::PlayerCharacter::GetSingleton();
        const bool isBlocking = player && player->IsBlocking();
        if (_alt.releasePending && !isBlocking) {
            stopAltBlocking();
        } else if (_altWaitingForBlockEnd && !isBlocking) {
            _altWaitingForBlockEnd = false;
            updateWantBlocking(player);
            if (settings::log()) SKSE::log::info("[blockCommit]: alt release state cleared on graph exit");
        }

        if (a_delta > 0.0f) {
            if (_left.releasePending) {
                _left.elapsed += a_delta;
            }
            if ((_altKeyHeld || _alt.releasePending) && isBlocking) {
                _alt.elapsed += a_delta;
            }
        }

        if (_left.releasePending && releaseReady(_left)) {
            _left.releasePending = false;
            if (!_leftKeyHeld) {
                _leftReleaseRequested = true;
            }
        }
        if (_alt.releasePending && releaseReady(_alt)) {
            stopAltBlocking();
        }
    }
}

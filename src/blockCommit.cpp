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
        // Released alt block keeps bash input active until its committed stop.
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

    bool Controller::OnLeftBlockUp() {
        if (!_leftKeyHeld) return false;
        _leftKeyHeld = false;
        _left.releasePending = false;
        _leftReleaseRequested = false;
        auto* player = RE::PlayerCharacter::GetSingleton();
        const bool wasBlocking = player && player->IsBlocking();
        // The native handler still needs wantBlocking when it receives the real or delayed key-up.

        if (releaseReady(_left)) {
            return false;
        }
        if (!wasBlocking) {
            return false;
        }
        _left.releasePending = true;
        if (settings::log()) {
            SKSE::log::info("[blockCommit]: left release pending, remaining={}", settings::getCommitDur() - _left.elapsed);
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
        _altBlockRequested = false;
        _altWaitingForBlockEnd = false;
        _alt = {};
        if (settings::log()) {
            bool graphBlocking = false;
            const bool hasGraphState = player && player->GetGraphVariableBool("IsBlocking", graphBlocking);
            SKSE::log::info("[blockCommit]: beginAltBlock, prior actor blocking={}, graph blocking={}, graph state available={}, wantBlocking={}, pending release={}",
                wasBlocking, graphBlocking, hasGraphState, wantedBlocking, hadPendingRelease);
        }
    }

    void Controller::onAltBlockRequested() {
        _altBlockRequested = true;
    }

    void Controller::onBlockStart() {
        if (_leftKeyHeld && !_left.releasePending) {
            _left.elapsed = 0.0f;
        }
        if (_altKeyHeld && !_alt.releasePending) {
            _alt.elapsed = 0.0f;
        }
    }

    void Controller::wantReleaseAltBlock() {
        _altKeyHeld = false;
        auto* player = RE::PlayerCharacter::GetSingleton();
        const bool wasBlocking = player && player->IsBlocking();
        _altWaitingForBlockEnd = _altBlockRequested;
        updateWantBlocking(player);
        if (settings::log()) {
            SKSE::log::info("[blockCommit]: alt key released, actor blocking={}, block requested={}",
                wasBlocking, _altBlockRequested);
        }
        if (!_altBlockRequested || releaseReady(_alt)) {
            stopAltBlocking();
            return;
        }

        _alt.releasePending = true;
        if (settings::log()) {
            SKSE::log::info("[blockCommit]: alt release pending, remaining={}", settings::getCommitDur() - _alt.elapsed);
        }
    }

    void Controller::stopAltBlocking() {
        const bool hadAltBlockRequest = _altBlockRequested;
        _alt = {};
        _altKeyHeld = false;
        _altBlockRequested = false;
        _altWaitingForBlockEnd = false;
        auto* player = RE::PlayerCharacter::GetSingleton();
        const bool wasBlocking = player && player->IsBlocking();
        // Clear the block request before sending blockStop so its handler sees
        // the released state and cannot continue the alt block.
        updateWantBlocking(player);
        if (player) {
            if (hadAltBlockRequest && !_leftKeyHeld) {
                const bool accepted = player->NotifyAnimationGraph("blockStop");
                if (settings::log()) {
                    bool graphBlocking = false;
                    const bool hasGraphState = player->GetGraphVariableBool("IsBlocking", graphBlocking);
                    SKSE::log::info("[blockCommit]: delayed blockStop accepted={}, actor blocking before={}, actor blocking after={}, graph blocking={}, graph state available={}",
                        accepted, wasBlocking, player->IsBlocking(), graphBlocking, hasGraphState);
                }
            }
        }
        if (settings::log() && !hadAltBlockRequest) {
            SKSE::log::info("[blockCommit]: alt release completed without a block request");
        }
    }

    void Controller::reset() {
        // An interrupted attack can briefly leave the blocking state before
        // the committed alt release is due. Keep its scheduled blockStop.
        if (_alt.releasePending) return;
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

        if (a_delta > 0.0f) {
            if ((_leftKeyHeld && isBlocking) || _left.releasePending) {
                _left.elapsed += a_delta;
            }
            if ((_altKeyHeld && isBlocking) || _alt.releasePending) {
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

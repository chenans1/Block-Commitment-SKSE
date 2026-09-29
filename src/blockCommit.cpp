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
        // Once the release has been forwarded, graph blocking alone cannot keep bash input enabled.
        if (player) {
            if (auto* state = player->AsActorState()) {
                state->actorState2.wantBlocking = (_leftKeyHeld || _altKeyHeld) ? 1 : 0;
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
                player->IsBlocking(), _altKeyHeld);
        }
    }

    void Controller::beginAltBlock() {
        _altKeyHeld = true;
        _alt = {};
        if (!settings::blockCommitOn()) return;
        if (settings::log()) SKSE::log::info("[blockCommit]: beginAltBlock");
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
        if (auto* player = RE::PlayerCharacter::GetSingleton()) {
            if (auto* state = player->AsActorState()) {
                if (player->IsBlocking()) {
                    player->NotifyAnimationGraph("blockStop");
                    if (settings::log()) SKSE::log::info("[blockCommit]: delayed blockStop fired");
                }
            }
            updateWantBlocking(player);
        }
    }

    void Controller::reset() {
        // A bash or another animation may stop blocking before release is due.
        if (_alt.releasePending) {
            updateWantBlocking(RE::PlayerCharacter::GetSingleton());
        }
        _alt = {};
    }

    void Controller::Update(float a_delta) {
        auto* player = RE::PlayerCharacter::GetSingleton();
        const bool isBlocking = player && player->IsBlocking();
        if (_alt.releasePending && !isBlocking) {
            stopAltBlocking();
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

#include "PCH.h"
#include "blockHandler.h"
#include "settings.h"
#include "utils.h"

using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

namespace block {
    blockHandler* blockHandler::GetSingleton() {
        static blockHandler inst;
        return std::addressof(inst);
    }

    void blockHandler::OnBlockDown(RE::ButtonEvent* event) {
        if (!event) return;
        _device = event->GetDevice();
        _idCode = event->GetIDCode();
        _blockKeyHeld = true;
        _releaseRequested = false;
        _waitingForBlockEnd = false;
        _pending.active = false;
        _pending.remaining = 0.0f;
        if (settings::log()) log::info("[blockHandler]: left/block key pressed");
    }
        
    bool blockHandler::OnBlockUp(float heldDuration) { 
        if (!_blockKeyHeld) return false;
        _blockKeyHeld = false;
        if (!settings::blockCommitOn()) {
            _releaseRequested = false;
            _pending.active = false;
            _pending.remaining = 0.0f;
            return false;
        }
        const float commit = settings::getCommitDur();
        if (heldDuration >= commit) {
            _pending.active = false;
            _pending.remaining = 0.0f;
            if (settings::log()) log::info("[blockHandler] Key held longer than commit, release");
            return false;
        }

        auto* pc = RE::PlayerCharacter::GetSingleton();
        if (!pc) {
            return false;
        }
        const bool isBlockingLike = pc->IsBlocking();
        if (!isBlockingLike) {
            return false;
        }
        //so set the remaining duration i think?
        _pending.active = true;
        _pending.remaining = (commit - heldDuration);
        if (settings::log()) log::info("[blockHandler] left/block was not held long enough, pending remain={}", _pending.remaining);
        return true;
    }

    void blockHandler::Update(float a_delta) {
        if (_waitingForBlockEnd) {
            auto* pc = RE::PlayerCharacter::GetSingleton();
            if (!pc || !pc->IsBlocking()) {
                if (pc) {
                    if (auto* st = pc->AsActorState()) {
                        st->actorState2.wantBlocking = 0;
                    }
                }
                _waitingForBlockEnd = false;
            }
        }
        if (!_pending.active) {
            return;
        }
        if (a_delta <= 0.0f) {
            return;
        }
        _pending.remaining -= a_delta;
        if (_pending.remaining > 0.0f) {
            return;
        }
        _pending.active = false;
        if (_blockKeyHeld) {
            return;
        }
        //release was requested
        _releaseRequested = true;
        if (settings::log()) log::info("releaseRequest=true");
    }

    bool blockHandler::TryInjectRelease(ProcessButton processButton) {
        if (!_releaseRequested || _blockKeyHeld || !processButton) return false;
        auto* controls = RE::PlayerControls::GetSingleton();
        if (!controls || !controls->attackBlockHandler || !RE::PlayerCharacter::GetSingleton()) return false;

        auto* release = RE::ButtonEvent::Create(_device, "Left Attack/Block", _idCode, 0.0f, 0.0f);
        if (!release) return false;

        processButton(controls->attackBlockHandler, release, std::addressof(controls->data));
        OnReleaseForwarded(RE::PlayerCharacter::GetSingleton());
        RE::free(release);
        _releaseRequested = false;
        if (settings::log()) log::info("[blockHandler]: injected delayed left/block release");
        return true;
    }

    void blockHandler::OnReleaseForwarded(RE::PlayerCharacter* player) {
        if (!player || _blockKeyHeld) return;
        auto* st = player->AsActorState();
        if (!st) return;

        _waitingForBlockEnd = player->IsBlocking();
        st->actorState2.wantBlocking = _waitingForBlockEnd ? 1 : 0;
        if (settings::log()) {
            log::info("[blockHandler]: release forwarded, graph blocking={}", _waitingForBlockEnd);
        }
    }
}

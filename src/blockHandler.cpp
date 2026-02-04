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

    void blockHandler::OnBlockDown() {
        auto* pc = RE::PlayerCharacter::GetSingleton();
        if (!pc) {
            return;
        }
        if (!utils::isLeftKeyBlock(pc)) {
            return;
        }
        _blockKeyHeld = true;
        _pending.active = false;
        _pending.remaining = 0.0f;
        log::info("block key held down");
    }
        
    bool blockHandler::OnBlockUp(float heldDuration) { 
        _blockKeyHeld = false;
        const float commit = settings::getCommitDur();
        if (heldDuration >= commit) {
            _pending.active = false;
            _pending.remaining = 0.0f;
            log::info("OnBlockUp: Key held longer than commit, release");
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
        return true;
    }

    void blockHandler::Update(float a_delta) {
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
        //in this case it means the block key is no longer held and the commitment is long enough
        _releaseRequested = true;
        log::info("releaseRequest=true");
    }

    bool blockHandler::consumeReleaseRequest() {
        if (!_releaseRequested) return false;
        _releaseRequested = false;
        return true;
    }
}

#include "PCH.h"

#include "updateHook.h"
#include "ABhandler.h"
#include "blockHandler.h"

using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

namespace updateHook {
    void PlayerUpdateHook::Install() {
        log::info("attempting to install PlayerCharacter::Update hook at 0x0AD");
        REL::Relocation<std::uintptr_t> vtbl{VTABLE[0]};
        _orig = vtbl.write_vfunc(0x0AD, &PlayerUpdateHook::Hook_Update);
        log::info("Installed PlayerCharacter::Update hook at 0x0AD");
    }

    void PlayerUpdateHook::Hook_Update(float a_delta) {
        _orig(this, a_delta);
        block::blockHandler::GetSingleton()->Update(a_delta);
        ABHook::Check();
        log::trace("Hook_Update tick {}", a_delta);
    }
}
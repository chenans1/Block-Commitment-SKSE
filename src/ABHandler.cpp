#include "PCH.h"
#include "ABHandler.h"

using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

using ProcessButton_t = void (*)(RE::AttackBlockHandler*, RE::ButtonEvent*, RE::PlayerControlsData*);
static inline ProcessButton_t _ProcessButton = nullptr;

static void ABHook_handler(RE::AttackBlockHandler* self, RE::ButtonEvent* ev, RE::PlayerControlsData* data) {
    if (!self || !ev || !data || !_ProcessButton) {
        log::warn("[ABHook]: missing self/ev/data/_ProcessButton");
        return;
    }
}

void ABHook::Install() {
    log::info("Attempting to install ABHook...");
    REL::Relocation<std::uintptr_t> vtbl{RE::VTABLE_AttackBlockHandler[0]};
    // return original vfunc address?
    const std::uintptr_t orig = vtbl.write_vfunc(0x4, &ABHook_handler);
    // convert raw address -> function pointer
    _ProcessButton = reinterpret_cast<ProcessButton_t>(orig);
    log::info("Finished Installing ABhook")
}
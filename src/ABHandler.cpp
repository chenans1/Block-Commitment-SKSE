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

    if (ev && ev->IsUp()) {
        const char* s = ev->QUserEvent().c_str();
        if (s && std::strcmp(s, "Left Attack/Block") == 0) {
            if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                if (player->IsBlocking()) {
                    log::info("Player is blocking and block key release detected");
                } else {
                    log::info("Left Attack/Block key released");
                }
            }
        }
    }
    return _ProcessButton(self, ev, data);
}

void ABHook::Install() {
    log::info("Attempting to install ABHook...");
    REL::Relocation<std::uintptr_t> vtbl{RE::VTABLE_AttackBlockHandler[0]};
    const std::uintptr_t orig = vtbl.write_vfunc(0x4, &ABHook_handler);
    _ProcessButton = reinterpret_cast<ProcessButton_t>(orig);
    log::info("Finished Installing ABhook");
};
#include "PCH.h"
#include "settings.h"
#include <SimpleIni.h>
#include <SKSEMenuFramework.h>

using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

static bool ini_bool(CSimpleIniA& ini, const char* section, const char* key, bool def) {
    return ini.GetLongValue(section, key, def ? 1L : 0L) != 0;
}

static float ini_float(CSimpleIniA& ini, const char* section, const char* key, float def) {
    return static_cast<float>(ini.GetDoubleValue(section, key, def));
}

namespace settings {
    //some stuff to handle the capturing of the keybind since i think onInput just straight up always polls even if the menu is closed
    inline std::atomic_bool g_captureBind{false};
    inline bool g_waitingRelease = false;

    //fetch the input and save it
    bool __stdcall OnInput(RE::InputEvent* event) {
        bool blockInput = false;

        // only cba if the user hit the rebind button
        if (!g_captureBind.load(std::memory_order_acquire)) {
            return blockInput;
        }

        if (!event) {
            return blockInput;
        }

        auto* btn = event->AsButtonEvent();
        if (!btn) {
            return blockInput;
        }

        if (!btn->IsDown()) {
            // only cba the down press
            if (g_waitingRelease) {
                g_waitingRelease = false;
            }
            return blockInput;
        }

        const int macro = toKeyCode(*btn);

        if (macro != 0) {
            auto& c = Get();
            c.altBlockKey = macro;

            save();
            g_captureBind.store(false, std::memory_order_release);
            g_waitingRelease = true;
        }

        return blockInput;
    }

    static config cfg{};
    config& Get() { return cfg; }

    void load() {
        constexpr auto path = "Data/SKSE/Plugins/blockOverhaul.ini";
        CSimpleIniA ini;

        ini.SetUnicode(false);
        const SI_Error rc = ini.LoadFile(path);

        if (rc < 0) {
            // File missing or unreadable. Keep defaults in g_cfg.
            log::warn("Could not load ini '{}'. Using defaults.", path);
            return;
        }

        auto& c = Get();
        c.commitDuration = ini_float(ini, "general", "commitDuration", c.commitDuration);
        c.log = ini_bool(ini, "general", "enableLog", c.log);
        c.leftAttack = ini_bool(ini, "general", "isLeftAttack", c.leftAttack);
        c.altBlockKey = static_cast<int>(ini.GetLongValue("general", "altBlockKey", c.altBlockKey));

        log::info("Settings Loaded: commitDuration={}, isLeftAttack={}", c.commitDuration, c.leftAttack);
    }

    void save() { 
        constexpr const char* path = "Data/SKSE/Plugins/blockOverhaul.ini";
        auto& c = Get();
        CSimpleIniA ini;
        ini.SetUnicode(false);

        // on fail create new file
        (void)ini.LoadFile(path);

        ini.SetDoubleValue("general", "commitDuration", static_cast<double>(c.commitDuration), "%.3f");
        ini.SetLongValue("general", "enableLog", c.log ? 1 : 0);
        ini.SetLongValue("general", "isLeftAttack", c.leftAttack ? 1 : 0);
        ini.SetLongValue("general", "altBlockKey", c.altBlockKey);

        const SI_Error rc = ini.SaveFile(path);
        if (rc < 0) {
            log::error("Failed to save ini '{}'. SI_Error={}", path, static_cast<int>(rc));
            return;
        }

        log::info("Saved ini '{}'", path);
    }

    void __stdcall RenderMenuPage() { 
        auto& c = Get();

        ImGuiMCP::DragFloat("Block Commitment Duration (Seconds)", &c.commitDuration, 0.01f, 0.0f, 5.0f, "%.2f");

        ImGuiMCP::Text("AltBlock Key: %d", c.altBlockKey);
        if (!g_captureBind.load(std::memory_order_acquire)) {
            if (ImGuiMCP::Button("Rebind")) {
                g_captureBind.store(true, std::memory_order_release);
            }
        } else {
            ImGuiMCP::SameLine();
            ImGuiMCP::TextUnformatted("Press any key / mouse button / gamepad button...");
            if (ImGuiMCP::Button("Cancel")) {
                g_captureBind.store(false, std::memory_order_release);
            }
        }
        

    }

    void RegisterMenu() {
        if (!SKSEMenuFramework::IsInstalled()) {
            SKSE::log::warn("SKSE Menu Framework not installed; skipping menu registration.");
            return;
        }
        log::info("RegisterMenu Installed()");
        SKSEMenuFramework::SetSection("Block Overhaul");
        SKSEMenuFramework::AddSectionItem("Settings", RenderMenuPage);
        SKSEMenuFramework::AddInputEvent(OnInput);
    }

    int toKeyCode(const RE::ButtonEvent& event) {
        const auto device = event.device.get();
        const auto id = event.GetIDCode();

        switch (device) {
            case RE::INPUT_DEVICE::kKeyboard:
                return static_cast<int>(id);

            case RE::INPUT_DEVICE::kMouse:
                return static_cast<int>(id + SKSE::InputMap::kMacro_MouseButtonOffset);

            case RE::INPUT_DEVICE::kGamepad:
                return static_cast<int>(SKSE::InputMap::GamepadMaskToKeycode(id));

            default:
                return 0;
        }
    }
}
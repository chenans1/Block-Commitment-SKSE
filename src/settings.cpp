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
    //inline bool g_waitingRelease = false;

    enum class CaptureTarget : std::uint8_t { None = 0, AltBlock, Modifier };
    inline std::atomic<CaptureTarget> g_captureTarget{CaptureTarget::None};
    inline std::atomic_bool g_waitingRelease{false};
    //inline std::atomic_bool unsaved{false};

    static config cfg{};
    config& Get() { return cfg; }

    static void startCapture(CaptureTarget target) { 
        g_captureTarget.store(target, std::memory_order_release);
        g_waitingRelease.store(true, std::memory_order_release);
    }

    static void stopCapture() {
        g_captureTarget.store(CaptureTarget::None, std::memory_order_release);
        g_waitingRelease.store(false, std::memory_order_release);
    }

    static void ApplyCapturedKey(CaptureTarget target, int keycode) {
        auto& c = Get();
        switch (target) {
            case CaptureTarget::AltBlock:
                c.altBlockKey = keycode;
                break;
            case CaptureTarget::Modifier:
                c.modifierKey = keycode;
                break;
            default:
                break;
        }
        //save();
        stopCapture();
    }

    //fetch the input and save it
    bool __stdcall OnInput(RE::InputEvent* event) {
        const auto target = g_captureTarget.load(std::memory_order_acquire);
        if (target == CaptureTarget::None) {
            return false;
        }

        bool blockInput = true;

        if (!event) {
            return blockInput;
        }

        auto* btn = event->AsButtonEvent();
        if (!btn) {
            return blockInput;
        }

        if (g_waitingRelease.load(std::memory_order_acquire)) {
            if (!btn->IsDown()) {
                g_waitingRelease.store(false, std::memory_order_release);
            }
            return blockInput;
        }

        if (btn->IsDown()) {
            return blockInput;
        }

        // ESC = unbind and exit
        if (btn->device.get() == RE::INPUT_DEVICE::kKeyboard) {
            if (btn->GetIDCode() == 0x01) {
                ApplyCapturedKey(target, -1);
                return blockInput;
            }
        }

        const int macro = toKeyCode(*btn);

        if (macro != 0) {
            //auto& c = Get();
            ApplyCapturedKey(target, macro);
        }

        return blockInput;
    }

    void load() {
        constexpr auto path = "Data/SKSE/Plugins/blockOverhaul.ini";
        CSimpleIniA ini;

        ini.SetUnicode(false);
        const SI_Error rc = ini.LoadFile(path);

        if (rc < 0) {
            log::warn("Could not load ini '{}'. Using defaults.", path);
            return;
        }

        auto& c = Get();
        c.commitDuration = ini_float(ini, "general", "commitDuration", c.commitDuration);
        c.log = ini_bool(ini, "general", "enableLog", c.log);
        c.leftAttack = ini_bool(ini, "general", "isLeftAttack", c.leftAttack);
        c.altBlockKey = static_cast<int>(ini.GetLongValue("general", "altBlockKey", c.altBlockKey));
        c.modifierKey = static_cast<int>(ini.GetLongValue("general", "modifierKey", c.modifierKey));
        c.isDoubleBindDisabled = ini_bool(ini, "general", "allowBlockDoubleBind", c.isDoubleBindDisabled);
        c.blockCancelCost = ini_float(ini, "general", "blockCancelCost", c.blockCancelCost);
        c.enableBlockCancel = ini_bool(ini, "general", "enableBlockCancel", c.enableBlockCancel);
        c.allowMCORecovery = ini_bool(ini, "general", "allowMCORecovery", c.allowMCORecovery);
        c.mageBlock = ini_bool(ini, "general", "mageBlock", c.mageBlock);
        c.mageBash = ini_bool(ini, "general", "mageBash", c.mageBash);
        //c.isSBF = ini_bool(ini, "general", "isSBF", c.isSBF);
        c.replaceLeftBlockWithBash = ini_bool(ini, "general", "replaceLeftBlockWithBash", c.replaceLeftBlockWithBash);

        log::info("Settings Loaded: commitDuration={}, isLeftAttack={}, allowBlockDoubleBind={}", 
            c.commitDuration, c.leftAttack, c.isDoubleBindDisabled);
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
        ini.SetLongValue("general", "modifierKey", c.modifierKey);
        ini.SetLongValue("general", "isDoubleBindDisabled", c.isDoubleBindDisabled ? 1 : 0);
        ini.SetDoubleValue("general", "blockCancelCost", static_cast<double>(c.blockCancelCost), "%.3f");
        ini.SetLongValue("general", "enableBlockCancel", c.enableBlockCancel ? 1 : 0);
        ini.SetLongValue("general", "allowMCORecovery", c.allowMCORecovery ? 1 : 0);
        ini.SetLongValue("general", "mageBlock", c.mageBlock ? 1 : 0);
        ini.SetLongValue("general", "mageBash", c.mageBash ? 1 : 0);
        //ini.SetLongValue("general", "isSBF", c.isSBF ? 1 : 0);
        ini.SetLongValue("general", "replaceLeftBlockWithBash", c.replaceLeftBlockWithBash ? 1 : 0);


        const SI_Error rc = ini.SaveFile(path);
        if (rc < 0) {
            log::error("Failed to save ini '{}'. SI_Error={}", path, static_cast<int>(rc));
            return;
        }

        log::info("Saved ini '{}'", path);
    }

    void __stdcall RenderMenuPage() { 
        auto& c = Get();
        static bool unsaved = false;
        unsaved |= ImGuiMCP::DragFloat("Block Commitment Duration (Seconds)", &c.commitDuration, 0.01f, 0.0f, 5.0f, "%.2f");
        unsaved |= ImGuiMCP::Checkbox("Is left attack? (MCO/BFCO users = no)", &c.leftAttack);
        unsaved |= ImGuiMCP::Checkbox("Disable alt block if left is already block?", &c.isDoubleBindDisabled);

        const auto capturing = g_captureTarget.load(std::memory_order_acquire);

        //unsaved |= ImGuiMCP::Separator();
        ImGuiMCP::Text("AltBlock Key: %d", c.altBlockKey);

        if (capturing == CaptureTarget::AltBlock) {
            ImGuiMCP::SameLine();
            ImGuiMCP::TextUnformatted("Press a key... (ESC = unbind)");
        } else if (capturing == CaptureTarget::None) {
            if (ImGuiMCP::Button("Rebind AltBlock")) {
                startCapture(CaptureTarget::AltBlock);
            }
            ImGuiMCP::SameLine();
            if (ImGuiMCP::Button("Unbind AltBlock")) {
                c.altBlockKey = -1;
            }
        }

        //ImGuiMCP::Separator();
        ImGuiMCP::Text("Modifier Key: %d", c.modifierKey);

        if (capturing == CaptureTarget::Modifier) {
            ImGuiMCP::SameLine();
            ImGuiMCP::TextUnformatted("Press a key... (ESC = unbind)");
        } else if (capturing == CaptureTarget::None) {
            if (ImGuiMCP::Button("Rebind Modifier")) {
                startCapture(CaptureTarget::Modifier);
            }
            ImGuiMCP::SameLine();
            if (ImGuiMCP::Button("Unbind Modifier")) {
                c.modifierKey = -1;
            }
        }

        if (capturing != CaptureTarget::None) {
            ImGuiMCP::Separator();
            ImGuiMCP::TextUnformatted("Listening for input... (ESC unbinds)");
        }

        // block cancelling stuff
        //ImGuiMCP::Separator();
        unsaved |= ImGuiMCP::Checkbox("Enable Block Cancelling Stamina Cost?", &c.enableBlockCancel);
        unsaved |= ImGuiMCP::Checkbox("No Stamina Cost During MCO_Recovery?", &c.allowMCORecovery);
        unsaved |= ImGuiMCP::DragFloat("Block Cancel Cost", &c.blockCancelCost, 1.0f, 0.0f, 50.0f, "%.2f");

        //unsaved |= ImGuiMCP::Checkbox("Replace left block with Bash?", &c.replaceLeftBlockWithBash);

        unsaved |= ImGuiMCP::Checkbox("Enable Alt Block for Mages? (Requires behavior patch)", &c.mageBlock);
        //unsaved |= ImGuiMCP::Checkbox("Enable Mage Bashing?", &c.mageBash);

        //ImGuiMCP::Separator();
        unsaved |= ImGuiMCP::Checkbox("Enable Log", &c.log);

        if (ImGuiMCP::Button("Save")) {
            save();
            unsaved = false;
        }
        ImGuiMCP::SameLine();
        if (ImGuiMCP::Button("Revert")) {
            load();
            unsaved = false;
        }

        if (unsaved) {
            ImGuiMCP::TextUnformatted("Unsaved changes");
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
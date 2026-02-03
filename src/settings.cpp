#include "PCH.h"
#include "settings.h"
#include <SimpleIni.h>

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

        log::info("Settings Loaded: commitDuration={}", c.commitDuration);
    }
}
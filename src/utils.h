#pragma once

#include <RE/Skyrim.h>
#include <REL/Relocation.h>

namespace utils {

    //hooks, addresses are adapted from valhalla combat
    inline REL::Relocation<float*> g_deltaTime{RELOCATION_ID(523660, 410199)};
    inline REL::Relocation<float*> g_deltaTimeRealTime{RELOCATION_ID(523661, 410200)};

    inline float DeltaTime() {
        auto* p = g_deltaTime.get();
        return p ? *p : 0.0f;
    }

    inline float RealTimeDelta() {
        auto* p = g_deltaTimeRealTime.get();
        return p ? *p : 0.0f;
    }
    bool hasTelescopeKeyword(const RE::TESObjectARMO* shield);
    bool isLeftKeyBlock(RE::PlayerCharacter* player);
    //borderline just the same function with less restrictions, will be expanded to include magic blocking later.
    bool canAltBlock(RE::PlayerCharacter* player);

    //check if player attacking test
    bool isPlayerAttacking(RE::PlayerCharacter* player);
    bool resolveBlockCancel(RE::PlayerCharacter* player);

    void initKeyword();
    //for somnium check for:
    //apo_key_telescope
}
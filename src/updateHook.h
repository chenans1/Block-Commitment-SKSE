#pragma once

namespace updateHook {
    class PlayerUpdateHook : public RE::PlayerCharacter {
    public:
        static void Install();

    private:
        void Hook_Update(float a_delta);
        static inline REL::Relocation<decltype(&RE::PlayerCharacter::Update)> _orig;
    };
}
#pragma once

namespace bash {
    class bashController {
    public:
        static bashController* GetSingleton() { 
            static bashController singleton;
            return std::addressof(singleton);
        }
        void start(float delay, RE::PlayerCharacter* player);
        void update(float a_delta);
        void init();
    private:
        bashController() = default;

        struct State {
            duration = 0.0f;
            bool isBashing = false;
        } _state{};

        void bashRelease(RE::Actor* a);
        static RE::BGSAction* actionBash = nullptr;
        static const SKSE::TaskInterface* g_taskInterface = nullptr;
    };
}
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
        void bashRelease(RE::Actor* a);
        void beginBash(RE::PlayerCharacter* player);

    private:
        bashController() = default;

        struct State {
            float duration = 0.0f;
            bool isBashing = false;
        } _state{};
        static inline RE::BGSAction* actionBash;
        const SKSE::TaskInterface* g_taskInterface = nullptr;
    };
}
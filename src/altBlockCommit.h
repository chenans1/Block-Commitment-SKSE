#pragma once

namespace altCommit {
    class altController {
    public:
        static altController* GetSingleton();
        void delayedBlockStop(float duration);
        void Update(float a_delta);

    private:
        altController() = default;

        struct Pending {
            bool wantStop = false; 
            float remaining = 0.0f;
        } _pending{};
    };
}
#pragma once

namespace altCommit {
    class altController {
    public:
        static altController* GetSingleton();
        void beginAltBlock();
        void wantReleaseBlock();
        void Update(float a_delta);

    private:
        altController() = default;

        struct Pending {
            bool wantStop = false; 
            float blockDuration = 0.0f;
        } _pending{};
    };
}
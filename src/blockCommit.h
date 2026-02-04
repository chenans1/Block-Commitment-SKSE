#pragma once

namespace blockCommit {
    class Controller {
    public:
        static Controller* GetSingleton();
        void beginAltBlock();
        void wantReleaseAltBlock();

        void beginLeftBlock();
        bool wantReleaseLeftBlock();

        void Update(float a_delta);

    private:
        Controller() = default;

        struct State {
            bool altBlockMode = false; //whether we are alt blocking or left block
            bool wantStop = false; //whether we want to stop blocking or not
            float blockDuration = 0.0f; //current block duration
        } _state{};


    };
}
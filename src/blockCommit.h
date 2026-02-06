#pragma once

namespace blockCommit {
    class Controller {
    public:
        static Controller* GetSingleton();
        void stopBlocking();

        void beginAltBlock();
        void wantReleaseAltBlock();

        void beginLeftBlock();
        bool wantReleaseLeftBlock();

        void Update(float a_delta);
        
        void reset();

    private:
        Controller() = default;

        struct State {
            bool altBlockMode = false; //whether we are alt blocking or left block
            bool wantStop = false; //whether we want to stop blocking or not
            float blockDuration = 0.0f; //current block duration
            bool isBlocking = false;
        } _state{};

        //input device stuff, cached for left release injection
        RE::INPUT_DEVICE _device = RE::INPUT_DEVICE::kKeyboard;
        std::uint32_t _idCode = 0;
    };
}
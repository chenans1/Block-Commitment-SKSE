#pragma once

namespace block {
    using ProcessButton = void (*)(RE::AttackBlockHandler*, RE::ButtonEvent*, RE::PlayerControlsData*);

    class blockHandler {
    public:
        static blockHandler* GetSingleton();
        //just sets stuff to true
        void OnBlockDown(RE::ButtonEvent* event);
        //return true if we swallow, false if we dont
        bool OnBlockUp(float heldDuration);

        //updates the 
        void Update(float a_delta);
        bool IsBlockHeld() const { return _blockKeyHeld; }

        bool TryInjectRelease(ProcessButton processButton);

    private:
        blockHandler() = default;
        bool _blockKeyHeld = false;
        bool _releaseRequested = false;

        //unlock statemachine
        struct Pending {
            bool active = false;
            float remaining = 0.0f;
        } _pending{};

        RE::INPUT_DEVICE _device = RE::INPUT_DEVICE::kKeyboard;
        std::uint32_t _idCode = 0;
    };
}

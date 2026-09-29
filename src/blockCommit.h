#pragma once

namespace blockCommit {
    using ProcessButton = void (*)(RE::AttackBlockHandler*, RE::ButtonEvent*, RE::PlayerControlsData*);

    class Controller {
    public:
        static Controller* GetSingleton();

        void OnLeftBlockDown(RE::ButtonEvent* event);
        bool OnLeftBlockUp(float heldDuration);
        bool IsLeftBlockHeld() const { return _leftKeyHeld; }
        bool IsAltBlockHeld() const { return _altKeyHeld; }
        bool TryInjectLeftRelease(ProcessButton processButton);
        void OnLeftReleaseForwarded(RE::PlayerCharacter* player);

        void beginAltBlock();
        void onBlockStart();
        void wantReleaseAltBlock();
        void reset();
        void Update(float a_delta);

    private:
        Controller() = default;

        struct Commitment {
            float elapsed = 0.0f;
            bool releasePending = false;
        };

        bool releaseReady(const Commitment& commitment) const;
        void updateWantBlocking(RE::PlayerCharacter* player);
        void stopAltBlocking();

        // Each input keeps its own deadline so one release cannot discard the other.
        Commitment _left{};
        Commitment _alt{};
        bool _leftKeyHeld = false;
        bool _leftReleaseRequested = false;
        bool _altKeyHeld = false;
        RE::INPUT_DEVICE _leftDevice = RE::INPUT_DEVICE::kKeyboard;
        std::uint32_t _leftIdCode = 0;
    };
}

#pragma once

namespace settings {
	struct config {
        bool log = false;
        bool enableBlockCommitment = true;
        float commitDuration = 0.5f;
        bool leftAttack = false;
        int altBlockKey = -1;
        int modifierKey = -1;
        bool isDoubleBindDisabled = false;
        bool enableBlockCancel = true;
        float blockCancelCost = 10.0f;
        bool allowMCORecovery = true;
        bool mageBlock = true;
        bool mageBash = true;
        bool altBlockBash = true;
        float powerBashDelay = 0.2f;
        bool mageWard = true;
        bool forceMCORecovery = true;
        float powerAttackBlockCancelCost = 10.0f;

	};

	config& Get();

	inline float getCommitDur() { return Get().commitDuration; }
    inline float log() { return Get().log; }
    inline bool leftAttack() { return Get().leftAttack; }
    inline int getAltBlock() { return Get().altBlockKey; }
    inline int getModifierKey() { return Get().modifierKey; }
    inline int isDoubleBindDisabled() { return Get().isDoubleBindDisabled; }
    inline bool isBlockCancelEnabled() { return Get().enableBlockCancel; }
    inline float blockCancelCost() { return Get().blockCancelCost; }
    inline bool allowMCORecovery() { return Get().allowMCORecovery; }
    inline bool mageBlock() { return Get().mageBlock; }
    inline bool mageBash() { return Get().mageBash; }
    inline bool mageWard() { return Get().mageWard; }
    inline int altBlockBash() { return Get().altBlockBash; }
    inline float powerBashDelay() { return Get().powerBashDelay; }
    inline bool MCORecoveryCancel() { return Get().forceMCORecovery; }
    inline float PACancelCost() { return Get().powerAttackBlockCancelCost; }
    inline bool blockCommitOn() { return Get().enableBlockCommitment; }

    void RegisterMenu();
    void __stdcall RenderMenuPage();

	void load();
    void save();
    int toKeyCode(const RE::ButtonEvent& event);
}
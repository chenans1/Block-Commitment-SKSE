#pragma once

namespace settings {
	struct config {
        bool log = false;
        float commitDuration = 0.3f;
        bool leftAttack = false;
        int altBlockKey = -1;
        bool isDoubleBindDisabled = false;
	};

	config& Get();

	inline float getCommitDur() { return Get().commitDuration; }
    inline float log() { return Get().log; }
    inline bool leftAttack() { return Get().leftAttack; }
    inline int getAltBlock() { return Get().altBlockKey; }
    inline int isDoubleBindDisabled() { return Get().isDoubleBindDisabled; }

    void RegisterMenu();
    void __stdcall RenderMenuPage();

	void load();
    void save();
    int toKeyCode(const RE::ButtonEvent& event);
}
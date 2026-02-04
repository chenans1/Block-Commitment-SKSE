#pragma once

namespace settings {
	struct config {
        bool log = false;
        float commitDuration = 0.3f;
        bool leftAttack = false;
        int altBlockKey = -1;
	};

	config& Get();

	inline float getCommitDur() { return Get().commitDuration; }
    inline float log() { return Get().log; }
    inline bool leftAttack() { return Get().leftAttack; }
    inline int getAltBlock() { return Get().altBlockKey; }
	void load();
    //skse menu stuff
    void RegisterMenu();
    void __stdcall RenderMenuPage();
    void save();

    int toKeyCode(const RE::ButtonEvent& event);
}
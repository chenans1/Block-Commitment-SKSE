#pragma once

namespace settings {
	struct config {
        bool log = false;
        float commitDuration = 0.3f;
        bool leftAttack = false;
	};

	config& Get();

	inline float getCommitDur() { return Get().commitDuration; }
    inline float log() { return Get().log; }
    inline bool leftAttack() { return Get().leftAttack; }
	void load();
}
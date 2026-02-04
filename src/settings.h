#pragma once

namespace settings {
	struct config {
        bool log = false;
        float commitDuration = 0.3f;
	};

	config& Get();

	inline float getCommitDur() { return Get().commitDuration; }
    inline float log() { return Get().log; }
	void load();
}
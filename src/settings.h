#pragma once

namespace settings {
	struct config {
        float commitDuration = 0.3f;
	};

	config& Get();

	inline float getCommitmentDuration() { return Get().commitDuration; }

	void load();
}
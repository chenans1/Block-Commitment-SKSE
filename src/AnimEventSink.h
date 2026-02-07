#pragma once

#include "blockCommit.h"
#include "utils.h"
#include "settings.h"

namespace block {
    class AnimEventSink : public RE::BSTEventSink<RE::BSAnimationGraphEvent> {
    public:
        static AnimEventSink& GetSingleton() {
            static AnimEventSink inst;
            return inst;
        }

        RE::BSEventNotifyControl ProcessEvent(
            const RE::BSAnimationGraphEvent* a_event,
            RE::BSTEventSource<RE::BSAnimationGraphEvent>*) {
            if (!a_event || !a_event->holder) {
                return RE::BSEventNotifyControl::kContinue;
            }

            if (settings::mageBlock() && a_event->tag == "blockStartOut"sv) {
                if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                    if (utils::isRightHandCaster(player)) {
                        utils::castWardSpell(player);
                    }
                    return RE::BSEventNotifyControl::kContinue;
                }
            }
            return RE::BSEventNotifyControl::kContinue;
        }

    private:
        // constructors and destructors
        AnimEventSink() = default;
        ~AnimEventSink() = default;
        AnimEventSink(const AnimEventSink&) = delete;
        AnimEventSink(AnimEventSink&&) = delete;
        AnimEventSink& operator=(const AnimEventSink&) = delete;
        AnimEventSink& operator=(AnimEventSink&&) = delete;
    };
}
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

            /*if (a_event->tag == "SBF_BlockStart"sv) {
                blockCommit::Controller::GetSingleton()->beginAltBlock();
                if (settings::isBlockCancelEnabled()) {
                    if (auto* player = RE::PlayerCharacter::GetSingleton()) {
                        resolveBlockCancel(player);
                    }
                }
                return RE::BSEventNotifyControl::kContinue;
            }*/
            /*const bool usingSBF = settings::isSBF();
            const bool isStartTag =
                (usingSBF && a_event->tag == "SBF_BlockStart"sv) || (!usingSBF && a_event->tag == "blockStartOut"sv);

            if (isStartTag) {*/
            if (a_event->tag == "blockStartOut"sv) {
                SKSE::log::info("start tag = {}", a_event->tag.data());
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
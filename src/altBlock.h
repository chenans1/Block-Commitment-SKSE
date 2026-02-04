#pragma once

namespace altBlock {
    RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_events,
                                        RE::BSTEventSource<RE::InputEvent*>* a_eventSource);
}
#pragma once

namespace altBlock {
    class AltBlockInputSink : public RE::BSTEventSink<RE::InputEvent*> {
    public:
        static AltBlockInputSink& GetSingleton() {
            static AltBlockInputSink singleton;
            return singleton;
        }

        RE::BSEventNotifyControl ProcessEvent(
            RE::InputEvent* const* a_events, 
            RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override;

    private:
        AltBlockInputSink() = default;
        ~AltBlockInputSink() = default;
        AltBlockInputSink(const AltBlockInputSink&) = delete;
        AltBlockInputSink(AltBlockInputSink&&) = delete;
        AltBlockInputSink& operator=(const AltBlockInputSink&) = delete;
        AltBlockInputSink& operator=(AltBlockInputSink&&) = delete;
    };
    
}
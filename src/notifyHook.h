#pragma once

//shamelessly ripped from OAR
namespace notify {
    void Install();
    bool PC_NotifyAnimationGraph(RE::IAnimationGraphManagerHolder* a_this, const RE::BSFixedString& a_eventName);
    inline static REL::Relocation<decltype(PC_NotifyAnimationGraph)> _PC_NotifyAnimationGraph;
}
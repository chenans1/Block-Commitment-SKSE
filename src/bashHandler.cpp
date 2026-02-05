#include "PCH.h"
#include "bashHandler.h"

namespace bash {
    // similar to OCPA - use the action left release
    void bashController::bashRelease(RE::Actor* a) {
        if (!actionBash) {
            SKSE::log::error("Bash action not initialized!");
            return;
        }
        if (!g_taskInterface) {
            SKSE::log::error("Task interface not available!");
            return;
        }
        g_taskInterface->AddTask([a]() {
            std::unique_ptr<RE::TESActionData> data(RE::TESActionData::Create());
            data->source = RE::NiPointer<RE::TESObjectREFR>(a);
            data->action = actionBash;

            typedef bool func_t(RE::TESActionData*);
            REL::Relocation<func_t> func{RELOCATION_ID(40551, 41557)};
            bool success = func(data.get());

            if (settings::log()) {
                SKSE::log::info("Bash trigger: {}", success ? "success" : "failed");
            }
        });
    }

    void bashController::init() {
        //action bash is just the actionLeftRelease
        actionBash = (RE::BGSAction*)RE::TESForm::LookupByID(0x13454);
        g_taskInterface = SKSE::GetTaskInterface();
        log::info("bashstuff: init....");
        if (!actionBash) {
            SKSE::log::error("Failed to find bash action!");
        }
        if (!g_taskInterface) {
            SKSE::log::error("Failed to get task interface!");
        }
    }

	//literally just sets the timer delay and starts bash anim
    void bashController::start(float delay, RE::PlayerCharacter* player) { 
        if (player) {
            _state.duration = delay;
            _state.isBashing = true;
            auto* st = player->AsActorState();
            if (st) {
                // Set the attack state to bash
                st->actorState1.meleeAttackState = RE::ATTACK_STATE_ENUM::kBash;
                player->NotifyAnimationGraph("bashStart");
                log::info("player forced bash attack");
            }
        }
	}

    void bashController::update(float a_delta) {
        if (!_state.isBashing) {
            return;
        }
        if (a_delta <= 0.0f) {
            return;
        }
        _state.duration -= a_delta;
        //means we need to bash release - delay elapsed
        if (_state.duration <= 0) {
            auto* pc = RE::PlayerCharacter::GetSingleton();
            if (pc) {
                bashRelease(pc);
                _state.duration = 0.0f;
                _state.isBashing = false;
            }
            return;
        }
        
    }

}
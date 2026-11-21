#include "effects.h"

#include "narrative_state.h"

#include <exception>

namespace han::narrative {
namespace {

bool fail(std::string* error, std::string message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
    return false;
}

bool apply_one(const Json& effect, NarrativeState& state, const EvalContext& context,
               std::string* error) {
    if (!effect.is_object()) {
        return fail(error, "effect must be an object");
    }

    if (effect.contains("condition")) {
        std::string condition_error;
        if (!ConditionEvaluator{}.evaluate(effect.at("condition"), context, &condition_error)) {
            if (!condition_error.empty()) {
                return fail(error, std::move(condition_error));
            }
            return true;
        }
    }

    try {
        if (effect.contains("set_flag")) {
            state.set_flag(effect.at("set_flag").get<std::string>(),
                           effect.value("value", true));
            return true;
        }
        if (effect.contains("add_var")) {
            state.add_int_var(effect.at("add_var").get<std::string>(),
                              effect.at("value").get<int>());
            return true;
        }
        if (effect.contains("set_var")) {
            const std::string id = effect.at("set_var").get<std::string>();
            const Json& value = effect.at("value");
            if (value.is_string()) {
                state.set_string_var(id, value.get<std::string>());
            } else if (value.is_number_integer()) {
                state.set_int_var(id, value.get<int>());
            } else {
                return fail(error, "set_var " + id + ": value must be int or string");
            }
            return true;
        }
        if (effect.contains("give_item")) {
            state.give_item(effect.at("give_item").get<std::string>(),
                            effect.value("count", 1));
            return true;
        }
        if (effect.contains("take_item")) {
            state.take_item(effect.at("take_item").get<std::string>(),
                            effect.value("count", 1));
            return true;
        }
        if (effect.contains("add_clue")) {
            state.add_clue(effect.at("add_clue").get<std::string>());
            return true;
        }
        if (effect.contains("add_ability")) {
            state.add_ability(effect.at("add_ability").get<std::string>());
            return true;
        }
        if (effect.contains("start_quest")) {
            const std::string id = effect.at("start_quest").get<std::string>();
            if (state.quest_state(id) == QuestState::Inactive) {
                state.set_quest_state(id, QuestState::Active);
            }
            return true;
        }
        if (effect.contains("complete_objective")) {
            state.complete_objective(effect.at("complete_objective").get<std::string>());
            return true;
        }
        if (effect.contains("set_quest_state")) {
            const std::string id = effect.at("set_quest_state").get<std::string>();
            const std::string value = effect.at("value").get<std::string>();
            if (value == "inactive") {
                state.set_quest_state(id, QuestState::Inactive);
            } else if (value == "active") {
                state.set_quest_state(id, QuestState::Active);
            } else if (value == "completed") {
                state.set_quest_state(id, QuestState::Completed);
            } else {
                return fail(error, "set_quest_state " + id + ": bad state " + value);
            }
            return true;
        }
    } catch (const std::exception& e) {
        return fail(error, std::string("effect error: ") + e.what());
    }

    return fail(error, "unknown effect command");
}

} // namespace

bool EffectExecutor::apply(const Json& effects, NarrativeState& state,
                           const std::string& last_choice, std::string* error) const {
    if (error != nullptr) {
        error->clear();
    }
    if (effects.is_null()) {
        return true;
    }

    const EvalContext context{&state, last_choice};
    if (effects.is_array()) {
        for (const Json& effect : effects) {
            if (!apply_one(effect, state, context, error)) {
                return false;
            }
        }
        return true;
    }
    return apply_one(effects, state, context, error);
}

} // namespace han::narrative

#pragma once

#include "condition.h"
#include "narrative_types.h"

#include <string>

namespace han::narrative {

/// Applies a list of effect commands to the narrative state.
///
/// Supported commands:
///   { "set_flag": "id" [, "value": bool] }
///   { "add_var": "id", "value": int }
///   { "set_var": "id", "value": int|string }
///   { "give_item": "id" [, "count": int] }
///   { "take_item": "id" [, "count": int] }
///   { "add_clue": "id" }
///   { "add_ability": "id" }
///   { "start_quest": "id" }
///   { "complete_objective": "id" }
///   { "set_quest_state": "id", "value": "inactive|active|completed" }
/// Every command may carry an optional "condition".
class EffectExecutor {
public:
    bool apply(const Json& effects, NarrativeState& state,
               const std::string& last_choice = {}, std::string* error = nullptr) const;

private:
    ConditionEvaluator conditions_;
};

} // namespace han::narrative

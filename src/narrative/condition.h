#pragma once

#include "narrative_types.h"

#include <string>

namespace han::narrative {

/// Evaluates the finite condition language used by chapter JSON.
///
/// Supported forms:
///   { "all": [ ... ] }, { "any": [ ... ] }, { "not": ... }
///   { "flag": "id" [, "value": bool] }
///   { "var": "id", "op": "eq|neq|gt|gte|lt|lte", "value": ... }
///   { "item": "id" }, { "clue": "id" }
///   { "quest": "id", "state": "inactive|active|completed" }
///   { "objective": "id" }, { "choice": "id" }
class ConditionEvaluator {
public:
    bool evaluate(const Json& expression, const EvalContext& context,
                  std::string* error = nullptr) const;
};

} // namespace han::narrative

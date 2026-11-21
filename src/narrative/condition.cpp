#include "condition.h"

#include "narrative_state.h"

#include <exception>

namespace han::narrative {
namespace {

bool set_error(std::string* error, std::string message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
    return false;
}

bool compare_ints(int lhs, const std::string& op, int rhs) {
    if (op == "eq") {
        return lhs == rhs;
    }
    if (op == "neq") {
        return lhs != rhs;
    }
    if (op == "gt") {
        return lhs > rhs;
    }
    if (op == "gte") {
        return lhs >= rhs;
    }
    if (op == "lt") {
        return lhs < rhs;
    }
    if (op == "lte") {
        return lhs <= rhs;
    }
    return false;
}

} // namespace

bool ConditionEvaluator::evaluate(const Json& expression, const EvalContext& context,
                                  std::string* error) const {
    if (context.state == nullptr) {
        return set_error(error, "condition evaluated without state");
    }
    if (error != nullptr) {
        error->clear();
    }
    if (expression.is_boolean()) {
        return expression.get<bool>();
    }
    if (expression.is_array()) {
        for (const Json& child : expression) {
            if (!evaluate(child, context, error)) {
                return false;
            }
        }
        return true;
    }
    if (!expression.is_object()) {
        return set_error(error, "condition must be an object or bool");
    }

    try {
        if (expression.contains("all")) {
            const Json& list = expression.at("all");
            if (!list.is_array()) {
                return set_error(error, "'all' must be an array");
            }
            for (const Json& child : list) {
                if (!evaluate(child, context, error)) {
                    return false;
                }
            }
            return true;
        }

        if (expression.contains("any")) {
            const Json& list = expression.at("any");
            if (!list.is_array()) {
                return set_error(error, "'any' must be an array");
            }
            std::string child_error;
            for (const Json& child : list) {
                if (evaluate(child, context, &child_error)) {
                    return true;
                }
            }
            return false;
        }

        if (expression.contains("not")) {
            return !evaluate(expression.at("not"), context, error);
        }

        if (expression.contains("flag")) {
            const std::string id = expression.at("flag").get<std::string>();
            const bool expected = expression.value("value", true);
            return context.state->flag(id) == expected;
        }

        if (expression.contains("var")) {
            const std::string id = expression.at("var").get<std::string>();
            if (!expression.contains("value")) {
                return set_error(error, "var condition missing 'value'");
            }
            const std::string op = expression.value("op", std::string("eq"));
            const Json& rhs = expression.at("value");

            const bool string_compare =
                rhs.is_string() || !context.state->string_var(id).empty();
            if (string_compare) {
                const std::string lhs = context.state->string_var(id);
                const std::string rhs_text = rhs.is_string()
                                                 ? rhs.get<std::string>()
                                                 : rhs.dump();
                if (op == "eq") {
                    return lhs == rhs_text;
                }
                if (op == "neq") {
                    return lhs != rhs_text;
                }
                return set_error(error, "string var only supports eq/neq");
            }

            if (!rhs.is_number_integer()) {
                return set_error(error, "int var requires an integer value");
            }
            const int lhs = context.state->int_var(id);
            const int rhs_int = rhs.get<int>();
            if (op != "eq" && op != "neq" && op != "gt" && op != "gte" && op != "lt" &&
                op != "lte") {
                return set_error(error, "unknown var operator " + op);
            }
            return compare_ints(lhs, op, rhs_int);
        }

        if (expression.contains("item")) {
            return context.state->has_item(expression.at("item").get<std::string>());
        }

        if (expression.contains("clue")) {
            return context.state->has_clue(expression.at("clue").get<std::string>());
        }

        if (expression.contains("quest")) {
            const std::string id = expression.at("quest").get<std::string>();
            const std::string wanted = expression.value("state", std::string("completed"));
            const std::string actual = to_string(context.state->quest_state(id));
            return wanted == actual;
        }

        if (expression.contains("objective")) {
            return context.state->objective_done(
                expression.at("objective").get<std::string>());
        }

        if (expression.contains("choice")) {
            return context.last_choice == expression.at("choice").get<std::string>();
        }

        return set_error(error, "unknown condition keys");
    } catch (const std::exception& e) {
        return set_error(error, std::string("condition error: ") + e.what());
    }
}

} // namespace han::narrative

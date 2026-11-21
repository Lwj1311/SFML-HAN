#include "validator.h"

#include <map>
#include <set>
#include <string>

namespace han::narrative {
namespace {

class Collector {
public:
    void error(std::string where, std::string message) {
        issues_.push_back(ValidationIssue{ValidationIssue::Severity::Error,
                                          std::move(where), std::move(message)});
    }

    void warning(std::string where, std::string message) {
        issues_.push_back(ValidationIssue{ValidationIssue::Severity::Warning,
                                          std::move(where), std::move(message)});
    }

    std::vector<ValidationIssue> take() { return std::move(issues_); }

private:
    std::vector<ValidationIssue> issues_;
};

} // namespace

std::vector<ValidationIssue> validate_chapter(const Json& state_doc,
                                              const QuestSystem& quests,
                                              const DialogueDocument& dialogue,
                                              const NarrativeState& state) {
    Collector out;

    std::map<std::string, const QuestDef*> quests_by_id;
    std::set<std::string> objective_ids;
    std::set<std::string> choice_ids;
    std::set<std::string> scene_ids;

    for (const QuestDef& quest : quests.definitions()) {
        if (quests_by_id.find(quest.id) != quests_by_id.end()) {
            out.error("quests", "duplicate quest id " + quest.id);
        }
        quests_by_id[quest.id] = &quest;
        for (const ObjectiveDef& objective : quest.objectives) {
            if (!objective_ids.insert(objective.id).second) {
                out.error("quest " + quest.id,
                          "duplicate objective id " + objective.id);
            }
        }
    }

    for (const SceneDef& scene : dialogue.scenes()) {
        if (!scene_ids.insert(scene.id).second) {
            out.error("dialogue", "duplicate scene id " + scene.id);
        }
        for (const std::string& node_id : scene.node_ids) {
            const Json* node = dialogue.find_node(node_id);
            if (node == nullptr) {
                out.error("scene " + scene.id, "missing node " + node_id);
                continue;
            }
            if (node->value("type", std::string{}) == "choice" &&
                node->contains("choices")) {
                std::set<std::string> local;
                for (const Json& choice : node->at("choices")) {
                    const std::string id = choice.value("id", std::string{});
                    choice_ids.insert(id);
                    if (!local.insert(id).second) {
                        out.error("node " + node_id, "duplicate choice id " + id);
                    }
                }
            }
        }
    }

    auto check_condition = [&](const Json& expression, const std::string& where,
                               auto&& self) -> void {
        if (expression.is_boolean()) {
            return;
        }
        if (expression.is_array()) {
            for (const Json& child : expression) {
                self(child, where, self);
            }
            return;
        }
        if (!expression.is_object()) {
            out.error(where, "condition must be an object, array or bool");
            return;
        }

        if (expression.contains("all") || expression.contains("any")) {
            const char* key = expression.contains("all") ? "all" : "any";
            const Json& list = expression.at(key);
            if (!list.is_array()) {
                out.error(where, std::string("'") + key + "' must be an array");
                return;
            }
            for (const Json& child : list) {
                self(child, where, self);
            }
            return;
        }
        if (expression.contains("not")) {
            self(expression.at("not"), where, self);
            return;
        }
        if (expression.contains("flag")) {
            const std::string id = expression.at("flag").get<std::string>();
            if (!state.declares_flag(id)) {
                out.error(where, "unknown flag " + id);
            }
            return;
        }
        if (expression.contains("var")) {
            const std::string id = expression.at("var").get<std::string>();
            const VariableDef* def = state.variable_def(id);
            if (def == nullptr) {
                out.error(where, "unknown variable " + id);
                return;
            }
            if (!expression.contains("value")) {
                out.error(where, "variable condition on " + id + " has no value");
                return;
            }
            const std::string op = expression.value("op", std::string("eq"));
            if (op != "eq" && op != "neq" && op != "gt" && op != "gte" && op != "lt" &&
                op != "lte") {
                out.error(where, "unknown variable operator " + op);
            }
            const Json& value = expression.at("value");
            if (def->type == VariableDef::Type::Enum) {
                if (!value.is_string()) {
                    out.error(where, "enum variable " + id + " needs a string value");
                } else if (op != "eq" && op != "neq") {
                    out.error(where, "enum variable " + id + " only supports eq/neq");
                }
            } else if (!value.is_number_integer()) {
                out.error(where, "int variable " + id + " needs an integer value");
            }
            return;
        }
        if (expression.contains("item")) {
            const std::string id = expression.at("item").get<std::string>();
            if (!state.declares_item(id)) {
                out.error(where, "unknown item " + id);
            }
            return;
        }
        if (expression.contains("clue")) {
            const std::string id = expression.at("clue").get<std::string>();
            if (!state.declares_clue(id)) {
                out.error(where, "unknown clue " + id);
            }
            return;
        }
        if (expression.contains("quest")) {
            const std::string id = expression.at("quest").get<std::string>();
            if (quests_by_id.find(id) == quests_by_id.end()) {
                out.error(where, "unknown quest " + id);
            }
            const std::string wanted = expression.value("state", std::string("completed"));
            if (wanted != "inactive" && wanted != "active" && wanted != "completed") {
                out.error(where, "unknown quest state " + wanted);
            }
            return;
        }
        if (expression.contains("objective")) {
            const std::string id = expression.at("objective").get<std::string>();
            if (objective_ids.find(id) == objective_ids.end()) {
                out.error(where, "unknown objective " + id);
            }
            return;
        }
        if (expression.contains("choice")) {
            if (choice_ids.find(expression.at("choice").get<std::string>()) ==
                choice_ids.end()) {
                out.error(where, "unknown choice id " +
                                     expression.at("choice").get<std::string>());
            }
            return;
        }
        out.error(where, "unknown condition keys");
    };

    auto check_condition_at = [&](const Json& expression, const std::string& where) {
        check_condition(expression, where, check_condition);
    };

    auto check_effects = [&](const Json& effects, const std::string& where, auto&& self) -> void {
        if (effects.is_null()) {
            return;
        }
        if (effects.is_array()) {
            for (const Json& effect : effects) {
                self(effect, where, self);
            }
            return;
        }
        if (!effects.is_object()) {
            out.error(where, "effect must be an object or array");
            return;
        }
        if (effects.contains("condition")) {
            check_condition_at(effects.at("condition"), where + " condition");
        }
        if (effects.contains("set_flag")) {
            const std::string id = effects.at("set_flag").get<std::string>();
            if (!state.declares_flag(id)) {
                out.error(where, "set unknown flag " + id);
            }
        } else if (effects.contains("add_var")) {
            const std::string id = effects.at("add_var").get<std::string>();
            if (!state.declares_variable(id)) {
                out.error(where, "add unknown variable " + id);
            }
            if (!effects.contains("value") || !effects.at("value").is_number_integer()) {
                out.error(where, "add_var " + id + " needs an integer value");
            }
        } else if (effects.contains("set_var")) {
            const std::string id = effects.at("set_var").get<std::string>();
            if (!state.declares_variable(id)) {
                out.error(where, "set unknown variable " + id);
            }
            if (!effects.contains("value")) {
                out.error(where, "set_var " + id + " has no value");
            }
        } else if (effects.contains("give_item")) {
            const std::string id = effects.at("give_item").get<std::string>();
            if (!state.declares_item(id)) {
                out.error(where, "give unknown item " + id);
            }
        } else if (effects.contains("take_item")) {
            const std::string id = effects.at("take_item").get<std::string>();
            if (!state.declares_item(id)) {
                out.error(where, "take unknown item " + id);
            }
        } else if (effects.contains("add_clue")) {
            const std::string id = effects.at("add_clue").get<std::string>();
            if (!state.declares_clue(id)) {
                out.error(where, "add unknown clue " + id);
            }
        } else if (effects.contains("add_ability")) {
            // Abilities are declared by the ability system, not by chapter state.
        } else if (effects.contains("start_quest")) {
            const std::string id = effects.at("start_quest").get<std::string>();
            if (quests_by_id.find(id) == quests_by_id.end()) {
                out.error(where, "start unknown quest " + id);
            }
        } else if (effects.contains("complete_objective")) {
            const std::string id = effects.at("complete_objective").get<std::string>();
            if (objective_ids.find(id) == objective_ids.end()) {
                out.error(where, "complete unknown objective " + id);
            }
        } else if (effects.contains("set_quest_state")) {
            const std::string id = effects.at("set_quest_state").get<std::string>();
            if (quests_by_id.find(id) == quests_by_id.end()) {
                out.error(where, "set state of unknown quest " + id);
            }
        } else {
            out.error(where, "unknown effect command");
        }
    };

    auto check_effects_at = [&](const Json& effects, const std::string& where) {
        check_effects(effects, where, check_effects);
    };

    auto check_next = [&](const Json& object, const std::string& where) {
        if (!object.contains("next") || object.at("next").is_null()) {
            return;
        }
        const std::string next = object.at("next").get<std::string>();
        if (dialogue.find_node(next) == nullptr) {
            out.error(where, "next points to missing node " + next);
        }
    };

    if (!state_doc.contains("schema_version")) {
        out.warning("state", "missing schema_version");
    }

    for (const QuestDef& quest : quests.definitions()) {
        const std::string where = "quest " + quest.id;
        if (!quest.prerequisites.is_null()) {
            check_condition_at(quest.prerequisites, where + " prerequisites");
        }
        for (const ObjectiveDef& objective : quest.objectives) {
            check_condition_at(objective.condition, where + " objective " + objective.id);
        }
        if (quest.completion.is_object() && quest.completion.contains("all")) {
            for (const Json& requirement : quest.completion.at("all")) {
                if (!requirement.contains("objective")) {
                    out.error(where, "completion entry without objective");
                    continue;
                }
                const std::string id = requirement.at("objective").get<std::string>();
                if (objective_ids.find(id) == objective_ids.end()) {
                    out.error(where, "completion references unknown objective " + id);
                }
            }
        } else {
            out.error(where, "missing completion.all");
        }

        for (const RewardDef& reward : quest.rewards) {
            if (!reward.condition.is_null()) {
                check_condition_at(reward.condition, where + " reward " + reward.type);
            }
            if (reward.type == "clue" && !state.declares_clue(reward.id)) {
                out.error(where, "reward unknown clue " + reward.id);
            } else if (reward.type == "item" && !state.declares_item(reward.id)) {
                out.error(where, "reward unknown item " + reward.id);
            } else if (reward.type == "variable" &&
                       !state.declares_variable(reward.id)) {
                out.error(where, "reward unknown variable " + reward.id);
            } else if (reward.type == "flag" && !state.declares_flag(reward.id)) {
                out.error(where, "reward unknown flag " + reward.id);
            }
        }

        for (const QuestBranchDef& branch : quest.branches) {
            if (!branch.condition.is_null()) {
                check_condition_at(branch.condition, where + " branch " + branch.id);
            }
            check_effects_at(branch.effects, where + " branch " + branch.id);
        }
    }

    for (const SceneDef& scene : dialogue.scenes()) {
        const std::string where = "scene " + scene.id;
        if (scene.entry.empty() || dialogue.find_node(scene.entry) == nullptr) {
            out.error(where, "missing entry node " + scene.entry);
        }
        if (scene.has_trigger) {
            check_condition_at(scene.trigger, where + " trigger");
        }

        for (const std::string& node_id : scene.node_ids) {
            const Json* node = dialogue.find_node(node_id);
            if (node == nullptr) {
                continue;
            }
            const std::string node_where = "node " + node_id;
            const std::string type = node->value("type", std::string{});
            if (node->contains("effects")) {
                check_effects_at(node->at("effects"), node_where);
            }

            if (type == "narration" || type == "line") {
                if (!node->contains("text") && !node->contains("variants")) {
                    out.error(node_where, "line has neither text nor variants");
                }
                if (node->contains("speaker") &&
                    dialogue.find_speaker(node->at("speaker").get<std::string>()) == nullptr) {
                    out.error(node_where, "unknown speaker " +
                                              node->at("speaker").get<std::string>());
                }
                check_next(*node, node_where);
                if (node->contains("variants")) {
                    bool has_fallback = false;
                    for (const Json& variant : node->at("variants")) {
                        if (!variant.contains("condition")) {
                            has_fallback = true;
                        } else {
                            check_condition_at(variant.at("condition"),
                                               node_where + " variant");
                        }
                        if (variant.contains("speaker") &&
                            dialogue.find_speaker(
                                variant.at("speaker").get<std::string>()) == nullptr) {
                            out.error(node_where,
                                      "unknown variant speaker " +
                                          variant.at("speaker").get<std::string>());
                        }
                        if (variant.contains("effects")) {
                            check_effects_at(variant.at("effects"),
                                             node_where + " variant");
                        }
                        check_next(variant, node_where + " variant");
                    }
                    if (!has_fallback) {
                        out.warning(node_where,
                                    "variants have no unconditional fallback; "
                                    "conditions must be exhaustive");
                    }
                }
            } else if (type == "choice") {
                if (!node->contains("choices") || !node->at("choices").is_array() ||
                    node->at("choices").empty()) {
                    out.error(node_where, "choice node has no choices");
                } else {
                    for (const Json& choice : node->at("choices")) {
                        const std::string choice_where =
                            node_where + " choice " + choice.value("id", std::string{});
                        if (!choice.contains("id")) {
                            out.error(choice_where, "choice without id");
                        }
                        if (!choice.contains("text")) {
                            out.error(choice_where, "choice without text");
                        }
                        check_next(choice, choice_where);
                        if (choice.contains("effects")) {
                            check_effects_at(choice.at("effects"), choice_where);
                        }
                    }
                }
            } else if (type == "branch") {
                if (!node->contains("branches") || node->at("branches").empty()) {
                    out.error(node_where, "branch node has no branches");
                } else {
                    bool has_fallback = false;
                    for (const Json& branch : node->at("branches")) {
                        if (!branch.contains("condition")) {
                            has_fallback = true;
                        } else {
                            check_condition_at(branch.at("condition"),
                                               node_where + " branch");
                        }
                        check_next(branch, node_where + " branch");
                    }
                    if (!has_fallback) {
                        out.error(node_where, "branches have no unconditional fallback");
                    }
                }
            } else if (type == "combat") {
                const std::string encounter = node->value("encounter", std::string{});
                if (!dialogue.has_encounter(encounter)) {
                    out.error(node_where, "unknown encounter " + encounter);
                }
                for (const char* key : {"on_win", "on_lose"}) {
                    if (!node->contains(key) || node->at(key).is_null()) {
                        continue;
                    }
                    const std::string next = node->at(key).get<std::string>();
                    if (dialogue.find_node(next) == nullptr) {
                        out.error(node_where,
                                  std::string(key) + " points to missing node " + next);
                    }
                }
            } else if (type == "register") {
                if (!node->contains("names") || !node->at("names").is_array() ||
                    node->at("names").empty()) {
                    out.error(node_where, "register node has no names");
                } else {
                    for (const Json& name : node->at("names")) {
                        if (!name.is_string() || name.get<std::string>().empty()) {
                            out.error(node_where,
                                      "register name must be a non-empty string");
                            break;
                        }
                    }
                }
                check_next(*node, node_where);
            } else if (type == "seal") {
                if (node->contains("text") && !node->at("text").is_string()) {
                    out.error(node_where, "seal text must be a string");
                }
                if (node->contains("seal") && !node->at("seal").is_string()) {
                    out.error(node_where, "seal id must be a string");
                }
                check_next(*node, node_where);
            } else if (type == "end") {
                // Nothing else to check.
            } else {
                out.error(node_where, "unknown node type '" + type + "'");
            }
        }
    }

    return out.take();
}

} // namespace han::narrative

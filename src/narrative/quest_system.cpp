#include "quest_system.h"

#include <exception>

namespace han::narrative {
namespace {

bool fail(std::string* error, std::string message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
    return false;
}

} // namespace

bool QuestSystem::load(const Json& doc, std::string* error) {
    try {
        for (const Json& item : doc.at("quests")) {
            QuestDef quest;
            quest.id = item.at("id").get<std::string>();
            quest.type = item.value("type", std::string("side"));
            quest.title = item.value("title", std::string{});
            quest.summary = item.value("summary", std::string{});
            quest.location = item.value("location", std::string{});
            if (item.contains("prerequisites")) {
                quest.prerequisites = item.at("prerequisites");
            }
            for (const Json& objective : item.at("objectives")) {
                ObjectiveDef def;
                def.id = objective.at("id").get<std::string>();
                def.text = objective.value("text", std::string{});
                def.optional = objective.value("optional", false);
                def.condition = objective.at("condition");
                quest.objectives.push_back(std::move(def));
            }
            if (item.contains("completion")) {
                quest.completion = item.at("completion");
            }
            for (const Json& reward : item.value("rewards", Json::array())) {
                RewardDef def;
                def.type = reward.at("type").get<std::string>();
                def.id = reward.value("id", std::string{});
                if (reward.contains("condition")) {
                    def.condition = reward.at("condition");
                }
                def.delta = reward.value("delta", 0);
                quest.rewards.push_back(std::move(def));
            }
            for (const Json& unlock : item.value("unlocks", Json::array())) {
                quest.unlocks.push_back(unlock.get<std::string>());
            }
            for (const Json& branch : item.value("branches", Json::array())) {
                QuestBranchDef def;
                def.id = branch.value("id", std::string{});
                if (branch.contains("condition")) {
                    def.condition = branch.at("condition");
                }
                def.effects = branch.value("effects", Json::array());
                quest.branches.push_back(std::move(def));
            }

            if (find(quest.id) != nullptr) {
                return fail(error, "duplicate quest id " + quest.id);
            }
            quests_.push_back(std::move(quest));
        }
    } catch (const std::exception& e) {
        return fail(error, std::string("quest load failed: ") + e.what());
    }
    return true;
}

void QuestSystem::reset(NarrativeState& state) {
    state.clear_quest_progress();
    for (const QuestDef& quest : quests_) {
        state.quest_status(quest.id);
    }
    started_.clear();
    completed_.clear();
}

const QuestDef* QuestSystem::find(const std::string& id) const {
    for (const QuestDef& quest : quests_) {
        if (quest.id == id) {
            return &quest;
        }
    }
    return nullptr;
}

void QuestSystem::clear_events() {
    started_.clear();
    completed_.clear();
}

void QuestSystem::update(NarrativeState& state) {
    const EvalContext context{&state, {}};
    bool progress = true;
    for (int pass = 0; pass < 16 && progress; ++pass) {
        progress = false;

        for (const QuestDef& quest : quests_) {
            QuestStatus& status = state.quest_status(quest.id);

            if (status.state == QuestState::Inactive) {
                if (quest.prerequisites.is_null() ||
                    conditions_.evaluate(quest.prerequisites, context, nullptr)) {
                    status.state = QuestState::Active;
                    started_.push_back(quest.id);
                    progress = true;
                }
            }

            if (status.state != QuestState::Active) {
                continue;
            }

            for (const ObjectiveDef& objective : quest.objectives) {
                if (status.objectives_done.find(objective.id) !=
                    status.objectives_done.end()) {
                    continue;
                }
                if (conditions_.evaluate(objective.condition, context, nullptr)) {
                    status.objectives_done.insert(objective.id);
                    state.complete_objective(objective.id);
                    progress = true;
                }
            }

            if (!completion_met(quest, state)) {
                continue;
            }

            status.state = QuestState::Completed;
            apply_rewards(quest, state);
            apply_branches(quest, state);
            completed_.push_back(quest.id);
            progress = true;
        }
    }
}

bool QuestSystem::completion_met(const QuestDef& quest, const NarrativeState& state) const {
    if (!quest.completion.is_object() || !quest.completion.contains("all")) {
        return false;
    }
    const Json& all = quest.completion.at("all");
    if (!all.is_array() || all.empty()) {
        return false;
    }
    for (const Json& requirement : all) {
        if (!requirement.contains("objective")) {
            return false;
        }
        if (!state.objective_done(requirement.at("objective").get<std::string>())) {
            return false;
        }
    }
    return true;
}

void QuestSystem::apply_rewards(const QuestDef& quest, NarrativeState& state) const {
    const EvalContext context{&state, {}};
    for (const RewardDef& reward : quest.rewards) {
        if (!reward.condition.is_null() &&
            !conditions_.evaluate(reward.condition, context, nullptr)) {
            continue;
        }
        if (reward.type == "clue") {
            state.add_clue(reward.id);
        } else if (reward.type == "item") {
            state.give_item(reward.id);
        } else if (reward.type == "variable") {
            state.add_int_var(reward.id, reward.delta);
        } else if (reward.type == "flag") {
            state.set_flag(reward.id, true);
        } else if (reward.type == "ability") {
            state.add_ability(reward.id);
        }
    }
}

void QuestSystem::apply_branches(const QuestDef& quest, NarrativeState& state) const {
    const EvalContext context{&state, {}};
    for (const QuestBranchDef& branch : quest.branches) {
        if (!branch.condition.is_null() &&
            !conditions_.evaluate(branch.condition, context, nullptr)) {
            continue;
        }
        effects_.apply(branch.effects, state, {}, nullptr);
    }
}

} // namespace han::narrative

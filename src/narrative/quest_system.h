#pragma once

#include "effects.h"
#include "narrative_state.h"
#include "narrative_types.h"

#include <string>
#include <vector>

namespace han::narrative {

struct ObjectiveDef {
    std::string id;
    std::string text;
    bool optional = false;
    Json condition;
};

struct RewardDef {
    std::string type;
    std::string id;
    Json condition;
    int delta = 0;
};

struct QuestBranchDef {
    std::string id;
    Json condition;
    Json effects;
};

struct QuestDef {
    std::string id;
    std::string type;
    std::string title;
    std::string summary;
    std::string location;
    Json prerequisites;
    std::vector<ObjectiveDef> objectives;
    Json completion;
    std::vector<RewardDef> rewards;
    std::vector<std::string> unlocks;
    std::vector<QuestBranchDef> branches;
};

/// Data-driven quest state machine. All progress lives in NarrativeState so it
/// can be snapshotted and tested without any rendering layer.
class QuestSystem {
public:
    bool load(const Json& doc, std::string* error = nullptr);
    void reset(NarrativeState& state);
    void update(NarrativeState& state);

    const std::vector<QuestDef>& definitions() const { return quests_; }
    const QuestDef* find(const std::string& id) const;
    std::size_t size() const { return quests_.size(); }

    const std::vector<std::string>& started_events() const { return started_; }
    const std::vector<std::string>& completed_events() const { return completed_; }
    void clear_events();

private:
    bool completion_met(const QuestDef& quest, const NarrativeState& state) const;
    bool apply_rewards(const QuestDef& quest, QuestStatus& status,
                       NarrativeState& state) const;
    bool apply_branches(const QuestDef& quest, QuestStatus& status,
                        NarrativeState& state) const;

    std::vector<QuestDef> quests_;
    ConditionEvaluator conditions_;
    EffectExecutor effects_;
    std::vector<std::string> started_;
    std::vector<std::string> completed_;
};

} // namespace han::narrative

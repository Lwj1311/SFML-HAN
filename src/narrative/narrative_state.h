#pragma once

#include "narrative_types.h"

#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace han::narrative {

struct VariableDef {
    enum class Type {
        Int,
        Enum,
    };

    Type type = Type::Int;
    int min_value = std::numeric_limits<int>::min();
    int max_value = std::numeric_limits<int>::max();
    std::vector<std::string> values;
};

struct QuestStatus {
    QuestState state = QuestState::Inactive;
    std::set<std::string> objectives_done;
};

/// Authoritative, renderer-independent narrative state for one chapter.
///
/// All conditions and effects operate on this object. It deliberately has no
/// SFML dependency so the whole narrative layer can be tested headlessly.
class NarrativeState {
public:
    bool load_initial(const Json& doc, std::string* error = nullptr);

    // --- reads ---
    bool flag(const std::string& id) const;
    int int_var(const std::string& id) const;
    std::string string_var(const std::string& id) const;
    bool has_clue(const std::string& id) const;
    bool has_item(const std::string& id) const;
    int item_count(const std::string& id) const;
    bool has_ability(const std::string& id) const;
    QuestState quest_state(const std::string& id) const;
    bool objective_done(const std::string& id) const;

    // --- writes ---
    void set_flag(const std::string& id, bool value);
    void add_int_var(const std::string& id, int delta);
    void set_int_var(const std::string& id, int value);
    void set_string_var(const std::string& id, std::string value);
    void give_item(const std::string& id, int count = 1);
    void take_item(const std::string& id, int count = 1);
    void add_clue(const std::string& id);
    void add_ability(const std::string& id);

    QuestStatus& quest_status(const std::string& id);
    const QuestStatus& quest_status(const std::string& id) const;
    void set_quest_state(const std::string& id, QuestState state);
    void complete_objective(const std::string& id);
    void clear_quest_progress();

    // --- chapter declarations, used by the validator ---
    bool declares_flag(const std::string& id) const;
    bool declares_variable(const std::string& id) const;
    bool declares_item(const std::string& id) const;
    bool declares_clue(const std::string& id) const;
    const VariableDef* variable_def(const std::string& id) const;

    const std::map<std::string, bool>& flags() const { return flags_; }
    const std::map<std::string, int>& int_vars() const { return ints_; }
    const std::map<std::string, std::string>& string_vars() const { return strings_; }
    const std::set<std::string>& clues() const { return clues_; }
    const std::map<std::string, int>& items() const { return items_; }
    const std::map<std::string, QuestStatus>& quests() const { return quests_; }
    const std::set<std::string>& completed_objectives() const { return objectives_done_; }

private:
    int clamp_int(const std::string& id, int value) const;

    std::map<std::string, bool> flags_;
    std::map<std::string, int> ints_;
    std::map<std::string, std::string> strings_;
    std::set<std::string> clues_;
    std::map<std::string, int> items_;
    std::set<std::string> abilities_;
    std::map<std::string, QuestStatus> quests_;
    std::set<std::string> objectives_done_;

    std::set<std::string> declared_flags_;
    std::map<std::string, VariableDef> variables_;
    std::set<std::string> declared_items_;
    std::set<std::string> declared_clues_;
};

} // namespace han::narrative

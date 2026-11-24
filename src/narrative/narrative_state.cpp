#include "narrative_state.h"

#include <algorithm>
#include <exception>

namespace han::narrative {

bool NarrativeState::load_initial(const Json& doc, std::string* error) {
    try {
        const Json& vars = doc.at("variables");
        for (auto it = vars.begin(); it != vars.end(); ++it) {
            const Json& def_json = it.value();
            VariableDef def;
            const std::string type = def_json.at("type").get<std::string>();
            if (type == "int") {
                def.type = VariableDef::Type::Int;
                def.min_value = def_json.value("min", std::numeric_limits<int>::min());
                def.max_value = def_json.value("max", std::numeric_limits<int>::max());
            } else if (type == "enum") {
                def.type = VariableDef::Type::Enum;
                for (const Json& value : def_json.at("values")) {
                    def.values.push_back(value.get<std::string>());
                }
            } else {
                if (error != nullptr) {
                    *error = "variable " + it.key() + ": unknown type " + type;
                }
                return false;
            }
            variables_[it.key()] = std::move(def);
        }

        const Json& initial = doc.at("initial");

        for (auto it = initial.at("flags").begin(); it != initial.at("flags").end(); ++it) {
            declared_flags_.insert(it.key());
            flags_[it.key()] = it.value().get<bool>();
        }

        for (auto it = initial.at("variables").begin(); it != initial.at("variables").end(); ++it) {
            const Json& value = it.value();
            const auto def = variables_.find(it.key());
            if (value.is_string()) {
                strings_[it.key()] = value.get<std::string>();
            } else if (value.is_number_integer()) {
                const int raw = value.get<int>();
                ints_[it.key()] = (def != variables_.end()) ? clamp_int(it.key(), raw) : raw;
            } else if (value.is_boolean()) {
                ints_[it.key()] = value.get<bool>() ? 1 : 0;
            } else {
                if (error != nullptr) {
                    *error = "initial variable " + it.key() + ": unsupported value type";
                }
                return false;
            }
        }

        for (auto it = doc.at("items").begin(); it != doc.at("items").end(); ++it) {
            declared_items_.insert(it.key());
        }
        for (auto it = doc.at("clues").begin(); it != doc.at("clues").end(); ++it) {
            declared_clues_.insert(it.key());
        }
    } catch (const std::exception& e) {
        if (error != nullptr) {
            *error = std::string("state load failed: ") + e.what();
        }
        return false;
    }
    return true;
}

bool NarrativeState::flag(const std::string& id) const {
    const auto it = flags_.find(id);
    return it != flags_.end() && it->second;
}

int NarrativeState::int_var(const std::string& id) const {
    const auto it = ints_.find(id);
    return it == ints_.end() ? 0 : it->second;
}

std::string NarrativeState::string_var(const std::string& id) const {
    const auto it = strings_.find(id);
    return it == strings_.end() ? std::string{} : it->second;
}

bool NarrativeState::has_clue(const std::string& id) const {
    return clues_.find(id) != clues_.end();
}

bool NarrativeState::has_item(const std::string& id) const {
    return item_count(id) > 0;
}

int NarrativeState::item_count(const std::string& id) const {
    const auto it = items_.find(id);
    return it == items_.end() ? 0 : it->second;
}

bool NarrativeState::has_ability(const std::string& id) const {
    return abilities_.find(id) != abilities_.end();
}

QuestState NarrativeState::quest_state(const std::string& id) const {
    const auto it = quests_.find(id);
    return it == quests_.end() ? QuestState::Inactive : it->second.state;
}

bool NarrativeState::objective_done(const std::string& id) const {
    return objectives_done_.find(id) != objectives_done_.end();
}

void NarrativeState::set_flag(const std::string& id, bool value) {
    flags_[id] = value;
}

void NarrativeState::add_int_var(const std::string& id, int delta) {
    ints_[id] = clamp_int(id, int_var(id) + delta);
}

void NarrativeState::set_int_var(const std::string& id, int value) {
    ints_[id] = clamp_int(id, value);
}

void NarrativeState::set_string_var(const std::string& id, std::string value) {
    strings_[id] = std::move(value);
}

void NarrativeState::give_item(const std::string& id, int count) {
    items_[id] += count;
}

void NarrativeState::take_item(const std::string& id, int count) {
    const auto it = items_.find(id);
    if (it == items_.end()) {
        return;
    }
    it->second -= count;
    if (it->second <= 0) {
        items_.erase(it);
    }
}

void NarrativeState::add_clue(const std::string& id) {
    clues_.insert(id);
}

void NarrativeState::add_ability(const std::string& id) {
    abilities_.insert(id);
}

QuestStatus& NarrativeState::quest_status(const std::string& id) {
    return quests_[id];
}

const QuestStatus& NarrativeState::quest_status(const std::string& id) const {
    static const QuestStatus inactive{};
    const auto it = quests_.find(id);
    return it == quests_.end() ? inactive : it->second;
}

void NarrativeState::set_quest_state(const std::string& id, QuestState state) {
    quests_[id].state = state;
}

void NarrativeState::complete_objective(const std::string& id) {
    objectives_done_.insert(id);
}

void NarrativeState::clear_quest_progress() {
    quests_.clear();
    objectives_done_.clear();
}

Json NarrativeState::snapshot() const {
    Json doc;
    doc["flags"] = flags_;
    doc["variables"] = Json{{"int", ints_}, {"string", strings_}};

    doc["clues"] = Json::array();
    for (const std::string& clue : clues_) {
        doc["clues"].push_back(clue);
    }

    doc["items"] = items_;

    doc["abilities"] = Json::array();
    for (const std::string& ability : abilities_) {
        doc["abilities"].push_back(ability);
    }

    doc["objectives"] = Json::array();
    for (const std::string& objective : objectives_done_) {
        doc["objectives"].push_back(objective);
    }

    Json quests = Json::object();
    for (const auto& [id, status] : quests_) {
        Json quest;
        quest["state"] = to_string(status.state);
        quest["objectives"] = Json::array();
        for (const std::string& objective : status.objectives_done) {
            quest["objectives"].push_back(objective);
        }
        quest["rewards_applied"] = Json::array();
        for (const std::string& reward : status.rewards_applied) {
            quest["rewards_applied"].push_back(reward);
        }
        quest["branches_applied"] = Json::array();
        for (const std::string& branch : status.branches_applied) {
            quest["branches_applied"].push_back(branch);
        }
        quests[id] = std::move(quest);
    }
    doc["quests"] = std::move(quests);
    return doc;
}

bool NarrativeState::restore(const Json& doc, std::string* error) {
    try {
        flags_ = doc.value("flags", std::map<std::string, bool>{});

        const Json variables =
            doc.contains("variables") ? doc.at("variables") : Json::object();
        ints_.clear();
        if (variables.contains("int")) {
            for (auto it = variables.at("int").begin(); it != variables.at("int").end();
                 ++it) {
                ints_[it.key()] = clamp_int(it.key(), it.value().get<int>());
            }
        }
        strings_ = variables.value("string", std::map<std::string, std::string>{});

        clues_.clear();
        for (const Json& clue : doc.value("clues", Json::array())) {
            clues_.insert(clue.get<std::string>());
        }

        items_ = doc.value("items", std::map<std::string, int>{});

        abilities_.clear();
        for (const Json& ability : doc.value("abilities", Json::array())) {
            abilities_.insert(ability.get<std::string>());
        }

        objectives_done_.clear();
        for (const Json& objective : doc.value("objectives", Json::array())) {
            objectives_done_.insert(objective.get<std::string>());
        }

        quests_.clear();
        if (doc.contains("quests") && doc.at("quests").is_object()) {
            for (auto it = doc.at("quests").begin(); it != doc.at("quests").end(); ++it) {
                const Json& source = it.value();
                QuestStatus status;
                const std::string state = source.value("state", std::string("inactive"));
                if (state == "active") {
                    status.state = QuestState::Active;
                } else if (state == "completed") {
                    status.state = QuestState::Completed;
                }
                for (const Json& objective : source.value("objectives", Json::array())) {
                    status.objectives_done.insert(objective.get<std::string>());
                }
                for (const Json& reward : source.value("rewards_applied", Json::array())) {
                    status.rewards_applied.insert(reward.get<std::string>());
                }
                for (const Json& branch : source.value("branches_applied", Json::array())) {
                    status.branches_applied.insert(branch.get<std::string>());
                }
                quests_[it.key()] = std::move(status);
            }
        }
    } catch (const std::exception& e) {
        if (error != nullptr) {
            *error = std::string("state restore failed: ") + e.what();
        }
        return false;
    }
    return true;
}

bool NarrativeState::declares_flag(const std::string& id) const {
    return declared_flags_.find(id) != declared_flags_.end();
}

bool NarrativeState::declares_variable(const std::string& id) const {
    return variables_.find(id) != variables_.end();
}

bool NarrativeState::declares_item(const std::string& id) const {
    return declared_items_.find(id) != declared_items_.end();
}

bool NarrativeState::declares_clue(const std::string& id) const {
    return declared_clues_.find(id) != declared_clues_.end();
}

const VariableDef* NarrativeState::variable_def(const std::string& id) const {
    const auto it = variables_.find(id);
    return it == variables_.end() ? nullptr : &it->second;
}

int NarrativeState::clamp_int(const std::string& id, int value) const {
    const auto it = variables_.find(id);
    if (it == variables_.end() || it->second.type != VariableDef::Type::Int) {
        return value;
    }
    return std::clamp(value, it->second.min_value, it->second.max_value);
}

} // namespace han::narrative

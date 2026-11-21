#include "session.h"

#include <algorithm>
#include <fstream>
#include <iterator>

namespace han::narrative {
namespace {

bool read_json(const std::filesystem::path& path, Json& out, std::string* error) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        if (error != nullptr) {
            *error = "cannot open " + path.string();
        }
        return false;
    }
    try {
        out = Json::parse(stream);
    } catch (const std::exception& e) {
        if (error != nullptr) {
            *error = path.string() + ": " + e.what();
        }
        return false;
    }
    return true;
}

bool fail(std::string* error, std::string message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
    return false;
}

} // namespace

bool NarrativeSession::load(const std::filesystem::path& data_dir,
                            const std::string& chapter_id, std::string* error) {
    chapter_id_ = chapter_id;
    if (!read_json(data_dir / (chapter_id + ".state.json"), state_doc_, error)) {
        return false;
    }
    if (!read_json(data_dir / (chapter_id + ".quests.json"), quest_doc_, error)) {
        return false;
    }
    if (!read_json(data_dir / (chapter_id + ".dialogue.json"), dialogue_doc_, error)) {
        return false;
    }

    state_ = std::make_unique<NarrativeState>();
    if (!state_->load_initial(state_doc_, error)) {
        return false;
    }

    quests_ = std::make_unique<QuestSystem>();
    if (!quests_->load(quest_doc_, error)) {
        return false;
    }
    quests_->reset(*state_);

    document_ = std::make_unique<DialogueDocument>();
    if (!document_->load(dialogue_doc_, error)) {
        return false;
    }

    runner_ = std::make_unique<DialogueRunner>(*document_, *state_);
    return true;
}

std::string NarrativeSession::chapter_title() const {
    return state_doc_.value("title", std::string{});
}

std::string NarrativeSession::next_chapter() const {
    return state_doc_.value("next_chapter", std::string{});
}

bool NarrativeSession::load(const std::filesystem::path& data_dir,
                            const std::string& chapter_id,
                            const NarrativeSession& previous, std::string* error) {
    if (!load(data_dir, chapter_id, error)) {
        return false;
    }
    return carry_from(previous, error);
}

bool NarrativeSession::carry_from(const NarrativeSession& previous, std::string* error) {
    const auto spec = state_doc_.find("carryover");
    if (spec == state_doc_.end()) {
        return true;
    }
    if (!spec->is_object()) {
        return fail(error, "carryover must be an object");
    }

    const std::string from = spec->value("from", std::string{});
    if (!from.empty() && from != previous.chapter_id()) {
        return fail(error, "carryover expects " + from + " but got " + previous.chapter_id());
    }

    if (const auto vars = spec->find("variables"); vars != spec->end()) {
        if (!vars->is_array()) {
            return fail(error, "carryover.variables must be an array");
        }
        for (const Json& entry : *vars) {
            if (!entry.is_string()) {
                return fail(error, "carryover.variables entries must be strings");
            }
            const std::string id = entry.get<std::string>();
            const VariableDef* def = previous.state().variable_def(id);
            if (def == nullptr) {
                return fail(error, "carryover variable " + id + " is not declared in " +
                                       previous.chapter_id());
            }
            if (!state().declares_variable(id)) {
                return fail(error, "carryover variable " + id + " is not declared in " +
                                       chapter_id_);
            }
            if (def->type == VariableDef::Type::Enum) {
                state().set_string_var(id, previous.state().string_var(id));
            } else {
                state().set_int_var(id, previous.state().int_var(id));
            }
        }
    }

    if (const auto flags = spec->find("flags"); flags != spec->end()) {
        if (!flags->is_array()) {
            return fail(error, "carryover.flags must be an array");
        }
        for (const Json& entry : *flags) {
            if (!entry.is_string()) {
                return fail(error, "carryover.flags entries must be strings");
            }
            const std::string id = entry.get<std::string>();
            if (!previous.state().declares_flag(id)) {
                return fail(error, "carryover flag " + id + " is not declared in " +
                                       previous.chapter_id());
            }
            if (!state().declares_flag(id)) {
                return fail(error, "carryover flag " + id + " is not declared in " + chapter_id_);
            }
            state().set_flag(id, previous.state().flag(id));
        }
    }

    // Carried flags can satisfy quest prerequisites, so re-evaluate them.
    quests_->reset(*state_);
    return true;
}

void NarrativeSession::start() {
    played_.clear();
    active_scene_.clear();
    sync();
}

std::optional<std::string> NarrativeSession::pick_scene() const {
    const EvalContext context{&state(), {}};
    ConditionEvaluator conditions;
    for (const SceneDef& scene : document_->scenes()) {
        if (!scene.has_trigger) {
            continue;
        }
        if (std::find(played_.begin(), played_.end(), scene.id) != played_.end()) {
            continue;
        }
        if (conditions.evaluate(scene.trigger, context, nullptr)) {
            return scene.id;
        }
    }
    return std::nullopt;
}

bool NarrativeSession::play_scene(const std::string& scene_id, std::string* error) {
    if (!active_scene_.empty()) {
        return fail(error, "scene " + active_scene_ + " is still active");
    }
    if (std::find(played_.begin(), played_.end(), scene_id) != played_.end()) {
        return fail(error, "scene " + scene_id + " has already been played");
    }
    if (!runner_->start_scene(scene_id, error)) {
        return false;
    }

    played_.push_back(scene_id);
    active_scene_ = scene_id;
    sync();
    finish_if_needed();
    return true;
}

bool NarrativeSession::play_next_scene(std::string* error) {
    const std::optional<std::string> scene_id = pick_scene();
    if (!scene_id.has_value()) {
        return false;
    }
    return play_scene(*scene_id, error);
}

void NarrativeSession::advance() {
    runner_->advance();
    sync();
    finish_if_needed();
}

bool NarrativeSession::choose(const std::string& choice_id, std::string* error) {
    if (!runner_->choose(choice_id, error)) {
        return false;
    }
    sync();
    finish_if_needed();
    return true;
}

void NarrativeSession::resolve_combat(bool win) {
    runner_->resolve_combat(win);
    sync();
    finish_if_needed();
}

void NarrativeSession::sync() {
    quests_->update(*state_);
}

void NarrativeSession::finish_if_needed() {
    if (!active_scene_.empty() && runner_->finished()) {
        active_scene_.clear();
        sync();
    }
}

} // namespace han::narrative

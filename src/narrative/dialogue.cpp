#include "dialogue.h"

#include "narrative_state.h"

#include <exception>
#include <stdexcept>

namespace han::narrative {
namespace {

bool fail(std::string* error, std::string message) {
    if (error != nullptr) {
        *error = std::move(message);
    }
    return false;
}

std::string read_next(const Json& selected, const Json& fallback) {
    if (selected.contains("next")) {
        return selected.at("next").is_null() ? std::string{}
                                             : selected.at("next").get<std::string>();
    }
    if (&selected != &fallback && fallback.contains("next")) {
        return fallback.at("next").is_null() ? std::string{}
                                             : fallback.at("next").get<std::string>();
    }
    return {};
}

} // namespace

bool DialogueDocument::load(const Json& doc, std::string* error) {
    scenes_.clear();
    nodes_.clear();
    speakers_.clear();
    encounters_.clear();

    try {
        for (auto it = doc.at("speakers").begin(); it != doc.at("speakers").end(); ++it) {
            SpeakerDef speaker;
            speaker.name = it.value().value("name", it.key());
            if (it.value().contains("portrait") && !it.value().at("portrait").is_null()) {
                speaker.portrait = it.value().at("portrait").get<std::string>();
            }
            speakers_[it.key()] = std::move(speaker);
        }

        for (auto it = doc.at("encounters").begin(); it != doc.at("encounters").end(); ++it) {
            encounters_[it.key()] = it.value();
        }

        for (const Json& scene_json : doc.at("scenes")) {
            SceneDef scene;
            scene.id = scene_json.at("id").get<std::string>();
            scene.title = scene_json.value("title", std::string{});
            scene.background = scene_json.value("background", std::string{});
            scene.entry = scene_json.value("entry", std::string{});
            if (scene_json.contains("trigger")) {
                scene.trigger = scene_json.at("trigger");
                scene.has_trigger = true;
            }

            for (const Json& node : scene_json.at("nodes")) {
                const std::string node_id = node.at("id").get<std::string>();
                if (nodes_.find(node_id) != nodes_.end()) {
                    return fail(error, "duplicate dialogue node id " + node_id);
                }
                nodes_.emplace(node_id, node);
                scene.node_ids.push_back(node_id);
            }
            scenes_.push_back(std::move(scene));
        }
    } catch (const std::exception& e) {
        return fail(error, std::string("dialogue load failed: ") + e.what());
    }
    return true;
}

const SceneDef* DialogueDocument::find_scene(const std::string& id) const {
    for (const SceneDef& scene : scenes_) {
        if (scene.id == id) {
            return &scene;
        }
    }
    return nullptr;
}

const Json* DialogueDocument::find_node(const std::string& id) const {
    const auto it = nodes_.find(id);
    return it == nodes_.end() ? nullptr : &it->second;
}

const SpeakerDef* DialogueDocument::find_speaker(const std::string& id) const {
    const auto it = speakers_.find(id);
    return it == speakers_.end() ? nullptr : &it->second;
}

bool DialogueDocument::has_encounter(const std::string& id) const {
    return encounters_.find(id) != encounters_.end();
}

DialogueRunner::DialogueRunner(const DialogueDocument& document, NarrativeState& state)
    : document_(document), state_(state) {}

bool DialogueRunner::start_scene(const std::string& scene_id, std::string* error) {
    const SceneDef* scene = document_.find_scene(scene_id);
    if (scene == nullptr) {
        return fail(error, "unknown scene " + scene_id);
    }
    if (scene->entry.empty() || document_.find_node(scene->entry) == nullptr) {
        return fail(error, "scene " + scene_id + " has missing entry " + scene->entry);
    }

    scene_id_ = scene_id;
    background_ = scene->background;
    last_choice_.clear();
    pending_next_.clear();
    current_ = NodeView{};
    finished_ = false;
    steps_ = 0;
    enter(scene->entry, 0);
    return true;
}

void DialogueRunner::advance() {
    if (current_.kind != NodeKind::Line && current_.kind != NodeKind::Register &&
        current_.kind != NodeKind::Seal) {
        return;
    }
    const std::string next = pending_next_;
    pending_next_.clear();
    enter(next, 0);
}

bool DialogueRunner::choose(const std::string& choice_id, std::string* error) {
    if (current_.kind != NodeKind::Choice) {
        return fail(error, "current node is not a choice");
    }
    const Json* node = document_.find_node(current_.node_id);
    if (node == nullptr || !node->contains("choices")) {
        return fail(error, "choice node " + current_.node_id + " is malformed");
    }

    const Json* selected = nullptr;
    for (const Json& choice : node->at("choices")) {
        if (choice.value("id", std::string{}) == choice_id) {
            selected = &choice;
            break;
        }
    }
    if (selected == nullptr) {
        return fail(error, "unknown choice id " + choice_id + " in node " + node->at("id").get<std::string>());
    }

    last_choice_ = choice_id;
    if (selected->contains("effects")) {
        if (!effects_.apply(selected->at("effects"), state_, last_choice_, error)) {
            return false;
        }
    }
    const std::string next =
        selected->contains("next") && !selected->at("next").is_null()
            ? selected->at("next").get<std::string>()
            : std::string{};
    enter(next, 0);
    return true;
}

void DialogueRunner::resolve_combat(bool win) {
    if (current_.kind != NodeKind::Combat) {
        return;
    }
    const Json* node = document_.find_node(current_.node_id);
    if (node == nullptr) {
        finish_scene();
        return;
    }
    const char* key = win ? "on_win" : "on_lose";
    if (!node->contains(key) || node->at(key).is_null()) {
        finish_scene();
        return;
    }
    enter(node->at(key).get<std::string>(), 0);
}

EvalContext DialogueRunner::context() const {
    return EvalContext{&state_, last_choice_};
}

void DialogueRunner::finish_scene() {
    current_ = NodeView{};
    current_.scene_id = scene_id_;
    finished_ = true;
}

void DialogueRunner::enter(const std::string& node_id, int depth) {
    if (depth > 64) {
        throw std::runtime_error("dialogue branch nesting too deep at " + node_id);
    }
    if (++steps_ > 4096) {
        throw std::runtime_error("dialogue step limit exceeded at " + node_id);
    }
    if (node_id.empty()) {
        finish_scene();
        return;
    }

    const Json* node = document_.find_node(node_id);
    if (node == nullptr) {
        throw std::runtime_error("missing dialogue node " + node_id);
    }

    current_ = NodeView{};
    current_.scene_id = scene_id_;
    current_.node_id = node_id;

    if (node->contains("effects")) {
        effects_.apply(node->at("effects"), state_, last_choice_, nullptr);
    }

    const std::string type = node->value("type", std::string{});

    if (type == "narration" || type == "line") {
        const Json* variant = nullptr;
        if (node->contains("variants")) {
            for (const Json& candidate : node->at("variants")) {
                if (!candidate.contains("condition") ||
                    conditions_.evaluate(candidate.at("condition"), context(), nullptr)) {
                    variant = &candidate;
                    break;
                }
            }
            if (variant == nullptr) {
                throw std::runtime_error("no matching variant in node " + node_id);
            }
            if (variant->contains("effects")) {
                effects_.apply(variant->at("effects"), state_, last_choice_, nullptr);
            }
        }

        const Json& selected = variant != nullptr ? *variant : *node;
        const std::string speaker =
            selected.contains("speaker") ? selected.at("speaker").get<std::string>()
                                         : node->value("speaker", std::string("narrator"));
        const SpeakerDef* speaker_def = document_.find_speaker(speaker);

        current_.kind = NodeKind::Line;
        current_.line.speaker_id = speaker;
        current_.line.speaker_name = speaker_def != nullptr ? speaker_def->name : speaker;
        current_.line.portrait = speaker_def != nullptr ? speaker_def->portrait : std::string{};
        current_.line.text = selected.value("text", std::string{});
        pending_next_ = read_next(selected, *node);
        return;
    }

    if (type == "choice") {
        current_.kind = NodeKind::Choice;
        current_.prompt = node->value("prompt", std::string{});
        for (const Json& choice : node->at("choices")) {
            current_.choices.push_back(
                ChoiceOption{choice.at("id").get<std::string>(),
                             choice.value("text", std::string{})});
        }
        return;
    }

    if (type == "branch") {
        for (const Json& branch : node->at("branches")) {
            if (!branch.contains("condition") ||
                conditions_.evaluate(branch.at("condition"), context(), nullptr)) {
                const std::string next =
                    branch.contains("next") && !branch.at("next").is_null()
                        ? branch.at("next").get<std::string>()
                        : std::string{};
                enter(next, depth + 1);
                return;
            }
        }
        finish_scene();
        return;
    }

    if (type == "combat") {
        current_.kind = NodeKind::Combat;
        current_.encounter = node->value("encounter", std::string{});
        return;
    }

    if (type == "register") {
        current_.kind = NodeKind::Register;
        current_.register_view.title = node->value("title", std::string("名籍"));
        current_.register_view.subtitle = node->value("subtitle", std::string{});
        if (node->contains("names")) {
            for (const Json& name : node->at("names")) {
                if (name.is_string()) {
                    current_.register_view.names.push_back(name.get<std::string>());
                }
            }
        }
        pending_next_ = read_next(*node, *node);
        return;
    }

    if (type == "seal") {
        current_.kind = NodeKind::Seal;
        current_.seal_view.title = node->value("title", std::string{});
        current_.seal_view.text = node->value("text", std::string{});
        current_.seal_view.seal_id =
            node->value("seal", std::string("ui_ending_seal"));
        pending_next_ = read_next(*node, *node);
        return;
    }

    if (type == "end") {
        current_.kind = NodeKind::End;
        current_.exit = node->value("exit", std::string{});
        finished_ = true;
        return;
    }

    throw std::runtime_error("unknown dialogue node type '" + type + "' at " + node_id);
}

} // namespace han::narrative

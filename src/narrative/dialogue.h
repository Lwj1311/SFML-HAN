#pragma once

#include "effects.h"
#include "narrative_types.h"

#include <map>
#include <string>
#include <vector>

namespace han::narrative {

struct SpeakerDef {
    std::string name;
    std::string portrait;
};

struct SceneDef {
    std::string id;
    std::string title;
    std::string background;
    std::string entry;
    Json trigger;
    bool has_trigger = false;
    std::vector<std::string> node_ids;
};

/// Parsed chapter dialogue file.
///
/// Nodes are indexed chapter-wide rather than per scene: several scenes are
/// alternative entry points into one shared node graph (for example the
/// counter scene jumps into the stable scene's continuation).
class DialogueDocument {
public:
    bool load(const Json& doc, std::string* error = nullptr);

    const SceneDef* find_scene(const std::string& id) const;
    const Json* find_node(const std::string& id) const;
    const SpeakerDef* find_speaker(const std::string& id) const;
    bool has_encounter(const std::string& id) const;

    const std::vector<SceneDef>& scenes() const { return scenes_; }
    const std::map<std::string, Json>& nodes() const { return nodes_; }
    const std::map<std::string, SpeakerDef>& speakers() const { return speakers_; }
    const std::map<std::string, Json>& encounters() const { return encounters_; }
    std::size_t node_count() const { return nodes_.size(); }
    std::size_t scene_count() const { return scenes_.size(); }

private:
    std::vector<SceneDef> scenes_;
    std::map<std::string, Json> nodes_;
    std::map<std::string, SpeakerDef> speakers_;
    std::map<std::string, Json> encounters_;
};

/// Walks the dialogue graph for one chapter and exposes the current node.
class DialogueRunner {
public:
    DialogueRunner(const DialogueDocument& document, NarrativeState& state);

    bool start_scene(const std::string& scene_id, std::string* error = nullptr);

    void advance();
    bool choose(const std::string& choice_id, std::string* error = nullptr);
    void resolve_combat(bool win);

    const NodeView& current() const { return current_; }
    bool finished() const { return finished_; }
    const std::string& scene_id() const { return scene_id_; }
    const std::string& background() const { return background_; }

private:
    void enter(const std::string& node_id, int depth);
    void finish_scene();
    EvalContext context() const;

    const DialogueDocument& document_;
    NarrativeState& state_;
    ConditionEvaluator conditions_;
    EffectExecutor effects_;

    NodeView current_;
    std::string pending_next_;
    std::string scene_id_;
    std::string background_;
    std::string last_choice_;
    bool finished_ = true;
    int steps_ = 0;
};

} // namespace han::narrative

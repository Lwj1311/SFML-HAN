#pragma once

#include "dialogue.h"
#include "quest_system.h"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace han::narrative {

/// Loads one chapter's three JSON documents and ties dialogue, quests and
/// state together. This is the entry point used by both the headless tests and
/// the future SFML client.
class NarrativeSession {
public:
    bool load(const std::filesystem::path& data_dir,
              const std::string& chapter_id = "ch01", std::string* error = nullptr);

    /// Loads a chapter and copies the state fields declared in its
    /// `carryover` block from the previous chapter's session.
    bool load(const std::filesystem::path& data_dir,
              const std::string& chapter_id,
              const NarrativeSession& previous,
              std::string* error = nullptr);

    /// Copies `carryover.variables` and `carryover.flags` from the previous
    /// chapter. Call before start().
    bool carry_from(const NarrativeSession& previous, std::string* error = nullptr);

    const std::string& chapter_id() const { return chapter_id_; }

    /// Chapter id recorded in the state document, or empty when this is the
    /// last chapter.
    std::string next_chapter() const;

    /// Chapter title from the state document, e.g. "第二章 白霜之约".
    std::string chapter_title() const;

    void start();

    NarrativeState& state() { return *state_; }
    const NarrativeState& state() const { return *state_; }
    QuestSystem& quests() { return *quests_; }
    const QuestSystem& quests() const { return *quests_; }
    DialogueDocument& document() { return *document_; }
    const DialogueDocument& document() const { return *document_; }

    /// First unplayed scene whose trigger currently evaluates to true.
    std::optional<std::string> pick_scene() const;
    bool play_scene(const std::string& scene_id, std::string* error = nullptr);
    bool play_next_scene(std::string* error = nullptr);

    bool scene_active() const { return !active_scene_.empty(); }
    const std::string& active_scene() const { return active_scene_; }
    /// Background id of the scene currently being played; empty when idle.
    std::string background() const { return runner_ ? runner_->background() : std::string{}; }
    const NodeView& current() const { return runner_->current(); }

    void advance();
    bool choose(const std::string& choice_id, std::string* error = nullptr);
    void resolve_combat(bool win);

    const std::vector<std::string>& played_scenes() const { return played_; }

private:
    void sync();
    void finish_if_needed();

    Json state_doc_;
    Json quest_doc_;
    Json dialogue_doc_;

    std::unique_ptr<NarrativeState> state_;
    std::unique_ptr<QuestSystem> quests_;
    std::unique_ptr<DialogueDocument> document_;
    std::unique_ptr<DialogueRunner> runner_;

    std::vector<std::string> played_;
    std::string active_scene_;
    std::string chapter_id_;
};

} // namespace han::narrative

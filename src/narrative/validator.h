#pragma once

#include "dialogue.h"
#include "narrative_state.h"
#include "quest_system.h"
#include "narrative_types.h"

#include <vector>

namespace han::narrative {

/// Structural validation for a chapter's three JSON documents.
///
/// Checks id uniqueness, every cross-reference (next / objective / reward /
/// clue / item / flag / variable / speaker / encounter) and the finite
/// condition/effect vocabulary. Intended to run in tests and in CI.
std::vector<ValidationIssue> validate_chapter(const Json& state_doc,
                                              const QuestSystem& quests,
                                              const DialogueDocument& dialogue,
                                              const NarrativeState& state);

} // namespace han::narrative

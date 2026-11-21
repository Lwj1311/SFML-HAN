#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace han::narrative {

using Json = nlohmann::json;

class NarrativeState;

/// Context used while evaluating condition expressions.
struct EvalContext {
    const NarrativeState* state = nullptr;
    /// Choice id selected in the current dialogue scene; empty when unavailable.
    std::string last_choice;
};

enum class NodeKind {
    Line,
    Choice,
    Combat,
    Register,
    Seal,
    End,
};

enum class QuestState {
    Inactive,
    Active,
    Completed,
};

struct LineView {
    std::string speaker_id;
    std::string speaker_name;
    std::string portrait;
    std::string text;
};

struct ChoiceOption {
    std::string id;
    std::string text;
};

/// Long register of names shown on the bamboo-slip scroll overlay.
struct RegisterView {
    std::string title;
    std::string subtitle;
    std::vector<std::string> names;
};

/// Ending card with a stamped seal impression.
struct SealView {
    std::string title;
    std::string text;
    std::string seal_id;
};

/// A snapshot of what the dialogue runner is currently presenting.
struct NodeView {
    NodeKind kind = NodeKind::End;
    std::string scene_id;
    std::string node_id;
    LineView line;
    std::string prompt;
    std::vector<ChoiceOption> choices;
    std::string encounter;
    RegisterView register_view;
    SealView seal_view;
    std::string exit;
};

struct ValidationIssue {
    enum class Severity {
        Error,
        Warning,
    };

    Severity severity = Severity::Error;
    std::string where;
    std::string message;
};

inline const char* to_string(QuestState state) {
    switch (state) {
    case QuestState::Inactive:
        return "inactive";
    case QuestState::Active:
        return "active";
    case QuestState::Completed:
        return "completed";
    }
    return "unknown";
}

} // namespace han::narrative

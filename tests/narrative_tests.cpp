#include "narrative/condition.h"
#include "narrative/dialogue.h"
#include "narrative/effects.h"
#include "narrative/narrative_state.h"
#include "narrative/quest_system.h"
#include "narrative/session.h"
#include "narrative/validator.h"
#include "test_framework.h"

#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <vector>

using namespace han::narrative;

namespace {

Json read_json(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw testing::Failure{"cannot open " + path.string()};
    }
    try {
        return Json::parse(stream);
    } catch (const std::exception& e) {
        throw testing::Failure{path.string() + ": " + e.what()};
    }
}

struct Fixture {
    Json state_doc;
    Json quest_doc;
    Json dialogue_doc;
    NarrativeState state;
    QuestSystem quests;
    DialogueDocument dialogue;

    void load() {
        const std::filesystem::path dir{HAN_DATA_DIR};
        state_doc = read_json(dir / "ch01.state.json");
        quest_doc = read_json(dir / "ch01.quests.json");
        dialogue_doc = read_json(dir / "ch01.dialogue.json");

        std::string error;
        HAN_CHECK(state.load_initial(state_doc, &error));
        if (!error.empty()) {
            std::cout << "state load note: " << error << '\n';
        }
        HAN_CHECK(quests.load(quest_doc, &error));
        quests.reset(state);
        HAN_CHECK(dialogue.load(dialogue_doc, &error));
    }
};

std::size_t count_errors(const std::vector<ValidationIssue>& issues) {
    std::size_t count = 0;
    for (const ValidationIssue& issue : issues) {
        if (issue.severity == ValidationIssue::Severity::Error) {
            ++count;
        }
    }
    return count;
}

void print_issues(const std::vector<ValidationIssue>& issues) {
    for (const ValidationIssue& issue : issues) {
        std::cout << (issue.severity == ValidationIssue::Severity::Error ? "ERROR "
                                                                         : "WARN  ")
                  << issue.where << ": " << issue.message << '\n';
    }
}

int quest_state(const NarrativeState& state, const std::string& id) {
    return static_cast<int>(state.quest_state(id));
}

/// Plays the active chapter to its end, always taking the first choice unless
/// one of `preferred` is offered.
void play_to_end(NarrativeSession& session,
                 const std::vector<std::string>& preferred = {}) {
    std::string error;
    int guard = 0;
    while (guard++ < 800) {
        if (!session.scene_active()) {
            if (!session.play_next_scene(&error)) {
                break;
            }
        }

        const NodeView& node = session.current();
        if (node.kind == NodeKind::Line || node.kind == NodeKind::Register ||
            node.kind == NodeKind::Seal) {
            session.advance();
        } else if (node.kind == NodeKind::Choice) {
            HAN_CHECK(!node.choices.empty());
            std::string pick = node.choices.front().id;
            for (const std::string& wanted : preferred) {
                for (const ChoiceOption& option : node.choices) {
                    if (option.id == wanted) {
                        pick = option.id;
                    }
                }
            }
            HAN_CHECK(session.choose(pick, &error));
        } else if (node.kind == NodeKind::Combat) {
            session.resolve_combat(true);
        } else {
            throw testing::Failure{"unexpected node kind while a scene is active"};
        }
    }
    HAN_CHECK(guard < 800);
}

void check_all_quests_completed(const NarrativeSession& session,
                                const std::vector<std::string>& ids) {
    for (const std::string& id : ids) {
        HAN_CHECK_EQ(quest_state(session.state(), id),
                     static_cast<int>(QuestState::Completed));
    }
}

} // namespace

HAN_TEST(chapter_data_loads_and_validates) {
    Fixture fixture;
    fixture.load();

    HAN_CHECK_EQ(fixture.dialogue.scene_count(), std::size_t{18});
    HAN_CHECK_EQ(fixture.dialogue.node_count(), std::size_t{157});
    HAN_CHECK_EQ(fixture.quests.size(), std::size_t{8});

    const std::vector<ValidationIssue> issues =
        validate_chapter(fixture.state_doc, fixture.quests, fixture.dialogue, fixture.state);
    print_issues(issues);
    HAN_CHECK_EQ(count_errors(issues), std::size_t{0});
}

HAN_TEST(condition_language_reads_state) {
    Fixture fixture;
    fixture.load();

    ConditionEvaluator conditions;
    const EvalContext context{&fixture.state, {}};

    HAN_CHECK(conditions.evaluate(Json{{"flag", "prologue_done"}}, context));
    HAN_CHECK(!conditions.evaluate(Json{{"flag", "ch1_done"}}, context));

    fixture.state.set_flag("ch1_letter_inspected", true);
    HAN_CHECK(conditions.evaluate(
        Json{{"all", Json::array({Json{{"flag", "prologue_done"}},
                                  Json{{"flag", "ch1_letter_inspected"}}})}},
        context));
    HAN_CHECK(conditions.evaluate(
        Json{{"any", Json::array({Json{{"flag", "ch1_done"}},
                                  Json{{"flag", "ch1_letter_inspected"}}})}},
        context));
    HAN_CHECK(conditions.evaluate(Json{{"not", Json{{"flag", "ch1_done"}}}}, context));

    fixture.state.set_int_var("num_su_trust", 3);
    HAN_CHECK(conditions.evaluate(
        Json{{"var", "num_su_trust"}, {"op", "gte"}, {"value", 3}}, context));
    HAN_CHECK(!conditions.evaluate(
        Json{{"var", "num_su_trust"}, {"op", "gt"}, {"value", 3}}, context));

    fixture.state.add_clue("clue_unsealed_letter");
    HAN_CHECK(conditions.evaluate(Json{{"clue", "clue_unsealed_letter"}}, context));

    fixture.state.give_item("item_qixin");
    HAN_CHECK(conditions.evaluate(Json{{"item", "item_qixin"}}, context));

    fixture.state.set_quest_state("mq_1_01", QuestState::Completed);
    HAN_CHECK(conditions.evaluate(
        Json{{"quest", "mq_1_01"}, {"state", "completed"}}, context));

    fixture.state.complete_objective("mq_1_01_obj_inspect");
    HAN_CHECK(conditions.evaluate(Json{{"objective", "mq_1_01_obj_inspect"}}, context));

    const EvalContext choice_context{&fixture.state, "know_dead"};
    HAN_CHECK(conditions.evaluate(Json{{"choice", "know_dead"}}, choice_context));
    HAN_CHECK(!conditions.evaluate(Json{{"choice", "remember"}}, choice_context));

    std::string error;
    HAN_CHECK(!conditions.evaluate(Json{{"bogus", "x"}}, context, &error));
    HAN_CHECK(!error.empty());
}

HAN_TEST(effects_write_state_and_clamp_variables) {
    Fixture fixture;
    fixture.load();

    EffectExecutor effects;
    std::string error;
    const Json commands = Json::array({
        Json{{"set_flag", "ch1_letter_inspected"}, {"value", true}},
        Json{{"add_var", "num_su_trust"}, {"value", 9}},
        Json{{"give_item", "item_post_pouch"}, {"count", 2}},
        Json{{"add_clue", "clue_receipt"}},
        Json{{"set_var", "receipt_choice"}, {"value", "concealed"}},
        Json{{"add_ability", "inspect_erasure"}},
    });
    HAN_CHECK(effects.apply(commands, fixture.state, {}, &error));

    HAN_CHECK(fixture.state.flag("ch1_letter_inspected"));
    HAN_CHECK_EQ(fixture.state.int_var("num_su_trust"), 5);
    HAN_CHECK_EQ(fixture.state.item_count("item_post_pouch"), 2);
    HAN_CHECK(fixture.state.has_clue("clue_receipt"));
    HAN_CHECK_EQ(fixture.state.string_var("receipt_choice"), std::string("concealed"));
    HAN_CHECK(fixture.state.has_ability("inspect_erasure"));

    const Json conditional = Json::array({
        Json{{"condition", Json{{"flag", "ch1_done"}}},
             {"add_clue", "clue_erasure_marks"}},
    });
    HAN_CHECK(effects.apply(conditional, fixture.state, {}, &error));
    HAN_CHECK(!fixture.state.has_clue("clue_erasure_marks"));
}

HAN_TEST(quests_start_complete_and_unlock) {
    Fixture fixture;
    fixture.load();

    fixture.quests.update(fixture.state);
    HAN_CHECK_EQ(quest_state(fixture.state, "mq_1_01"),
                 static_cast<int>(QuestState::Active));
    HAN_CHECK_EQ(quest_state(fixture.state, "mq_1_02"),
                 static_cast<int>(QuestState::Inactive));
    HAN_CHECK_EQ(quest_state(fixture.state, "sq_1_01"),
                 static_cast<int>(QuestState::Active));

    fixture.state.set_flag("ch1_letter_inspected", true);
    fixture.state.set_flag("ch1_hanbo_consulted", true);
    fixture.quests.update(fixture.state);

    HAN_CHECK_EQ(quest_state(fixture.state, "mq_1_01"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK(fixture.state.has_clue("clue_unsealed_letter"));
    HAN_CHECK_EQ(fixture.state.int_var("num_hanbo_trust"), 1);
    HAN_CHECK(fixture.state.objective_done("mq_1_01_obj_inspect"));
    HAN_CHECK_EQ(quest_state(fixture.state, "mq_1_02"),
                 static_cast<int>(QuestState::Active));
}

HAN_TEST(dialogue_scene_a_runs_to_the_end) {
    Fixture fixture;
    fixture.load();
    fixture.quests.update(fixture.state);
    fixture.state.set_flag("ch1_letter_inspected", true);
    fixture.state.set_flag("ch1_hanbo_consulted", true);

    DialogueRunner runner(fixture.dialogue, fixture.state);
    std::string error;
    HAN_CHECK(runner.start_scene("scene_a_night_letter", &error));
    HAN_CHECK_EQ(static_cast<int>(runner.current().kind),
                 static_cast<int>(NodeKind::Line));
    HAN_CHECK_EQ(runner.current().line.speaker_id, std::string("narrator"));

    runner.advance();
    HAN_CHECK_EQ(runner.current().line.speaker_id, std::string("zhangji"));

    runner.advance();
    HAN_CHECK_EQ(static_cast<int>(runner.current().kind),
                 static_cast<int>(NodeKind::Choice));
    HAN_CHECK(runner.choose("inspect", &error));
    HAN_CHECK(fixture.state.flag("ch1_letter_inspected"));
    HAN_CHECK(fixture.state.has_clue("clue_unsealed_letter"));

    runner.advance();
    HAN_CHECK_EQ(runner.current().line.speaker_id, std::string("hanbo"));
    runner.advance();
    HAN_CHECK_EQ(static_cast<int>(runner.current().kind),
                 static_cast<int>(NodeKind::Choice));
    HAN_CHECK(runner.choose("ask_names", &error));
    HAN_CHECK(fixture.state.flag("ch1_hanbo_consulted"));
    HAN_CHECK_EQ(fixture.state.int_var("num_hanbo_trust"), 1);

    runner.advance();
    HAN_CHECK_EQ(runner.current().line.text, std::string("天亮再说。"));
    runner.advance();
    HAN_CHECK(runner.finished());

    fixture.quests.update(fixture.state);
    HAN_CHECK_EQ(quest_state(fixture.state, "mq_1_01"),
                 static_cast<int>(QuestState::Completed));
}

HAN_TEST(dialogue_variants_follow_flags) {
    Fixture fixture;
    fixture.load();
    fixture.state.set_flag("sq1_01_done", true);

    DialogueRunner runner(fixture.dialogue, fixture.state);
    std::string error;
    HAN_CHECK(runner.start_scene("scene_a_night_letter", &error));
    runner.advance();
    runner.advance();
    HAN_CHECK(runner.choose("inspect", &error));
    runner.advance();
    runner.advance();
    HAN_CHECK(runner.choose("ask_names", &error));
    runner.advance();

    HAN_CHECK_EQ(runner.current().line.text, std::string("刀要快，嘴要慢。"));
}

HAN_TEST(dialogue_combat_grants_items) {
    Fixture fixture;
    fixture.load();

    DialogueRunner runner(fixture.dialogue, fixture.state);
    std::string error;
    HAN_CHECK(runner.start_scene("scene_b_patrol", &error));
    runner.advance();
    runner.advance();
    runner.advance();
    HAN_CHECK_EQ(static_cast<int>(runner.current().kind),
                 static_cast<int>(NodeKind::Choice));
    HAN_CHECK(runner.choose("follow", &error));

    runner.advance();
    HAN_CHECK_EQ(static_cast<int>(runner.current().kind),
                 static_cast<int>(NodeKind::Combat));
    HAN_CHECK_EQ(runner.current().encounter, std::string("enc_ch1_scouts"));

    runner.resolve_combat(true);
    HAN_CHECK(fixture.state.flag("ch1_scouts_cleared"));
    HAN_CHECK_EQ(fixture.state.item_count("item_post_pouch"), 1);
    HAN_CHECK(fixture.state.has_clue("clue_receipt_half"));

    runner.advance();
    runner.advance();
    HAN_CHECK(runner.finished());
}

HAN_TEST(session_plays_the_whole_chapter) {
    NarrativeSession session;
    std::string error;
    HAN_CHECK(session.load(std::filesystem::path{HAN_DATA_DIR}, "ch01", &error));
    session.start();

    int guard = 0;
    while (guard++ < 500) {
        if (!session.scene_active()) {
            if (!session.play_next_scene(&error)) {
                break;
            }
        }

        const NodeView& node = session.current();
        if (node.kind == NodeKind::Line) {
            session.advance();
        } else if (node.kind == NodeKind::Choice) {
            HAN_CHECK(!node.choices.empty());
            HAN_CHECK(session.choose(node.choices.front().id, &error));
        } else if (node.kind == NodeKind::Combat) {
            session.resolve_combat(true);
        } else {
            throw testing::Failure{"unexpected node kind while a scene is active"};
        }
    }

    HAN_CHECK(guard < 500);
    HAN_CHECK(session.state().flag("ch1_done"));
    HAN_CHECK_EQ(quest_state(session.state(), "mq_1_01"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK_EQ(quest_state(session.state(), "mq_1_02"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK_EQ(quest_state(session.state(), "mq_1_03"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK_EQ(quest_state(session.state(), "mq_1_04"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK_EQ(quest_state(session.state(), "mq_1_05"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK_EQ(quest_state(session.state(), "sq_1_02"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK_EQ(quest_state(session.state(), "sq_1_01"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK_EQ(quest_state(session.state(), "sq_1_04"),
                 static_cast<int>(QuestState::Completed));

    // Optional objectives and their conditional rewards must not be lost when
    // the main quest completes before the optional scene is reached.
    HAN_CHECK(session.state().objective_done("mq_1_01_obj_aqi"));
    HAN_CHECK(session.state().has_clue("clue_hu_order"));
    HAN_CHECK(session.state().has_clue("clue_erasure_marks"));
    HAN_CHECK(session.state().has_clue("clue_fast_horse"));
    HAN_CHECK(session.state().has_item("item_brush_knife"));
    HAN_CHECK(session.state().has_item("item_crossbow"));
    HAN_CHECK(session.state().has_item("item_bamboo_slip"));
    HAN_CHECK(session.state().flag("sq1_01_done"));
    HAN_CHECK(session.state().flag("sq1_04_done"));
    HAN_CHECK(session.state().int_var("num_aqi_trust") >= 2);

    HAN_CHECK_EQ(session.state().string_var("receipt_choice"), std::string("handed"));
    HAN_CHECK(session.state().has_item("item_qixin"));
    HAN_CHECK(session.state().has_item("item_post_pouch"));

    bool stable_scene_played = false;
    bool second_visit_played = false;
    for (const std::string& scene : session.played_scenes()) {
        stable_scene_played = stable_scene_played || scene == "scene_bj2_stable";
        second_visit_played = second_visit_played || scene == "scene_bj7_second_visit";
    }
    HAN_CHECK(stable_scene_played);
    HAN_CHECK(second_visit_played);
    HAN_CHECK(session.played_scenes().size() >= 16);

    // The real campaign path: finish ch01, then hand its state to ch02.
    NarrativeSession ch2;
    HAN_CHECK(ch2.load(std::filesystem::path{HAN_DATA_DIR}, "ch02", session, &error));
    HAN_CHECK_EQ(ch2.state().string_var("receipt_choice"), std::string("handed"));
    HAN_CHECK_EQ(ch2.state().int_var("num_hanbo_trust"),
                 session.state().int_var("num_hanbo_trust"));
}

HAN_TEST(chapter_two_data_loads_and_validates) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    const Json state_doc = read_json(dir / "ch02.state.json");
    const Json quest_doc = read_json(dir / "ch02.quests.json");
    const Json dialogue_doc = read_json(dir / "ch02.dialogue.json");

    NarrativeState state;
    QuestSystem quests;
    DialogueDocument dialogue;
    std::string error;
    HAN_CHECK(state.load_initial(state_doc, &error));
    HAN_CHECK(quests.load(quest_doc, &error));
    quests.reset(state);
    HAN_CHECK(dialogue.load(dialogue_doc, &error));

    HAN_CHECK_EQ(dialogue.scene_count(), std::size_t{9});
    HAN_CHECK_EQ(dialogue.node_count(), std::size_t{119});
    HAN_CHECK_EQ(quests.size(), std::size_t{8});

    const std::vector<ValidationIssue> issues =
        validate_chapter(state_doc, quests, dialogue, state);
    print_issues(issues);
    HAN_CHECK_EQ(count_errors(issues), std::size_t{0});
}

HAN_TEST(session_plays_the_second_chapter) {
    NarrativeSession session;
    std::string error;
    HAN_CHECK(session.load(std::filesystem::path{HAN_DATA_DIR}, "ch02", &error));
    session.start();

    int guard = 0;
    while (guard++ < 500) {
        if (!session.scene_active()) {
            if (!session.play_next_scene(&error)) {
                break;
            }
        }

        const NodeView& node = session.current();
        if (node.kind == NodeKind::Line) {
            session.advance();
        } else if (node.kind == NodeKind::Choice) {
            HAN_CHECK(!node.choices.empty());
            HAN_CHECK(session.choose(node.choices.front().id, &error));
        } else if (node.kind == NodeKind::Combat) {
            session.resolve_combat(true);
        } else {
            throw testing::Failure{"unexpected node kind while a scene is active"};
        }
    }

    HAN_CHECK(guard < 500);
    HAN_CHECK(session.state().flag("ch2_done"));
    HAN_CHECK_EQ(session.state().string_var("ch2_ending"), std::string("expose"));
    HAN_CHECK_EQ(quest_state(session.state(), "mq_2_01"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK_EQ(quest_state(session.state(), "mq_2_02"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK_EQ(quest_state(session.state(), "mq_2_03"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK_EQ(quest_state(session.state(), "mq_2_04"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK_EQ(quest_state(session.state(), "mq_2_05"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK_EQ(quest_state(session.state(), "sq_2_02"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK(session.state().has_clue("clue_zhougan_testimony"));
    HAN_CHECK(session.state().has_item("item_convoy_tally"));
    HAN_CHECK_EQ(session.played_scenes().size(), std::size_t{9});
}

HAN_TEST(chapter_two_copies_declared_carryover) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession ch1;
    std::string error;
    HAN_CHECK(ch1.load(dir, "ch01", &error));
    ch1.start();
    ch1.state().add_int_var("num_su_trust", 3);
    ch1.state().set_string_var("receipt_choice", "su");
    ch1.state().set_flag("ch1_guowang_hurt", true);
    ch1.state().set_flag("ch1_zhangji_found", true);

    NarrativeSession ch2;
    HAN_CHECK(ch2.load(dir, "ch02", ch1, &error));
    HAN_CHECK_EQ(ch2.chapter_id(), std::string("ch02"));
    HAN_CHECK_EQ(ch1.next_chapter(), std::string("ch02"));
    HAN_CHECK_EQ(ch2.next_chapter(), std::string("ch03"));
    HAN_CHECK_EQ(ch2.state().int_var("num_su_trust"), 3);
    HAN_CHECK_EQ(ch2.state().string_var("receipt_choice"), std::string("su"));
    HAN_CHECK(ch2.state().flag("ch1_guowang_hurt"));
    // Not listed in ch02's carryover block: must stay at its own default.
    HAN_CHECK(!ch2.state().flag("ch1_zhangji_found"));

    NarrativeSession mismatched;
    HAN_CHECK(!mismatched.load(dir, "ch02", ch2, &error));
    HAN_CHECK(!error.empty());
}

HAN_TEST(chapter_two_carryover_drives_opening) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession ch1;
    std::string error;
    HAN_CHECK(ch1.load(dir, "ch01", &error));
    ch1.start();
    ch1.state().set_string_var("receipt_choice", "su");

    NarrativeSession ch2;
    HAN_CHECK(ch2.load(dir, "ch02", ch1, &error));
    ch2.start();
    HAN_CHECK(ch2.play_next_scene(&error));
    ch2.advance();
    HAN_CHECK(ch2.scene_active());
    HAN_CHECK(ch2.current().line.text.find("苏娘灶膛") != std::string::npos);
}

HAN_TEST(chapter_three_data_loads_and_validates) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    const Json state_doc = read_json(dir / "ch03.state.json");
    const Json quest_doc = read_json(dir / "ch03.quests.json");
    const Json dialogue_doc = read_json(dir / "ch03.dialogue.json");

    NarrativeState state;
    QuestSystem quests;
    DialogueDocument dialogue;
    std::string error;
    HAN_CHECK(state.load_initial(state_doc, &error));
    HAN_CHECK(quests.load(quest_doc, &error));
    quests.reset(state);
    HAN_CHECK(dialogue.load(dialogue_doc, &error));

    HAN_CHECK_EQ(dialogue.scene_count(), std::size_t{9});
    HAN_CHECK_EQ(dialogue.node_count(), std::size_t{101});
    HAN_CHECK_EQ(quests.size(), std::size_t{8});

    const std::vector<ValidationIssue> issues =
        validate_chapter(state_doc, quests, dialogue, state);
    print_issues(issues);
    HAN_CHECK_EQ(count_errors(issues), std::size_t{0});
}

HAN_TEST(session_plays_the_third_chapter) {
    NarrativeSession session;
    std::string error;
    HAN_CHECK(session.load(std::filesystem::path{HAN_DATA_DIR}, "ch03", &error));
    session.start();

    int guard = 0;
    while (guard++ < 500) {
        if (!session.scene_active()) {
            if (!session.play_next_scene(&error)) {
                break;
            }
        }

        const NodeView& node = session.current();
        if (node.kind == NodeKind::Line) {
            session.advance();
        } else if (node.kind == NodeKind::Choice) {
            HAN_CHECK(!node.choices.empty());
            HAN_CHECK(session.choose(node.choices.front().id, &error));
        } else if (node.kind == NodeKind::Combat) {
            session.resolve_combat(true);
        } else {
            throw testing::Failure{"unexpected node kind while a scene is active"};
        }
    }

    HAN_CHECK(guard < 500);
    HAN_CHECK(session.state().flag("ch3_done"));
    HAN_CHECK_EQ(session.state().string_var("ch3_ending"), std::string("handover"));
    HAN_CHECK_EQ(quest_state(session.state(), "mq_3_01"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK_EQ(quest_state(session.state(), "mq_3_02"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK_EQ(quest_state(session.state(), "mq_3_03"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK_EQ(quest_state(session.state(), "mq_3_04"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK_EQ(quest_state(session.state(), "mq_3_05"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK(session.state().has_clue("clue_gate_manifest"));
    HAN_CHECK(session.state().has_item("item_gate_token"));
    // 独立开局时没有第二章的周敢/白姬/杜延状态，三场条件插场应跳过。
    HAN_CHECK_EQ(session.played_scenes().size(), std::size_t{6});
}

HAN_TEST(session_snapshot_round_trip) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession session;
    std::string error;
    HAN_CHECK(session.load(dir, "ch01", &error));
    session.start();

    // Finish the first scene so the snapshot is taken at a scene boundary,
    // matching how the client autosaves.
    HAN_CHECK(session.play_next_scene(&error));
    while (session.scene_active()) {
        const NodeView& node = session.current();
        if (node.kind == NodeKind::Line) {
            session.advance();
        } else if (node.kind == NodeKind::Choice) {
            HAN_CHECK(!node.choices.empty());
            HAN_CHECK(session.choose(node.choices.front().id, &error));
        } else if (node.kind == NodeKind::Combat) {
            session.resolve_combat(true);
        } else {
            throw testing::Failure{"unexpected node kind while a scene is active"};
        }
    }
    HAN_CHECK(!session.played_scenes().empty());

    const Json save = session.snapshot();
    HAN_CHECK_EQ(save.value("chapter_id", std::string{}), std::string("ch01"));

    NarrativeSession restored;
    HAN_CHECK(restored.load(dir, "ch01", &error));
    HAN_CHECK(restored.restore(save, &error));
    HAN_CHECK_EQ(restored.played_scenes().size(), session.played_scenes().size());
    HAN_CHECK_EQ(restored.state().int_var("num_hanbo_trust"),
                 session.state().int_var("num_hanbo_trust"));
    HAN_CHECK_EQ(restored.state().string_var("receipt_choice"),
                 session.state().string_var("receipt_choice"));
    HAN_CHECK_EQ(restored.state().clues().size(), session.state().clues().size());
    HAN_CHECK_EQ(restored.state().items().size(), session.state().items().size());
    HAN_CHECK_EQ(restored.state().completed_objectives().size(),
                 session.state().completed_objectives().size());
    HAN_CHECK_EQ(restored.state().quests().size(), session.state().quests().size());
    HAN_CHECK_EQ(quest_state(restored.state(), "mq_1_01"),
                 quest_state(session.state(), "mq_1_01"));

    // A restored session keeps advancing from the same boundary.
    HAN_CHECK(restored.play_next_scene(&error));
    HAN_CHECK(restored.scene_active());
    HAN_CHECK_EQ(restored.played_scenes().size(), session.played_scenes().size() + 1);

    // Finishing the chapter from a restored snapshot must not re-apply rewards.
    play_to_end(restored);
    HAN_CHECK(restored.state().flag("ch1_done"));
    HAN_CHECK_EQ(quest_state(restored.state(), "mq_1_01"),
                 static_cast<int>(QuestState::Completed));
}

HAN_TEST(chapter_one_receipts_all_reach_chapter_two) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    std::string error;

    for (const std::string& receipt : {"handed", "concealed", "su"}) {
        NarrativeSession ch1;
        HAN_CHECK(ch1.load(dir, "ch01", &error));
        ch1.start();
        play_to_end(ch1, {receipt});
        HAN_CHECK_EQ(ch1.state().string_var("receipt_choice"), receipt);
        HAN_CHECK(ch1.state().flag("ch1_done"));

        NarrativeSession ch2;
        HAN_CHECK(ch2.load(dir, "ch02", ch1, &error));
        ch2.start();
        play_to_end(ch2);
        HAN_CHECK(ch2.state().flag("ch2_done"));
        HAN_CHECK(ch2.state().string_var("ch2_ending") != "none");
    }
}

HAN_TEST(campaign_completes_every_quest_and_scene) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    std::string error;

    NarrativeSession ch1;
    HAN_CHECK(ch1.load(dir, "ch01", &error));
    ch1.start();
    play_to_end(ch1);
    check_all_quests_completed(ch1, {"mq_1_01", "mq_1_02", "mq_1_03", "mq_1_04",
                                     "mq_1_05", "sq_1_01", "sq_1_02", "sq_1_04"});
    HAN_CHECK(ch1.state().flag("ch1_done"));
    HAN_CHECK(ch1.state().string_var("receipt_choice") != "none");
    // 18 个场景中 scene_bj3_counter 是柜台分支的共享入口，正常流程共 17 场。
    HAN_CHECK_EQ(ch1.played_scenes().size(), std::size_t{17});

    NarrativeSession ch2;
    HAN_CHECK(ch2.load(dir, "ch02", ch1, &error));
    ch2.start();
    play_to_end(ch2, {"repair_slips"});
    check_all_quests_completed(ch2, {"mq_2_01", "mq_2_02", "mq_2_03", "mq_2_04",
                                     "mq_2_05", "sq_2_01", "sq_2_02", "sq_2_03"});
    HAN_CHECK(ch2.state().flag("ch2_done"));
    HAN_CHECK(ch2.state().string_var("ch2_ending") != "none");
    HAN_CHECK(ch2.state().has_clue("clue_ledger_collation"));
    HAN_CHECK_EQ(ch2.played_scenes().size(), std::size_t{9});

    NarrativeSession ch3;
    HAN_CHECK(ch3.load(dir, "ch03", ch2, &error));
    ch3.start();
    play_to_end(ch3, {"mend_roster"});
    check_all_quests_completed(ch3, {"mq_3_01", "mq_3_02", "mq_3_03", "mq_3_04",
                                     "mq_3_05", "sq_3_01", "sq_3_02", "sq_3_03"});
    HAN_CHECK(ch3.state().flag("ch3_done"));
    HAN_CHECK(ch3.state().string_var("ch3_ending") != "none");
    HAN_CHECK(ch3.state().has_clue("clue_erased_name"));
    HAN_CHECK_EQ(ch3.played_scenes().size(), std::size_t{9});

    NarrativeSession ch4;
    HAN_CHECK(ch4.load(dir, "ch04", ch3, &error));
    ch4.start();
    play_to_end(ch4);
    check_all_quests_completed(ch4, {"mq_4_01", "mq_4_02", "mq_4_03", "mq_4_04",
                                     "mq_4_05", "sq_4_01", "sq_4_02", "sq_4_03"});
    HAN_CHECK(ch4.state().flag("ch4_done"));
    HAN_CHECK_EQ(ch4.state().string_var("ch4_ending"), std::string("names"));
    HAN_CHECK(ch4.state().has_clue("clue_su_name"));
    HAN_CHECK_EQ(ch4.next_chapter(), std::string("ch05"));
    HAN_CHECK_EQ(ch4.played_scenes().size(), std::size_t{9});

    NarrativeSession ch5;
    HAN_CHECK(ch5.load(dir, "ch05", ch4, &error));
    ch5.start();
    play_to_end(ch5);
    check_all_quests_completed(ch5, {"mq_5_01"});
    HAN_CHECK(ch5.state().flag("ch5_done"));
    HAN_CHECK_EQ(ch5.state().string_var("ch4_ending"), std::string("names"));
    HAN_CHECK_EQ(ch5.played_scenes().size(), std::size_t{1});
    HAN_CHECK_EQ(ch5.played_scenes().back(), std::string("scene_5_names"));
    HAN_CHECK_EQ(ch5.next_chapter(), std::string("ch06"));

    NarrativeSession ch6;
    HAN_CHECK(ch6.load(dir, "ch06", ch5, &error));
    ch6.start();
    play_to_end(ch6);
    check_all_quests_completed(ch6, {"mq_6_01", "mq_6_02", "mq_6_03", "mq_6_04",
                                     "mq_6_05", "sq_6_01", "sq_6_02", "sq_6_03"});
    HAN_CHECK(ch6.state().flag("ch6_done"));
    HAN_CHECK_EQ(ch6.state().string_var("ch4_ending"), std::string("names"));
    HAN_CHECK_EQ(ch6.state().string_var("ch6_ending"), std::string("balance"));
    HAN_CHECK_EQ(ch6.played_scenes().size(), std::size_t{8});
    HAN_CHECK_EQ(ch6.played_scenes().back(), std::string("scene_6_end_balance"));
    HAN_CHECK_EQ(ch6.next_chapter(), std::string("ch07"));

    NarrativeSession ch7;
    HAN_CHECK(ch7.load(dir, "ch07", ch6, &error));
    ch7.start();
    play_to_end(ch7);
    check_all_quests_completed(ch7, {"mq_7_01", "mq_7_02", "mq_7_03", "mq_7_04",
                                     "mq_7_05", "sq_7_01", "sq_7_02", "sq_7_03"});
    HAN_CHECK(ch7.state().flag("ch7_done"));
    HAN_CHECK_EQ(ch7.state().string_var("ch6_ending"), std::string("balance"));
    HAN_CHECK_EQ(ch7.state().string_var("ch7_ending"), std::string("annex"));
    HAN_CHECK_EQ(ch7.played_scenes().size(), std::size_t{7});
    HAN_CHECK_EQ(ch7.played_scenes().back(), std::string("scene_7_end_annex"));
    HAN_CHECK_EQ(ch7.next_chapter(), std::string("ch08"));

    NarrativeSession ch8;
    HAN_CHECK(ch8.load(dir, "ch08", ch7, &error));
    ch8.start();
    play_to_end(ch8);
    check_all_quests_completed(ch8, {"mq_8_01", "mq_8_02", "mq_8_03", "mq_8_04",
                                     "mq_8_05", "sq_8_01", "sq_8_02", "sq_8_03"});
    HAN_CHECK(ch8.state().flag("ch8_done"));
    HAN_CHECK_EQ(ch8.state().string_var("ch7_ending"), std::string("annex"));
    HAN_CHECK_EQ(ch8.state().string_var("ch8_ending"), std::string("indict"));
    HAN_CHECK_EQ(ch8.state().int_var("num_horses"), 6);
    HAN_CHECK_EQ(ch8.played_scenes().size(), std::size_t{7});
    HAN_CHECK_EQ(ch8.played_scenes().back(), std::string("scene_8_end_indict"));
    HAN_CHECK_EQ(ch8.next_chapter(), std::string("ch09"));

    NarrativeSession ch9;
    HAN_CHECK(ch9.load(dir, "ch09", ch8, &error));
    ch9.start();
    play_to_end(ch9);
    check_all_quests_completed(ch9, {"mq_9_01", "mq_9_02", "mq_9_03", "mq_9_04",
                                     "mq_9_05", "sq_9_01", "sq_9_02", "sq_9_03"});
    HAN_CHECK(ch9.state().flag("ch9_done"));
    HAN_CHECK_EQ(ch9.state().string_var("ch8_ending"), std::string("indict"));
    HAN_CHECK_EQ(ch9.state().string_var("ch9_ending"), std::string("strict"));
    HAN_CHECK_EQ(ch9.state().int_var("num_coins"), 11);
    HAN_CHECK_EQ(ch9.played_scenes().size(), std::size_t{7});
    HAN_CHECK_EQ(ch9.played_scenes().back(), std::string("scene_9_end_strict"));
    HAN_CHECK_EQ(ch9.next_chapter(), std::string("ch10"));

    NarrativeSession ch10;
    HAN_CHECK(ch10.load(dir, "ch10", ch9, &error));
    ch10.start();
    play_to_end(ch10);
    check_all_quests_completed(ch10, {"mq_10_01", "mq_10_02", "mq_10_03", "mq_10_04",
                                      "mq_10_05", "sq_10_01", "sq_10_02", "sq_10_03"});
    HAN_CHECK(ch10.state().flag("ch10_done"));
    HAN_CHECK_EQ(ch10.state().string_var("ch9_ending"), std::string("strict"));
    HAN_CHECK_EQ(ch10.state().string_var("ch10_ending"), std::string("seal"));
    HAN_CHECK_EQ(ch10.state().int_var("num_letters"), 8);
    HAN_CHECK_EQ(ch10.played_scenes().size(), std::size_t{7});
    HAN_CHECK_EQ(ch10.played_scenes().back(), std::string("scene_10_end_seal"));
    HAN_CHECK(ch10.next_chapter().empty());
}

HAN_TEST(chapter_three_copies_declared_carryover) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession ch2;
    std::string error;
    HAN_CHECK(ch2.load(dir, "ch02", &error));
    ch2.start();
    ch2.state().set_string_var("ch2_ending", "deal");
    ch2.state().set_int_var("num_baiji_trust", 4);
    ch2.state().set_int_var("num_zhougan_trust", 3);
    ch2.state().set_flag("ch2_duyan_deal", true);
    ch2.state().set_flag("ch2_zhougan_protected", true);

    NarrativeSession ch3;
    HAN_CHECK(ch3.load(dir, "ch03", ch2, &error));
    HAN_CHECK_EQ(ch3.chapter_id(), std::string("ch03"));
    HAN_CHECK_EQ(ch3.next_chapter(), std::string("ch04"));
    HAN_CHECK_EQ(ch3.state().string_var("ch2_ending"), std::string("deal"));
    HAN_CHECK_EQ(ch3.state().int_var("num_baiji_trust"), 4);
    HAN_CHECK_EQ(ch3.state().int_var("num_zhougan_trust"), 3);
    HAN_CHECK(ch3.state().flag("ch2_duyan_deal"));
    HAN_CHECK(ch3.state().flag("ch2_zhougan_protected"));
    // 未列入携带块的第二章状态不应被带入。
    HAN_CHECK(!ch3.state().flag("ch2_done"));
}

HAN_TEST(chapter_three_carryover_drives_opening) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession ch2;
    std::string error;
    HAN_CHECK(ch2.load(dir, "ch02", &error));
    ch2.start();
    ch2.state().set_string_var("ch2_ending", "deal");
    ch2.state().set_int_var("num_baiji_trust", 3);
    ch2.state().set_flag("ch2_zhougan_protected", true);

    NarrativeSession ch3;
    HAN_CHECK(ch3.load(dir, "ch03", ch2, &error));
    ch3.start();
    HAN_CHECK(ch3.play_next_scene(&error));
    ch3.advance();
    HAN_CHECK(ch3.scene_active());
    HAN_CHECK(ch3.current().line.text.find("杜延的商队") != std::string::npos);
    for (int step = 0; step < 3 && ch3.scene_active(); ++step) {
        ch3.advance();
    }
    HAN_CHECK(ch3.state().has_item("item_qixin"));
    HAN_CHECK(ch3.state().has_item("item_zhougan_slip"));
    HAN_CHECK(ch3.state().has_clue("clue_baiji_letter"));
}

HAN_TEST(chapter_four_data_loads_and_validates) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    const Json state_doc = read_json(dir / "ch04.state.json");
    const Json quest_doc = read_json(dir / "ch04.quests.json");
    const Json dialogue_doc = read_json(dir / "ch04.dialogue.json");

    NarrativeState state;
    QuestSystem quests;
    DialogueDocument dialogue;
    std::string error;
    HAN_CHECK(state.load_initial(state_doc, &error));
    HAN_CHECK(quests.load(quest_doc, &error));
    quests.reset(state);
    HAN_CHECK(dialogue.load(dialogue_doc, &error));

    HAN_CHECK_EQ(dialogue.scene_count(), std::size_t{11});
    HAN_CHECK_EQ(dialogue.node_count(), std::size_t{139});
    HAN_CHECK_EQ(quests.size(), std::size_t{8});

    const std::vector<ValidationIssue> issues =
        validate_chapter(state_doc, quests, dialogue, state);
    print_issues(issues);
    HAN_CHECK_EQ(count_errors(issues), std::size_t{0});
}

HAN_TEST(session_plays_the_fourth_chapter) {
    NarrativeSession session;
    std::string error;
    HAN_CHECK(session.load(std::filesystem::path{HAN_DATA_DIR}, "ch04", &error));
    session.start();
    play_to_end(session);

    HAN_CHECK(session.state().flag("ch4_done"));
    HAN_CHECK_EQ(session.state().string_var("ch4_ending"), std::string("names"));
    HAN_CHECK(session.state().flag("ch4_names_restored"));
    HAN_CHECK(session.state().flag("ch4_archive_saved"));
    HAN_CHECK(session.state().int_var("num_evidence") >= 4);
    HAN_CHECK(session.state().has_clue("clue_register_slips"));
    HAN_CHECK(session.state().has_item("item_indictment"));
    check_all_quests_completed(session, {"mq_4_01", "mq_4_02", "mq_4_03", "mq_4_04",
                                         "mq_4_05", "sq_4_01", "sq_4_02", "sq_4_03"});
    HAN_CHECK_EQ(session.played_scenes().size(), std::size_t{9});
    HAN_CHECK_EQ(session.played_scenes().back(), std::string("scene_4_end_names"));
    HAN_CHECK_EQ(session.next_chapter(), std::string("ch05"));
}

HAN_TEST(chapter_four_supports_all_three_endings) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    struct EndingCase {
        const char* choice;
        const char* ending;
        const char* scene;
    };
    const EndingCase cases[] = {
        {"final_names", "names", "scene_4_end_names"},
        {"final_deal", "exile", "scene_4_end_exile"},
        {"final_road", "road", "scene_4_end_road"},
    };

    for (const EndingCase& test : cases) {
        NarrativeSession session;
        std::string error;
        HAN_CHECK(session.load(dir, "ch04", &error));
        session.start();
        play_to_end(session, {test.choice});

        HAN_CHECK(session.state().flag("ch4_done"));
        HAN_CHECK_EQ(session.state().string_var("ch4_ending"), std::string(test.ending));
        HAN_CHECK_EQ(session.played_scenes().back(), std::string(test.scene));
        HAN_CHECK_EQ(session.played_scenes().size(), std::size_t{9});
    }
}

HAN_TEST(chapter_four_copies_declared_carryover) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession ch3;
    std::string error;
    HAN_CHECK(ch3.load(dir, "ch03", &error));
    ch3.start();
    ch3.state().set_string_var("ch3_ending", "cargo");
    ch3.state().set_int_var("num_su_trust", 4);
    ch3.state().set_int_var("num_reputation", 9);
    ch3.state().set_int_var("num_zhaoqin_pressure", 2);
    ch3.state().set_flag("ch3_seal_seized", true);
    ch3.state().set_flag("ch3_zhougan_testified_2", true);
    ch3.state().set_flag("ch3_zhaoqin_flipped", true);
    ch3.state().set_flag("ch3_done", true);

    NarrativeSession ch4;
    HAN_CHECK(ch4.load(dir, "ch04", ch3, &error));
    HAN_CHECK_EQ(ch4.chapter_id(), std::string("ch04"));
    HAN_CHECK_EQ(ch4.next_chapter(), std::string("ch05"));
    HAN_CHECK_EQ(ch4.state().string_var("ch3_ending"), std::string("cargo"));
    HAN_CHECK_EQ(ch4.state().int_var("num_su_trust"), 4);
    HAN_CHECK_EQ(ch4.state().int_var("num_reputation"), 9);
    HAN_CHECK_EQ(ch4.state().int_var("num_zhaoqin_pressure"), 2);
    HAN_CHECK(ch4.state().flag("ch3_seal_seized"));
    HAN_CHECK(ch4.state().flag("ch3_zhougan_testified_2"));
    HAN_CHECK(ch4.state().flag("ch3_zhaoqin_flipped"));
    HAN_CHECK(!ch4.state().flag("ch3_done"));

    NarrativeSession ch2;
    HAN_CHECK(ch2.load(dir, "ch02", &error));
    NarrativeSession mismatched;
    HAN_CHECK(!mismatched.load(dir, "ch04", ch2, &error));
    HAN_CHECK(!error.empty());
}

HAN_TEST(chapter_four_carryover_drives_opening) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession ch3;
    std::string error;
    HAN_CHECK(ch3.load(dir, "ch03", &error));
    ch3.start();
    ch3.state().set_string_var("ch3_ending", "stand");
    ch3.state().set_int_var("num_zhaoqin_pressure", 1);
    ch3.state().set_flag("ch3_seal_seized", true);
    ch3.state().set_flag("ch3_zhougan_stayed", true);

    NarrativeSession ch4;
    HAN_CHECK(ch4.load(dir, "ch04", ch3, &error));
    ch4.start();
    HAN_CHECK(ch4.play_next_scene(&error));
    ch4.advance();
    HAN_CHECK(ch4.scene_active());
    HAN_CHECK(ch4.current().line.text.find("关下那一案") != std::string::npos);

    int guard = 0;
    while (ch4.scene_active() && guard++ < 40) {
        ch4.advance();
    }
    HAN_CHECK(guard < 40);
    HAN_CHECK(ch4.state().flag("ch4_open_done"));
    HAN_CHECK_EQ(ch4.state().int_var("num_zhaoqin_pressure"), 2);
    HAN_CHECK_EQ(ch4.state().int_var("num_evidence"), 1);
    HAN_CHECK(ch4.state().has_item("item_bamboo_slip"));
    HAN_CHECK(ch4.state().has_item("item_brush_knife"));
    HAN_CHECK(ch4.state().has_item("item_seal_clay"));
    HAN_CHECK(ch4.state().has_item("item_zhougan_slip"));
}

HAN_TEST(chapter_four_names_ending_has_complete_register_and_seal) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeState state;
    DialogueDocument dialogue;
    std::string error;
    HAN_CHECK(state.load_initial(read_json(dir / "ch04.state.json"), &error));
    HAN_CHECK(dialogue.load(read_json(dir / "ch04.dialogue.json"), &error));

    DialogueRunner runner(dialogue, state);
    HAN_CHECK(runner.start_scene("scene_4_end_names", &error));
    runner.advance();
    HAN_CHECK_EQ(static_cast<int>(runner.current().kind),
                 static_cast<int>(NodeKind::Register));
    HAN_CHECK_EQ(runner.current().register_view.title, std::string("名籍"));
    HAN_CHECK_EQ(runner.current().register_view.subtitle,
                 std::string("悬泉置 · 三十七人"));
    const std::vector<std::string>& names = runner.current().register_view.names;
    HAN_CHECK_EQ(names.size(), std::size_t{37});
    HAN_CHECK_EQ(std::set<std::string>(names.begin(), names.end()).size(),
                 std::size_t{37});
    HAN_CHECK_EQ(names.front(), std::string("张季"));
    HAN_CHECK_EQ(names.back(), std::string("郭买"));

    runner.advance();
    runner.advance();
    runner.advance();
    HAN_CHECK_EQ(static_cast<int>(runner.current().kind),
                 static_cast<int>(NodeKind::Seal));
    HAN_CHECK_EQ(runner.current().seal_view.title, std::string("结局 · 名籍"));
    HAN_CHECK_EQ(runner.current().seal_view.seal_id, std::string("ui_ending_seal"));
    HAN_CHECK_EQ(runner.current().seal_view.text,
                 std::string("三十七个名字，重新写回名籍。"));

    runner.advance();
    HAN_CHECK(state.flag("ch4_done"));
    runner.advance();
    HAN_CHECK_EQ(static_cast<int>(runner.current().kind),
                 static_cast<int>(NodeKind::End));
}

HAN_TEST(chapter_five_data_loads_and_validates) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    const Json state_doc = read_json(dir / "ch05.state.json");
    const Json quest_doc = read_json(dir / "ch05.quests.json");
    const Json dialogue_doc = read_json(dir / "ch05.dialogue.json");

    NarrativeState state;
    QuestSystem quests;
    DialogueDocument dialogue;
    std::string error;
    HAN_CHECK(state.load_initial(state_doc, &error));
    HAN_CHECK(quests.load(quest_doc, &error));
    quests.reset(state);
    HAN_CHECK(dialogue.load(dialogue_doc, &error));

    HAN_CHECK_EQ(dialogue.scene_count(), std::size_t{4});
    HAN_CHECK_EQ(dialogue.node_count(), std::size_t{40});
    HAN_CHECK_EQ(quests.size(), std::size_t{1});

    const std::vector<ValidationIssue> issues =
        validate_chapter(state_doc, quests, dialogue, state);
    print_issues(issues);
    HAN_CHECK_EQ(count_errors(issues), std::size_t{0});
}

HAN_TEST(chapter_four_endings_continue_into_epilogue) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    struct EpilogueCase {
        const char* ending;
        const char* scene;
    };
    const EpilogueCase cases[] = {
        {"names", "scene_5_names"},
        {"exile", "scene_5_exile"},
        {"road", "scene_5_road"},
    };

    for (const EpilogueCase& test : cases) {
        NarrativeSession ch4;
        std::string error;
        HAN_CHECK(ch4.load(dir, "ch04", &error));
        ch4.start();
        ch4.state().set_string_var("ch4_ending", test.ending);
        ch4.state().set_flag("ch4_done", true);

        NarrativeSession ch5;
        HAN_CHECK(ch5.load(dir, "ch05", ch4, &error));
        HAN_CHECK_EQ(ch5.chapter_id(), std::string("ch05"));
        HAN_CHECK_EQ(ch5.state().string_var("ch4_ending"), std::string(test.ending));
        ch5.start();
        play_to_end(ch5);

        HAN_CHECK(ch5.state().flag("ch5_done"));
        HAN_CHECK_EQ(quest_state(ch5.state(), "mq_5_01"),
                     static_cast<int>(QuestState::Completed));
        HAN_CHECK_EQ(ch5.played_scenes().size(), std::size_t{1});
        HAN_CHECK_EQ(ch5.played_scenes().back(), std::string(test.scene));
        HAN_CHECK_EQ(ch5.next_chapter(), std::string("ch06"));
    }
}

HAN_TEST(chapter_five_standalone_has_a_fallback) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession session;
    std::string error;
    HAN_CHECK(session.load(dir, "ch05", &error));
    session.start();
    play_to_end(session);

    HAN_CHECK_EQ(session.state().string_var("ch4_ending"), std::string("none"));
    HAN_CHECK(session.state().flag("ch5_done"));
    HAN_CHECK_EQ(quest_state(session.state(), "mq_5_01"),
                 static_cast<int>(QuestState::Completed));
    HAN_CHECK_EQ(session.played_scenes().size(), std::size_t{1});
    HAN_CHECK_EQ(session.played_scenes().back(), std::string("scene_5_standalone"));
    HAN_CHECK_EQ(session.next_chapter(), std::string("ch06"));

    NarrativeSession ch3;
    HAN_CHECK(ch3.load(dir, "ch03", &error));
    NarrativeSession mismatched;
    HAN_CHECK(!mismatched.load(dir, "ch05", ch3, &error));
    HAN_CHECK(!error.empty());
}

HAN_TEST(chapter_six_data_loads_and_validates) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    const Json state_doc = read_json(dir / "ch06.state.json");
    const Json quest_doc = read_json(dir / "ch06.quests.json");
    const Json dialogue_doc = read_json(dir / "ch06.dialogue.json");

    NarrativeState state;
    QuestSystem quests;
    DialogueDocument dialogue;
    std::string error;
    HAN_CHECK(state.load_initial(state_doc, &error));
    HAN_CHECK(quests.load(quest_doc, &error));
    quests.reset(state);
    HAN_CHECK(dialogue.load(dialogue_doc, &error));

    HAN_CHECK_EQ(dialogue.scene_count(), std::size_t{10});
    HAN_CHECK_EQ(dialogue.node_count(), std::size_t{97});
    HAN_CHECK_EQ(quests.size(), std::size_t{8});

    const std::vector<ValidationIssue> issues =
        validate_chapter(state_doc, quests, dialogue, state);
    print_issues(issues);
    HAN_CHECK_EQ(count_errors(issues), std::size_t{0});
}

HAN_TEST(chapter_six_standalone_plays_to_an_ending) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession session;
    std::string error;
    HAN_CHECK(session.load(dir, "ch06", &error));
    session.start();
    play_to_end(session);

    HAN_CHECK_EQ(session.state().string_var("ch4_ending"), std::string("none"));
    HAN_CHECK_EQ(session.state().string_var("ch6_ending"), std::string("balance"));
    HAN_CHECK(session.state().flag("ch6_done"));
    HAN_CHECK_EQ(session.state().int_var("num_proof"), 6);
    HAN_CHECK_EQ(session.played_scenes().size(), std::size_t{8});
    HAN_CHECK_EQ(session.played_scenes().front(), std::string("scene_6_open"));
    HAN_CHECK_EQ(session.played_scenes().back(), std::string("scene_6_end_balance"));
    HAN_CHECK_EQ(session.next_chapter(), std::string("ch07"));
}

HAN_TEST(chapter_seven_data_loads_and_validates) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    const Json state_doc = read_json(dir / "ch07.state.json");
    const Json quest_doc = read_json(dir / "ch07.quests.json");
    const Json dialogue_doc = read_json(dir / "ch07.dialogue.json");

    NarrativeState state;
    QuestSystem quests;
    DialogueDocument dialogue;
    std::string error;
    HAN_CHECK(state.load_initial(state_doc, &error));
    HAN_CHECK(quests.load(quest_doc, &error));
    quests.reset(state);
    HAN_CHECK(dialogue.load(dialogue_doc, &error));

    HAN_CHECK_EQ(dialogue.scene_count(), std::size_t{9});
    HAN_CHECK_EQ(dialogue.node_count(), std::size_t{88});
    HAN_CHECK_EQ(quests.size(), std::size_t{8});

    const std::vector<ValidationIssue> issues =
        validate_chapter(state_doc, quests, dialogue, state);
    print_issues(issues);
    HAN_CHECK_EQ(count_errors(issues), std::size_t{0});
}

HAN_TEST(chapter_seven_standalone_plays_to_an_ending) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession session;
    std::string error;
    HAN_CHECK(session.load(dir, "ch07", &error));
    session.start();
    play_to_end(session);

    HAN_CHECK_EQ(session.state().string_var("ch6_ending"), std::string("none"));
    HAN_CHECK_EQ(session.state().string_var("ch7_ending"), std::string("annex"));
    HAN_CHECK(session.state().flag("ch7_done"));
    HAN_CHECK_EQ(session.state().int_var("num_kinship"), 5);
    HAN_CHECK_EQ(session.played_scenes().size(), std::size_t{7});
    HAN_CHECK_EQ(session.played_scenes().front(), std::string("scene_7_open"));
    HAN_CHECK_EQ(session.played_scenes().back(), std::string("scene_7_end_annex"));
    HAN_CHECK_EQ(session.next_chapter(), std::string("ch08"));
}

HAN_TEST(chapter_seven_supports_three_endings) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    struct EndingCase {
        const char* choice;
        const char* ending;
        const char* scene;
    };
    const EndingCase cases[] = {
        {"choose_annex", "annex", "scene_7_end_annex"},
        {"choose_fine", "fine", "scene_7_end_fine"},
        {"choose_leave", "leave", "scene_7_end_leave"},
    };

    for (const EndingCase& test : cases) {
        NarrativeSession session;
        std::string error;
        HAN_CHECK(session.load(dir, "ch07", &error));
        session.start();
        play_to_end(session, {test.choice});

        HAN_CHECK(session.state().flag("ch7_done"));
        HAN_CHECK_EQ(session.state().string_var("ch7_ending"), std::string(test.ending));
        HAN_CHECK_EQ(session.played_scenes().size(), std::size_t{7});
        HAN_CHECK_EQ(session.played_scenes().back(), std::string(test.scene));
        HAN_CHECK_EQ(session.next_chapter(), std::string("ch08"));
    }
}

HAN_TEST(chapter_seven_copies_carryover_from_chapter_six) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession ch6;
    std::string error;
    HAN_CHECK(ch6.load(dir, "ch06", &error));
    ch6.start();
    ch6.state().set_string_var("ch6_ending", "expose");

    NarrativeSession ch7;
    HAN_CHECK(ch7.load(dir, "ch07", ch6, &error));
    HAN_CHECK_EQ(ch7.chapter_id(), std::string("ch07"));
    HAN_CHECK_EQ(ch7.state().string_var("ch6_ending"), std::string("expose"));
    HAN_CHECK_EQ(ch7.state().string_var("ch7_ending"), std::string("none"));
    HAN_CHECK_EQ(ch7.next_chapter(), std::string("ch08"));

    NarrativeSession ch5;
    HAN_CHECK(ch5.load(dir, "ch05", &error));
    NarrativeSession mismatched;
    HAN_CHECK(!mismatched.load(dir, "ch07", ch5, &error));
    HAN_CHECK(!error.empty());
}

HAN_TEST(chapter_eight_data_loads_and_validates) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    const Json state_doc = read_json(dir / "ch08.state.json");
    const Json quest_doc = read_json(dir / "ch08.quests.json");
    const Json dialogue_doc = read_json(dir / "ch08.dialogue.json");

    NarrativeState state;
    QuestSystem quests;
    DialogueDocument dialogue;
    std::string error;
    HAN_CHECK(state.load_initial(state_doc, &error));
    HAN_CHECK(quests.load(quest_doc, &error));
    quests.reset(state);
    HAN_CHECK(dialogue.load(dialogue_doc, &error));

    HAN_CHECK_EQ(dialogue.scene_count(), std::size_t{9});
    HAN_CHECK_EQ(dialogue.node_count(), std::size_t{92});
    HAN_CHECK_EQ(quests.size(), std::size_t{8});

    const std::vector<ValidationIssue> issues =
        validate_chapter(state_doc, quests, dialogue, state);
    print_issues(issues);
    HAN_CHECK_EQ(count_errors(issues), std::size_t{0});
}

HAN_TEST(chapter_eight_standalone_plays_to_an_ending) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession session;
    std::string error;
    HAN_CHECK(session.load(dir, "ch08", &error));
    session.start();
    play_to_end(session);

    HAN_CHECK_EQ(session.state().string_var("ch7_ending"), std::string("none"));
    HAN_CHECK_EQ(session.state().string_var("ch8_ending"), std::string("indict"));
    HAN_CHECK(session.state().flag("ch8_done"));
    HAN_CHECK_EQ(session.state().int_var("num_horses"), 6);
    HAN_CHECK_EQ(session.state().int_var("num_ledger"), 5);
    HAN_CHECK_EQ(session.played_scenes().size(), std::size_t{7});
    HAN_CHECK_EQ(session.played_scenes().front(), std::string("scene_8_open"));
    HAN_CHECK_EQ(session.played_scenes().back(), std::string("scene_8_end_indict"));
    HAN_CHECK_EQ(session.next_chapter(), std::string("ch09"));
}

HAN_TEST(chapter_eight_supports_three_endings) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    struct EndingCase {
        const char* choice;
        const char* ending;
        const char* scene;
    };
    const EndingCase cases[] = {
        {"choose_indict", "indict", "scene_8_end_indict"},
        {"choose_settle", "settle", "scene_8_end_settle"},
        {"choose_pursue", "pursue", "scene_8_end_pursue"},
    };

    for (const EndingCase& test : cases) {
        NarrativeSession session;
        std::string error;
        HAN_CHECK(session.load(dir, "ch08", &error));
        session.start();
        play_to_end(session, {test.choice});

        HAN_CHECK(session.state().flag("ch8_done"));
        HAN_CHECK_EQ(session.state().string_var("ch8_ending"), std::string(test.ending));
        HAN_CHECK_EQ(session.played_scenes().size(), std::size_t{7});
        HAN_CHECK_EQ(session.played_scenes().back(), std::string(test.scene));
        HAN_CHECK_EQ(session.next_chapter(), std::string("ch09"));
    }
}

HAN_TEST(chapter_eight_copies_carryover_from_chapter_seven) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession ch7;
    std::string error;
    HAN_CHECK(ch7.load(dir, "ch07", &error));
    ch7.start();
    ch7.state().set_string_var("ch7_ending", "leave");

    NarrativeSession ch8;
    HAN_CHECK(ch8.load(dir, "ch08", ch7, &error));
    HAN_CHECK_EQ(ch8.chapter_id(), std::string("ch08"));
    HAN_CHECK_EQ(ch8.state().string_var("ch7_ending"), std::string("leave"));
    HAN_CHECK_EQ(ch8.state().string_var("ch8_ending"), std::string("none"));
    HAN_CHECK_EQ(ch8.next_chapter(), std::string("ch09"));

    NarrativeSession ch6;
    HAN_CHECK(ch6.load(dir, "ch06", &error));
    NarrativeSession mismatched;
    HAN_CHECK(!mismatched.load(dir, "ch08", ch6, &error));
    HAN_CHECK(!error.empty());
}

HAN_TEST(chapter_nine_data_loads_and_validates) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    const Json state_doc = read_json(dir / "ch09.state.json");
    const Json quest_doc = read_json(dir / "ch09.quests.json");
    const Json dialogue_doc = read_json(dir / "ch09.dialogue.json");

    NarrativeState state;
    QuestSystem quests;
    DialogueDocument dialogue;
    std::string error;
    HAN_CHECK(state.load_initial(state_doc, &error));
    HAN_CHECK(quests.load(quest_doc, &error));
    quests.reset(state);
    HAN_CHECK(dialogue.load(dialogue_doc, &error));

    HAN_CHECK_EQ(dialogue.scene_count(), std::size_t{9});
    HAN_CHECK_EQ(dialogue.node_count(), std::size_t{92});
    HAN_CHECK_EQ(quests.size(), std::size_t{8});

    const std::vector<ValidationIssue> issues =
        validate_chapter(state_doc, quests, dialogue, state);
    print_issues(issues);
    HAN_CHECK_EQ(count_errors(issues), std::size_t{0});
}

HAN_TEST(chapter_nine_standalone_plays_to_an_ending) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession session;
    std::string error;
    HAN_CHECK(session.load(dir, "ch09", &error));
    session.start();
    play_to_end(session);

    HAN_CHECK_EQ(session.state().string_var("ch8_ending"), std::string("none"));
    HAN_CHECK_EQ(session.state().string_var("ch9_ending"), std::string("strict"));
    HAN_CHECK(session.state().flag("ch9_done"));
    HAN_CHECK_EQ(session.state().int_var("num_coins"), 11);
    HAN_CHECK_EQ(session.state().int_var("num_forgery"), 5);
    HAN_CHECK_EQ(session.played_scenes().size(), std::size_t{7});
    HAN_CHECK_EQ(session.played_scenes().front(), std::string("scene_9_open"));
    HAN_CHECK_EQ(session.played_scenes().back(), std::string("scene_9_end_strict"));
    HAN_CHECK_EQ(session.next_chapter(), std::string("ch10"));
}

HAN_TEST(chapter_nine_supports_three_endings) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    struct EndingCase {
        const char* choice;
        const char* ending;
        const char* scene;
    };
    const EndingCase cases[] = {
        {"choose_strict", "strict", "scene_9_end_strict"},
        {"choose_soft", "soft", "scene_9_end_soft"},
        {"choose_trace", "trace", "scene_9_end_trace"},
    };

    for (const EndingCase& test : cases) {
        NarrativeSession session;
        std::string error;
        HAN_CHECK(session.load(dir, "ch09", &error));
        session.start();
        play_to_end(session, {test.choice});

        HAN_CHECK(session.state().flag("ch9_done"));
        HAN_CHECK_EQ(session.state().string_var("ch9_ending"), std::string(test.ending));
        HAN_CHECK_EQ(session.played_scenes().size(), std::size_t{7});
        HAN_CHECK_EQ(session.played_scenes().back(), std::string(test.scene));
        HAN_CHECK_EQ(session.next_chapter(), std::string("ch10"));
    }
}

HAN_TEST(chapter_nine_copies_carryover_from_chapter_eight) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession ch8;
    std::string error;
    HAN_CHECK(ch8.load(dir, "ch08", &error));
    ch8.start();
    ch8.state().set_string_var("ch8_ending", "pursue");

    NarrativeSession ch9;
    HAN_CHECK(ch9.load(dir, "ch09", ch8, &error));
    HAN_CHECK_EQ(ch9.chapter_id(), std::string("ch09"));
    HAN_CHECK_EQ(ch9.state().string_var("ch8_ending"), std::string("pursue"));
    HAN_CHECK_EQ(ch9.state().string_var("ch9_ending"), std::string("none"));
    HAN_CHECK_EQ(ch9.next_chapter(), std::string("ch10"));

    NarrativeSession ch7;
    HAN_CHECK(ch7.load(dir, "ch07", &error));
    NarrativeSession mismatched;
    HAN_CHECK(!mismatched.load(dir, "ch09", ch7, &error));
    HAN_CHECK(!error.empty());
}

HAN_TEST(chapter_ten_data_loads_and_validates) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    const Json state_doc = read_json(dir / "ch10.state.json");
    const Json quest_doc = read_json(dir / "ch10.quests.json");
    const Json dialogue_doc = read_json(dir / "ch10.dialogue.json");

    NarrativeState state;
    QuestSystem quests;
    DialogueDocument dialogue;
    std::string error;
    HAN_CHECK(state.load_initial(state_doc, &error));
    HAN_CHECK(quests.load(quest_doc, &error));
    quests.reset(state);
    HAN_CHECK(dialogue.load(dialogue_doc, &error));

    HAN_CHECK_EQ(dialogue.scene_count(), std::size_t{9});
    HAN_CHECK_EQ(dialogue.node_count(), std::size_t{94});
    HAN_CHECK_EQ(quests.size(), std::size_t{8});

    const std::vector<ValidationIssue> issues =
        validate_chapter(state_doc, quests, dialogue, state);
    print_issues(issues);
    HAN_CHECK_EQ(count_errors(issues), std::size_t{0});
}

HAN_TEST(chapter_ten_standalone_plays_to_an_ending) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession session;
    std::string error;
    HAN_CHECK(session.load(dir, "ch10", &error));
    session.start();
    play_to_end(session);

    HAN_CHECK_EQ(session.state().string_var("ch9_ending"), std::string("none"));
    HAN_CHECK_EQ(session.state().string_var("ch10_ending"), std::string("seal"));
    HAN_CHECK(session.state().flag("ch10_done"));
    HAN_CHECK_EQ(session.state().int_var("num_letters"), 8);
    HAN_CHECK_EQ(session.state().int_var("num_seals"), 4);
    HAN_CHECK_EQ(session.played_scenes().size(), std::size_t{7});
    HAN_CHECK_EQ(session.played_scenes().front(), std::string("scene_10_open"));
    HAN_CHECK_EQ(session.played_scenes().back(), std::string("scene_10_end_seal"));
    HAN_CHECK(session.next_chapter().empty());
}

HAN_TEST(chapter_ten_supports_three_endings) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    struct EndingCase {
        const char* choice;
        const char* ending;
        const char* scene;
    };
    const EndingCase cases[] = {
        {"choose_seal", "seal", "scene_10_end_seal"},
        {"choose_cover", "cover", "scene_10_end_cover"},
        {"choose_chase", "chase", "scene_10_end_chase"},
    };

    for (const EndingCase& test : cases) {
        NarrativeSession session;
        std::string error;
        HAN_CHECK(session.load(dir, "ch10", &error));
        session.start();
        play_to_end(session, {test.choice});

        HAN_CHECK(session.state().flag("ch10_done"));
        HAN_CHECK_EQ(session.state().string_var("ch10_ending"), std::string(test.ending));
        HAN_CHECK_EQ(session.played_scenes().size(), std::size_t{7});
        HAN_CHECK_EQ(session.played_scenes().back(), std::string(test.scene));
        HAN_CHECK(session.next_chapter().empty());
    }
}

HAN_TEST(chapter_ten_copies_carryover_from_chapter_nine) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession ch9;
    std::string error;
    HAN_CHECK(ch9.load(dir, "ch09", &error));
    ch9.start();
    ch9.state().set_string_var("ch9_ending", "trace");

    NarrativeSession ch10;
    HAN_CHECK(ch10.load(dir, "ch10", ch9, &error));
    HAN_CHECK_EQ(ch10.chapter_id(), std::string("ch10"));
    HAN_CHECK_EQ(ch10.state().string_var("ch9_ending"), std::string("trace"));
    HAN_CHECK_EQ(ch10.state().string_var("ch10_ending"), std::string("none"));
    HAN_CHECK(ch10.next_chapter().empty());

    NarrativeSession ch8;
    HAN_CHECK(ch8.load(dir, "ch08", &error));
    NarrativeSession mismatched;
    HAN_CHECK(!mismatched.load(dir, "ch10", ch8, &error));
    HAN_CHECK(!error.empty());
}

HAN_TEST(chapter_six_supports_three_endings) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    struct EndingCase {
        const char* choice;
        const char* ending;
        const char* scene;
    };
    const EndingCase cases[] = {
        {"choose_balance", "balance", "scene_6_end_balance"},
        {"choose_expose", "expose", "scene_6_end_expose"},
        {"choose_quiet", "quiet", "scene_6_end_quiet"},
    };

    for (const EndingCase& test : cases) {
        NarrativeSession session;
        std::string error;
        HAN_CHECK(session.load(dir, "ch06", &error));
        session.start();
        play_to_end(session, {test.choice});

        HAN_CHECK(session.state().flag("ch6_done"));
        HAN_CHECK_EQ(session.state().string_var("ch6_ending"), std::string(test.ending));
        HAN_CHECK_EQ(session.played_scenes().size(), std::size_t{8});
        HAN_CHECK_EQ(session.played_scenes().back(), std::string(test.scene));
        HAN_CHECK_EQ(session.next_chapter(), std::string("ch07"));
    }
}

HAN_TEST(chapter_six_copies_carryover_from_chapter_five) {
    const std::filesystem::path dir{HAN_DATA_DIR};
    NarrativeSession ch4;
    std::string error;
    HAN_CHECK(ch4.load(dir, "ch04", &error));
    ch4.start();
    ch4.state().set_string_var("ch4_ending", "road");

    NarrativeSession ch5;
    HAN_CHECK(ch5.load(dir, "ch05", ch4, &error));
    HAN_CHECK_EQ(ch5.state().string_var("ch4_ending"), std::string("road"));

    NarrativeSession ch6;
    HAN_CHECK(ch6.load(dir, "ch06", ch5, &error));
    HAN_CHECK_EQ(ch6.chapter_id(), std::string("ch06"));
    HAN_CHECK_EQ(ch6.state().string_var("ch4_ending"), std::string("road"));
    HAN_CHECK_EQ(ch6.state().string_var("ch6_ending"), std::string("none"));
    HAN_CHECK_EQ(ch6.next_chapter(), std::string("ch07"));

    NarrativeSession mismatched;
    HAN_CHECK(!mismatched.load(dir, "ch06", ch4, &error));
    HAN_CHECK(!error.empty());
}

HAN_TEST(register_and_seal_nodes_parse_and_advance) {
    Fixture fixture;
    fixture.load();

    const Json doc{
        {"schema_version", 1},
        {"speakers", Json::object()},
        {"encounters", Json::object()},
        {"scenes", Json::array({Json{
             {"id", "preview"},
             {"title", "预览"},
             {"background", "bg_station_interior"},
             {"entry", "r1"},
             {"nodes", Json::array({
                  Json{{"id", "r1"},
                       {"type", "register"},
                       {"title", "名籍"},
                       {"subtitle", "三十七人"},
                       {"names", Json::array({"张季", "周敢", "白姬"})},
                       {"next", "s1"}},
                  Json{{"id", "s1"},
                       {"type", "seal"},
                       {"title", "结局·自守"},
                       {"text", "三十七个名字，重新写回名籍。"},
                       {"next", "done"}},
                  Json{{"id", "done"}, {"type", "end"}},
              })},
         }})},
    };

    DialogueDocument preview;
    std::string error;
    HAN_CHECK(preview.load(doc, &error));

    const std::vector<ValidationIssue> issues =
        validate_chapter(fixture.state_doc, fixture.quests, preview, fixture.state);
    print_issues(issues);
    HAN_CHECK_EQ(count_errors(issues), std::size_t{0});

    DialogueRunner runner(preview, fixture.state);
    HAN_CHECK(runner.start_scene("preview", &error));
    HAN_CHECK_EQ(static_cast<int>(runner.current().kind),
                 static_cast<int>(NodeKind::Register));
    HAN_CHECK_EQ(runner.current().register_view.title, std::string("名籍"));
    HAN_CHECK_EQ(runner.current().register_view.names.size(), std::size_t{3});
    HAN_CHECK_EQ(runner.current().register_view.names.at(1), std::string("周敢"));

    runner.advance();
    HAN_CHECK_EQ(static_cast<int>(runner.current().kind),
                 static_cast<int>(NodeKind::Seal));
    HAN_CHECK_EQ(runner.current().seal_view.seal_id, std::string("ui_ending_seal"));
    HAN_CHECK_EQ(runner.current().seal_view.text,
                 std::string("三十七个名字，重新写回名籍。"));

    runner.advance();
    HAN_CHECK_EQ(static_cast<int>(runner.current().kind),
                 static_cast<int>(NodeKind::End));
}

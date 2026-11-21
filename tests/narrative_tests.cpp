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
                 static_cast<int>(QuestState::Active));
    HAN_CHECK_EQ(quest_state(session.state(), "sq_1_04"),
                 static_cast<int>(QuestState::Active));

    HAN_CHECK_EQ(session.state().string_var("receipt_choice"), std::string("handed"));
    HAN_CHECK(session.state().has_item("item_qixin"));
    HAN_CHECK(session.state().has_item("item_post_pouch"));
    HAN_CHECK(session.played_scenes().size() >= 14);

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

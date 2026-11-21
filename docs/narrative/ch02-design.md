# 第二章《白霜之约》章节设计

> 状态：已落地为 `ch02.state.json` / `ch02.quests.json` / `ch02.dialogue.json`，共 9 场景、119 节点、5 主线 + 3 支线任务；客户端可用 `--chapter ch02` 直接进入。
>
> 承接第一章《无印之信》。本章把"一封假信"推进为"一条用死人名字运转的军资私运链"，
> 并把第一章的三个证据处置结果真正变成开局压力。

## 1. 设计目标

- **主题**：名字与活人。第一章问"周敢死没死"，第二章问"谁在替死人领粮、谁在替活人销名"。
- **时间窗**：冬至前十日 → 冬至前夜，约十天，用"霜"作为倒计时意象。
- **玩家体验**：调查（督邮查账）→ 追索（障塞活口、车马店账目）→ 抉择（截车、交易、放线）。
- **节奏**：1 场必打战斗（押车私兵）+ 1 场非战斗对峙（白姬）+ 1 个多分支章末。
- **不锁死**：战斗失败统一为"重伤被郭望拖回"，主线继续；任何结局都进入第三章。

## 2. 直接承接第一章

第一章 `receipt_choice` 的三种结果在本章开局分成三个短变体，随后汇流：

| 第一章处置 | 第二章开局状态 | 本章压力 |
| --- | --- | --- |
| `handed` 上报郡府 | 证据已交出，冯安手上只剩记忆 | 郡府督邮带账册来"复核"，实则定调 |
| `concealed` 压下私查 | 证据在箱底，郡府开始注意冯安 | 督邮以"失职"为由逼交，韩伯被问话 |
| `su` 交给苏娘 | 证据在灶膛，白姬的人来问过灰 | 车马店先动手，苏娘线成为软肋 |

其他承接：`num_su_trust`、`num_baiji_suspicion`、`num_hanbo_trust`、`num_guowang_trust`、`num_aqi_trust`、`num_prefecture_suspicion`、`num_reputation`、`ch1_duyan_alerted`、`ch1_guowang_hurt`、`evidence_with_su`。

## 3. 剧情梗概（三幕）

**第一幕 · 督邮到驿**
郡府督邮赵钦携廪食簿副本抵达悬泉置，名义是"核冬至粮"，实际要赶在冬至前把账做平。他并不知情全部内情，但为了考课，愿意压下任何"不该有的名字"。韩伯的私简抄本成了烫手之物：交出去，韩伯担罪；不交，冯安担罪。

**第二幕 · 活人与账**
阿耆从商队口音与马印辨出：那封无印信的语序不是匈奴语，而是"常年跑关外的人"故意学的腔。线索指向障塞废墟里的一个活人——周敢。他没死，伤重，藏在废弃烽燧；他的名字被用来领三十七人的粮，又被用来收十具弩机。另一边，白姬的车马店账目对不上：后门钥匙牌、深夜快马、被烧的"冬至"二字，都指向她替人中转。她既是被拿住把柄的人，也是唯一知道车队几时过关的人。

**第三幕 · 冬至前夜**
车队在广至驿装最后一批货，准备趁冬至关禁松懈出玉门关。冯安必须在"截货抓人""放长线""做交易"之间选一条。三条路分别通向第三章的三种开局：被通缉、混入商队、或被郡府追索。

## 4. 关键角色

| 角色 | 定位 | 本章作用 | 备注 |
| --- | --- | --- | --- |
| 周敢 | 活口/证人 | 证明名籍被改、弩机私运 | 重伤；是否活到第三章由玩家抉择决定 |
| 赵钦（新） | 郡府督邮 | 制度性压力；不是主谋，但要政绩 | 可合作、可反制；第三章揭示其上报对象 |
| 白姬 | 车马店掌柜 | 中转节点；可争取、可利用 | 与"白狐"关系本章只给暗示，不揭底 |
| 杜延 | 河西豪侠 | 提供"私了"方案，想控制商路 | 接受则第三章有商队通行便利，代价是欠人情 |
| 韩伯 | 老吏 | 私简抄本与名籍证据 | 信任不足时拒绝交出抄本 |
| 苏娘 | 遗孀 | 证据保管人/情感锚点 | `su` 开局时是本章最脆弱的一环 |
| 郭望 | 戍卒 | 战斗协助与障塞带路 | `ch1_guowang_hurt` 为真时本章开头带伤 |
| 阿耆 | 译人 | 语言/马印/商队线 | 信任低时只给一半情报 |

## 5. 场景结构（9 场）

| # | 场景 id | 标题 | 背景 | 触发 | 核心内容 |
| --- | --- | --- | --- | --- | --- |
| 1 | `scene_2_open` | 冬至前十日 | `bg_station_interior` | `ch2_start` | 按 `receipt_choice` 三变体开局，汇流至督邮入驿 |
| 2 | `scene_2_audit` | 督邮到驿 | `bg_station_courtyard` | `ch2_open_done` | 查账对峙；抄本交/藏/引向广至驿 |
| 3 | `scene_2_zhougan` | 障塞里的活人 | `bg_beacon_tower` | `ch2_audit_done` | 找到周敢；听供词；决定护/交 |
| 4 | `scene_2_baiji` | 柜台后的账 | `bg_guangzhi_post` | `ch2_zhougan_done` | 白姬对峙；钥匙牌/快马/账目；谈条件 |
| 5 | `scene_2_su` | 灶膛里的灰 | `bg_station_interior` | `ch2_baiji_done` 且证据在苏娘处或已压箱底 | 证据是否还在；苏娘安危；白姬的人来过 |
| 6 | `scene_2_duyan` | 豪侠的价码 | `bg_station_courtyard` | `ch2_duyan_offer` | 杜延提出"替你压下"，选择合作/拒绝/反将 |
| 7 | `scene_2_convoy` | 白霜夜行 | `bg_gobi_postroad` | `ch2_convoy_ready` | 跟踪车队；战斗；截货/放线/换货 |
| 8 | `scene_2_climax` | 冬至前夜 | `bg_guangzhi_post` | `ch2_convoy_done` | 广至驿最终处置；周敢/白姬/赵钦同时在场 |
| 9 | `scene_2_end` | 霜落 | `bg_yumenguan_dusk` | `ch2_climax_done` | 三结局收束，设置 `ch2_ending` |

场景 5 与 6 为可选支线插场，未触发时主线从 4 直接到 7；场景 9 一定执行。

## 6. 主线任务（5 条）

### mq_2_01 督邮的账
- 目标：`mq_2_01_obj_receive`（接待赵钦，flag `ch2_zhaoqin_met`）、`mq_2_01_obj_ledger`（核对廪食簿，flag `ch2_ledger_checked`）、`mq_2_01_obj_copy`（决定抄本去向，flag `ch2_copy_decided`）
- 完成：前两项 + 第三项
- 奖励：`clue_erasure_marks` 复述、`num_prefecture_suspicion` ±1（按选择）、`num_hanbo_trust` ±1
- 分支：`hand_copy`（交抄本，韩伯信任 -2）、`hide_copy`（藏，韩伯信任 +1）、`redirect`（引向广至驿，郡府怀疑 +1）

### mq_2_02 障塞里的活人
- 前置：`mq_2_01` completed
- 目标：`mq_2_02_obj_find`（找到周敢，flag `ch2_zhougan_found`）、`mq_2_02_obj_testify`（听完供词，flag `ch2_zhougan_testified`）、`mq_2_02_obj_protect`（决定去留，flag `ch2_zhougan_decided`）
- 奖励：`clue_zhougan_testimony`、`item_zhougan_slip`、`num_zhougan_trust` +1（保护时 +2）
- 分支：`protect`（藏回障塞）、`handover`（交督邮，周敢线断）、`escape`（放走，第三章再遇）

### mq_2_03 柜台后的账
- 前置：`mq_2_02` completed
- 目标：`mq_2_03_obj_key`（核钥匙牌，flag `ch2_key_checked`）、`mq_2_03_obj_ledger`（查车马店账，flag `ch2_shop_ledger_seen`）、`mq_2_03_obj_baiji`（与白姬摊牌，flag `ch2_baiji_confronted`）
- 奖励：`clue_convoy_manifest`、`clue_baiji_letter`、`num_baiji_suspicion` +1（强硬）/ `num_baiji_trust` +1（交易）
- 分支：`deal`（放她一马换车队时辰）、`press`（逼问，怀疑 +2）、`leave`（不摊牌，情报少一半）

### mq_2_04 白霜夜行
- 前置：`mq_2_03` completed
- 目标：`mq_2_04_obj_track`（追踪车队，flag `ch2_convoy_tracked`）、`mq_2_04_obj_fight`（击退押车私兵，flag `ch2_convoy_cleared`）、`mq_2_04_obj_cargo`（查验货箱，flag `ch2_cargo_checked`）
- 奖励：`clue_convoy_manifest`、`item_convoy_tally`、`num_reputation` +1
- 战斗：`enc_ch2_convoy`（私兵 3 + 弓手 1），失败 → 重伤回驿，`ch2_convoy_cleared` 不置位但仍推进

### mq_2_05 冬至前夜
- 前置：`mq_2_04` completed
- 目标：`mq_2_05_obj_confront`（最终处置，flag `ch2_climax_done`）、`mq_2_05_obj_choose`（`ch2_ending` != `none`）
- 完成奖励：flag `ch2_done`，解锁 `ch03`
- 分支见第 8 节

## 7. 支线任务（3 条）

| id | 标题 | 前置 | 目标 | 奖励 | 作用 |
| --- | --- | --- | --- | --- | --- |
| `sq_2_01` | 断简 | 持有 `item_brush_knife` 且 `ch2_audit_done` | 修复韩伯抄本 | `clue_ledger_collation`、`num_hanbo_trust` +1 | 提高第三章翻案筹码（复用第一章 `clue_hanbo_copy` 为底本） |
| `sq_2_02` | 白狐的信 | `ch2_baiji_confronted` | 决定送/拆/烧白姬托付的信 | `num_baiji_trust` 或 `num_baiji_suspicion` | 决定白姬第三章立场 |
| `sq_2_03` | 归义印 | `num_aqi_trust` ≥ 2 且 `ch2_baiji_done` | 为羌商验印 | `clue_seal_owner`、`item_seal` 相关线索、`num_aqi_trust` +1 | 打开第三章商队通行线 |

## 8. 章末分支与结局

`scene_2_climax` 的最终选择结合状态决定结局，写入 `ch2_ending`：

| 结局 | 条件（选择 + 状态） | 结果 | 第三章开局 |
| --- | --- | --- | --- |
| `expose` 明面结案 | 选"交给督邮"且 `num_prefecture_suspicion` ≤ 1 | 车队被查，但上层提前得信；周敢失踪 | 冯安被调往玉门关"协查"，实为支开 |
| `deal` 暗线交易 | 选"与白姬/杜延做交易"且任一人信任 ≥ 3 | 货被私吞，周敢活，白姬欠情 | 混入商队出关，第三章商队线 |
| `hold` 压案自守 | 选"扣下证据，放车走"或周敢被保护 | 证据链完整但冯安成靶子 | 郡府追索，第三章逃亡+翻案线 |

任一结局：`clue_convoy_manifest` 至少获得一份；`ch2_done` 置真。章节链接不放 state 文件（当前 schema 无此字段），由外层章节流程/存档层记录"下一章 = ch03"。

## 9. 新增状态

### flags（已按实现落地）
`ch2_intro`、`ch2_open_done`、`ch2_copy_hidden`、`ch2_zhaoqin_met`、`ch2_ledger_checked`、`ch2_copy_decided`、`ch2_audit_done`、`ch2_zhougan_found`、`ch2_zhougan_testified`、`ch2_zhougan_decided`、`ch2_zhougan_protected`、`ch2_zhougan_done`、`ch2_key_checked`、`ch2_shop_ledger_seen`、`ch2_baiji_confronted`、`ch2_baiji_done`、`ch2_duyan_deal`、`ch2_convoy_tracked`、`ch2_convoy_fought`、`ch2_cargo_checked`、`ch2_plan_seize`、`ch2_plan_trail`、`ch2_plan_swap`、`ch2_convoy_done`、`ch2_climax_done`、`ch2_done`、`sq2_01_done`、`sq2_02_done`、`sq2_03_done`；跨章只读取 `ch1_guowang_hurt`、`ch1_duyan_alerted`、`evidence_with_su`。

### variables
| id | 范围 | 说明 |
| --- | --- | --- |
| `num_zhougan_trust` | 0–5 | 周敢配合度，影响第三章证词 |
| `num_baiji_trust` | 0–5 | 白姬倾向，影响交易与第三章放行 |
| `num_zhaoqin_pressure` | 0–3 | 督邮施压程度，影响查账与结局 |
| `ch2_ending` | enum `none/expose/deal/hold` | 章末结局，第三章分支键 |

### clues
`clue_zhougan_testimony`（周敢口供）、`clue_ledger_collation`（韩伯抄本与廪食簿比对结果）、`clue_convoy_manifest`（车队货单）、`clue_seal_owner`（废止官署印的持有人）、`clue_baiji_letter`（白姬托付的信）、`clue_zhaoqin_ledger`（督邮账册里的空白名字）。

### items
`item_zhougan_slip`（周敢供词简，图标可复用 `item_bamboo_slip`）、`item_convoy_tally`（车队筹牌，图标待补）。

### encounters
`enc_ch2_convoy`：私兵 ×3、弓手 ×1，地形 `gobi`；若 `ch1_guowang_hurt` 为真，郭望本章不参战，战斗难度提示 +1。

## 10. 技术契约与资源

- **章节切换**：现有 `NarrativeSession` 只加载单章。建议在 `ch02.state.json` 增加：
  ```json
  "carryover": {
    "from": "ch01",
    "variables": ["num_su_trust", "num_baiji_suspicion", "num_hanbo_trust",
                  "num_guowang_trust", "num_aqi_trust", "num_prefecture_suspicion",
                  "num_reputation", "receipt_choice"],
    "flags": ["ch1_duyan_alerted", "ch1_guowang_hurt", "evidence_with_su"]
  }
  ```
  已实现：`NarrativeSession::load(data_dir, chapter_id, previous, &err)` / `carry_from()` 会读取该块并复制字段；`ch01.state.json` 用 `next_chapter` 指向 `ch02`；客户端 `--campaign` 在章节结束后自动携状态进入下一章。无上一章时 ch02 仍以默认值独立跑通（`receipt_choice = none` 走通用开局）。
- **节点 id**：全章前缀 `ch2_`，节点 id 章内唯一（沿用第一章规则：节点图章内共享）。
- **场景触发**：全部使用 `flag` 触发器，按 `ch2_*_done` 链式推进，保证 `play_next_scene` 可自动串场。
- **美术缺口**：已补齐。`bg_guangzhi_post`、`bg_red_willow_beach` 背景图已生成入库；周敢（`char_zhougan`）、赵钦（`char_zhaoqin`）立绘已生成；`item_post_pouch`、`item_convoy_tally` 图标已生成并回填 state。第一、二章所有被剧情引用的背景与立绘、物品图标均无缺失。
- **文本量**：实际 9 场景、119 节点、5 主线 + 3 支线，与第一章（18 场景/157 节点）相比更紧凑，已能闭环。

## 11. 后续章节大纲

- **第三章《过关》**：冯安携证据/周敢/商队三种开局之一前往玉门关。核心是"棨信"的使用与关吏查验；揭示废止官署印属于郡府某曹史，赵钦只是被推到台前的人。结尾逼玩家在"交人"与"交货"之间选。
- **第四章《三十七个名字》**：冬天与清算。三十七人的名字要被重新写回名籍，代价是有人必须认罪。以悬泉置/烽燧防守或郡府对质收束全篇，按 `num_reputation`、`num_su_trust`、周敢生死、证据完整度给出多结局评价。

## 12. 默认创作取舍（可直接按此实现）

1. 周敢默认可救活：玩家选择保护且 `num_zhougan_trust ≥ 3` 时活到第三章；否则第三章只留供词简。
2. 白姬不洗白也不彻底反派：本章给"被胁迫 + 牟利"的双重动机，真相留到第三章。
3. 赵钦不是主谋：他只是想压案考课，真正的主使在郡府曹署，第三章再落锤。
4. 杜延的"帮忙"一定有代价：接受交易会在第三章以"人情债"形式回收。

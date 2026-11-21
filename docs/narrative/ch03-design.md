# 第三章《过关》章节设计

> 状态：已落地。`ch03.state.json` / `ch03.quests.json` / `ch03.dialogue.json` 已实现，
> 9 场景 / 101 节点 / 8 任务（5 主线 + 3 支线），测试覆盖校验、自动通关与 ch02→ch03 携带。
>
> 承接第二章《白霜之约》。第二章把"一封假信"推进为"一条用死人名字运转的军资私运链"，
> 本章要回答下一个问题：**这条链子是谁批准过关的**。答案不在赵钦身上，而在郡府的印与关上。

## 1. 设计目标

- **主题**：印与关。谁有资格开门、放人、销名；一张真印如何替一件假事作保。
- **时间窗**：冬至前一日傍晚 → 冬至日关禁，约一日一夜，用"关"作为倒计时意象（关一闭，人货俱留）。
- **玩家体验**：混关（文书与人）→ 追印（棨信副券、仓曹名录）→ 关下抉择（交人 / 交货 / 自守）。
- **节奏**：1 场必打战斗（关下夺印或拦车）+ 1 场非战斗对峙（关令 + 赵钦）+ 1 个三向章末。
- **不锁死**：第二章三种结局在第二幕中段汇流；战斗失败统一为"重伤被郭望拖回关内"，主线继续；任一结局进入第四章。

## 2. 直接承接第二章

`ch2_ending` 的三种结果在本章开局分成三个短变体，第二幕汇流：

| 第二章结局 | 第三章开局状态 | 本章压力 |
| --- | --- | --- |
| `expose` 明面结案 | 冯安以"协查"名义被调往玉门关，随行有郡府文书 | 名义上是公差，实则被支开；关吏对你的来意早有耳闻 |
| `deal` 暗线交易 | 冯安混在杜延/白姬的商队里出关 | 没有官身，只有一张借来的棨信；商队里有白姬的人 |
| `hold` 压案自守 | 冯安带着完整证据链独自赴关，郡府已发追索 | 关下盘查最严；赵钦会在关前等你 |

其他承接变量：`num_su_trust`、`num_baiji_suspicion`、`num_hanbo_trust`、`num_guowang_trust`、`num_aqi_trust`、`num_prefecture_suspicion`、`num_reputation`、`num_zhougan_trust`、`num_baiji_trust`、`num_zhaoqin_pressure`；标志：`ch2_zhougan_protected`、`ch2_duyan_deal`、`ch2_plan_seize`、`ch2_plan_trail`、`ch2_plan_swap`、`evidence_with_su`、`ch1_guowang_hurt`、`ch1_duyan_alerted`。

## 3. 剧情梗概（三幕）

**第一幕 · 关前**
玉门关冬至日闭门三日。冯安必须在关闭前把"人、货、证"三件事中的两件带过去。关令成公朔只认文书不认人：棨信、传、验封，一样不能少。冯安手里的棨信是张季的——第二章已经证明，张季的棨信在红柳滩之后仍被人用过。谁在用它？副券在关吏的存根里，这是唯一能倒查的实物。

**第二幕 · 印与人**
关内驿舍里，赵钦先到了。他不再谈考课，只谈"上头有人要这车货过去"。顺着副券与残印，冯安查到：那枚废止官署的印，是郡府仓曹史卫敞私刻的"旧印"，专为过关文书作保；赵钦只是被推到台前签字的人。周敢若活着，他的证词能让"死人领粮"与"死人过关"对上；白姬若还欠着情，她能换来车队放行的时辰；杜延的"人情债"在这一章到期。

**第三幕 · 关下**
货队在关前被拦。成公朔要一个说法：要么交人（周敢/白姬），要么交货（三十七人的粮与弩机），要么把关下的事闹到郡府——第三条路最险，也是唯一能同时保住人和证的。三条路分别通向第四章的三种开局：被押回郡府、带证出关、或关下自守待援。

## 4. 关键角色

| 角色 | 定位 | 本章作用 | 立绘 |
| --- | --- | --- | --- |
| 成公朔（新） | 玉门关关令 | 只认文书；可被证据说服，也可被上命压住 | `char_chenggong` |
| 卫敞（新） | 郡府仓曹史 | 私刻旧印、为过关文书作保；本章真正的对手 | `char_weichang` |
| 赵钦 | 郡府督邮 | 台前签字人；可反制、可合作 | `char_zhaoqin` |
| 周敢 | 活口/证人 | 活着则当关作证；只留供词简则证据链缺一环 | `char_zhougan` |
| 白姬 | 车马店掌柜 | 兑现第二章的交易，或临阵反水 | `char_baiji` |
| 杜延 | 河西豪侠 | 第二章的人情债到期；可借商队通行 | `char_duyan` |
| 韩伯 | 老吏 | 抄本比对结果决定翻案筹码 | `char_hanbo` |
| 苏娘 | 遗孀 | 证据保管人；仅书信出场，不回驿 | `char_suhe` |
| 郭望 | 戍卒 | 关下战斗主力；`ch1_guowang_hurt` 为真时带伤 | `char_guowang` |
| 阿耆 | 译人 | 商队伪装与关外路线 | `char_aqi` |

## 5. 场景结构（9 场）

| # | 场景 id | 标题 | 背景 | 触发 | 核心内容 |
| --- | --- | --- | --- | --- | --- |
| 1 | `scene_3_open` | 关前一日 | `bg_gobi_postroad`（按 `ch2_ending` 切换开场文本） | `ch3_start` | 按 `ch2_ending` 三变体开局，汇流至关前 |
| 2 | `scene_3_customs` | 关门查验 | `bg_yumenguan_dusk` | `ch3_open_done` | 棨信、传、验封三验；决定申报口径 |
| 3 | `scene_3_stub` | 副券 | `bg_yumenguan_dusk` | `ch3_customs_done` | 查棨信存根副券，发现张季之后仍有人用过 |
| 4 | `scene_3_roster` | 仓曹的名录 | `bg_station_interior` | `ch3_stub_done` | 残印比对、仓曹名录、赵钦与卫敞的关系 |
| 5 | `scene_3_zhougan` | 当关作证 | `bg_beacon_tower` | `ch3_roster_done` 且周敢存活 | 周敢是否出面；作证与保护二选一 |
| 6 | `scene_3_baiji` | 白狐的价码 | `bg_guangzhi_post` | `ch3_roster_done` 且 `ch2_baiji_trust ≥ 2` | 兑现交易或反水；换放行时辰 |
| 7 | `scene_3_duyan` | 到期的人情 | `bg_oasis` | `ch3_roster_done` 且 `ch2_duyan_deal` | 杜延索取代价；可拒绝并反制 |
| 8 | `scene_3_gate` | 关下 | `bg_yumenguan_gate` | `ch3_party_ready` | 拦车对峙 + 战斗；夺印 / 拦货 / 放行 |
| 9 | `scene_3_end` | 过关 | `bg_qilian_mountain` | `ch3_gate_done` | 交人 / 交货 / 自守，写 `ch3_ending` |

场景 5–7 为可选插场，未满足条件时主线从 4 直接到 8；场景 9 一定执行。
所有场景使用 `flag` 触发器，按 `ch3_*_done` 链式推进，保证 `play_next_scene` 可自动串场。

## 6. 主线任务（5 条）

### mq_3_01 过关文书
- 目标：`mq_3_01_obj_qixin`（验棨信，flag `ch3_qixin_checked`）、`mq_3_01_obj_manifest`（申报货物，flag `ch3_manifest_declared`）、`mq_3_01_obj_party`（带同伴过关，flag `ch3_party_passed`）
- 完成：三项全成
- 奖励：`clue_gate_registry`、`num_reputation` +1
- 分支：`declare_truth`（照实申报，关吏起疑 +1）、`conceal`（按"农具"申报，`clue_convoy_manifest` 复用 + 风险）

### mq_3_02 棨信的来路
- 前置：`mq_3_01` completed
- 目标：`mq_3_02_obj_stub`（查副券，flag `ch3_stub_checked`）、`mq_3_02_obj_trace`（问阿耆/白姬，flag `ch3_qixin_traced`）、`mq_3_02_obj_forge`（确认冒名，flag `ch3_qixin_forgery`）
- 奖励：`clue_qixin_stub`、`item_qixin`（若第一章未持有）
- 分支：`press_clerk`（硬问关吏，`num_prefecture_suspicion` +1）、`buy_stub`（用钱/人情换存根抄录）

### mq_3_03 仓曹的印
- 前置：`mq_3_02` completed
- 目标：`mq_3_03_obj_seal`（残印比对，flag `ch3_seal_compared`）、`mq_3_03_obj_roster`（取得仓曹名录，flag `ch3_roster_got`）、`mq_3_03_obj_confront`（与卫敞/赵钦对质，flag `ch3_weichang_confronted`）
- 奖励：`clue_caocao_roster`、`clue_seal_owner`（第二章已有则合并为 `clue_seal_owner_full`）、`num_zhaoqin_pressure` +1
- 分支：`threaten_zhaoqin`（拿账册反制，赵钦可转合作）、`deal_weichang`（接卫敞的条件，后续翻案变难）

### mq_3_04 关下
- 前置：`mq_3_03` completed
- 目标：`mq_3_04_obj_track`（锁定货队位置，flag `ch3_gate_tracked`）、`mq_3_04_obj_fight`（击退押货私兵，flag `ch3_gate_cleared`）、`mq_3_04_obj_seize`（夺取文书/印，flag `ch3_seal_seized`）
- 奖励：`clue_gate_manifest`、`item_gate_token`、`num_reputation` +1
- 战斗：`enc_ch3_gate`（私兵 ×3、弓手 ×2、关卒 ×1），地形 `gate`；失败 → 重伤回关内，`ch3_gate_cleared` 不置位但仍推进

### mq_3_05 过关
- 前置：`mq_3_04` completed
- 目标：`mq_3_05_obj_choose`（交人/交货/自守，flag `ch3_final_choice`）、`mq_3_05_obj_ending`（`ch3_ending` != `none`）
- 完成奖励：flag `ch3_done`，`ch03.state.json` 写 `"next_chapter": "ch04"`

## 7. 支线任务（3 条）

| id | 标题 | 前置 | 目标 | 奖励 | 作用 |
| --- | --- | --- | --- | --- | --- |
| `sq_3_01` | 无名卒 | `ch3_customs_done` | 替一名被顶名的关卒补正名籍 | `clue_erased_name`、`num_reputation` +1 | 第四章"三十七个名字"的预演 |
| `sq_3_02` | 白狐的信 | 持有 `clue_baiji_letter` | 送 / 拆 / 烧第二章托付的信 | `num_baiji_trust` 或 `num_baiji_suspicion` | 决定白姬本章立场与第四章是否放行 |
| `sq_3_03` | 商队的空位 | `num_aqi_trust ≥ 2` 且 `ch3_customs_done` | 为阿耆商队补一名向导 | `clue_oasis_route`、`item_convoy_tally` 复用、`num_aqi_trust` +1 | 打开第四章关外路线 |

## 8. 章末分支与结局

`scene_3_end` 的最终选择结合状态写入 `ch3_ending`：

| 结局 | 条件（选择 + 状态） | 结果 | 第四章开局 |
| --- | --- | --- | --- |
| `handover` 交人 | 选"交人"且 `ch2_zhougan_protected` 为假或 `num_zhougan_trust < 3` | 周敢/白姬被押，货放行；冯安保住官身 | 郡府对质线，证据链缺口 |
| `cargo` 交货 | 选"交货"且 `num_baiji_trust ≥ 3` 或 `ch2_duyan_deal` 为真 | 货被扣、人被放；商路断，白姬记情 | 关外追索线，商队可用 |
| `stand` 自守 | 选"自守"且 `num_reputation ≥ 8` 或 `clue_caocao_roster` 已得 | 关下对峙闹到郡府；赵钦反水作证 | 悬泉置清算线，声望最高 |

任一结局：`clue_gate_manifest` 至少获得一份；`ch3_done` 置真；`ch03.state.json` 的 `next_chapter` 指向 `ch04`。

## 9. 新增状态

### flags
`ch3_start`、`ch3_open_done`、`ch3_qixin_checked`、`ch3_manifest_declared`、`ch3_party_passed`、`ch3_customs_done`、`ch3_stub_checked`、`ch3_qixin_traced`、`ch3_qixin_forgery`、`ch3_stub_done`、`ch3_seal_compared`、`ch3_roster_got`、`ch3_weichang_confronted`、`ch3_roster_done`、`ch3_zhougan_testified_2`、`ch3_zhougan_stayed`、`ch3_baiji_paid`、`ch3_baiji_done`、`ch3_duyan_paid`、`ch3_duyan_done`、`ch3_party_ready`、`ch3_gate_tracked`、`ch3_gate_cleared`、`ch3_seal_seized`、`ch3_gate_done`、`ch3_final_choice`、`ch3_done`、`sq3_01_done`、`sq3_02_done`、`sq3_03_done`。

跨章只读取：`ch2_ending`、`ch2_zhougan_protected`、`ch2_duyan_deal`、`ch2_plan_seize`、`ch2_plan_trail`、`ch2_plan_swap`、`evidence_with_su`、`ch1_guowang_hurt`、`ch1_duyan_alerted`。

### variables
| id | 范围 | 说明 |
| --- | --- | --- |
| `num_gate_trust` | 0–3 | 关令成公朔的信任；影响查验宽严与关下是否动武 |
| `num_weichang_pressure` | 0–3 | 卫敞对冯安的施压；高值时关吏提前得信 |
| `ch3_ending` | enum `none/handover/cargo/stand` | 章末结局，第四章分支键 |

### clues
`clue_gate_registry`（关传登记）、`clue_qixin_stub`（棨信副券存根）、`clue_caocao_roster`（仓曹名录）、`clue_seal_owner_full`（残印与卫敞的对应关系）、`clue_gate_manifest`（关下货单）、`clue_erased_name`（被顶名的关卒）、`clue_oasis_route`（关外路线）。

### items
`item_gate_token`（关传，图标待补，可先复用 `item_qixin`）、`item_caocao_roster`（仓曹名录，图标复用 `item_bamboo_slip`）。

### encounters
`enc_ch3_gate`：私兵 ×3、弓手 ×2、关卒 ×1，地形 `gate`；`ch1_guowang_hurt` 为真时郭望带伤，战斗难度提示 +1。

## 10. 技术契约与资源

- **章节链接**：`ch03.state.json` 写 `"next_chapter": "ch04"`；`ch02.state.json` 已补 `"next_chapter": "ch03"`，`--campaign` 可从第二章自动进入本章。
- **携带块**（`ch03.state.json`）：
  ```json
  "carryover": {
    "from": "ch02",
    "variables": ["num_su_trust", "num_baiji_suspicion", "num_hanbo_trust",
                  "num_guowang_trust", "num_aqi_trust", "num_prefecture_suspicion",
                  "num_reputation", "num_zhougan_trust", "num_baiji_trust",
                  "num_zhaoqin_pressure", "ch2_ending"],
    "flags": ["ch2_zhougan_protected", "ch2_duyan_deal", "ch2_plan_seize",
              "ch2_plan_trail", "ch2_plan_swap", "evidence_with_su",
              "ch1_guowang_hurt", "ch1_duyan_alerted"]
  }
  ```
- **节点 id**：全章前缀 `ch3_`，节点 id 章内唯一。
- **独立可跑**：无上一章时以默认值开局（`ch2_ending = none` 走通用开局），沿用第二章做法。
- **美术缺口**：已补齐。背景 `bg_yumenguan_gate`（关楼内侧/关下夜景），立绘 `char_chenggong`（关令成公朔）、`char_weichang`（仓曹史卫敞），图标 `item_gate_token`（关传）均已生成并入库；对话中的人物立绘与 `item_gate_token` 图标引用已回填。
- **文本量**：9 场景、101 节点、5 主线 + 3 支线，与第二章相当。

## 11. 后续章节大纲

- **第四章《三十七个名字》**：冬天与清算。三十七人的名字要被重新写回名籍，代价是有人必须认罪。以悬泉置/烽燧防守或郡府对质收束全篇，按 `num_reputation`、`num_su_trust`、周敢生死、证据完整度给出多结局评价。

## 12. 默认创作取舍（可直接按此实现）

1. 成公朔不是贪官：他只认文书，玩家要用证据链说服他，而不是靠钱。
2. 卫敞必须是"制度内的人"：他能私刻旧印，是因为他管着旧印档案；第四章才追到他头上。
3. 赵钦可转化：`threaten_zhaoqin` + 账册在手时，他在第四章会反水作证。
4. 周敢生死沿用第二章判定：保护且 `num_zhougan_trust ≥ 3` 才在本章当关作证，否则只留供词简。
5. 三条结局必须都能进第四章，不做死局。

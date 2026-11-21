# 第四章《三十七个名字》美术需求清单

> 配套设计：`docs/narrative/ch03-design.md` §11 第四章大纲（尚未落地章节数据）。
> 本清单只定义**美术资产需求**，不包含剧情数据与代码改动。
> 生成时机：第四章设计稿定稿后、数据落地前/并行。
>
> **已确认决策**（本轮锁定）：
> 1. 郡守定名为 **严崇**，职务为敦煌郡守，第四章主审；外观取**墨绿肃穆**（深墨绿官袍配赤缘、梁冠、清瘦半白须）。
> 2. 结局 CG **做 3 张**，**走现有背景通道**承载（不新增 UI 全屏图层，结局时作为背景图切换），存 `game-assets/backgrounds/cg_ending_*.jpg`，`assets.json` 中 `category` 记为 `backgrounds`。
> 3. 功曹、狱吏维持 P1，暂不与郡守同列主审。

## 1. 视觉基调

- **季节与色调**：深冬。整体偏冷灰蓝与雪白，灯火/火把提供暖橙点缀；避免高饱和色。
- **画风**：延续第一至三章的水墨平涂 + 干净线稿 + 低饱和土色，背景保留暖米色纸纹质感。
- **情绪**：清算与告别。前面的"追查"变成"落笔"，画面重心从道路、关楼转向室内、名册与人群。
- **禁止项**：画面内不得出现可读文字、水印、现代器物；名册上的字一律用示意笔画处理。

## 2. 现有可复用资源

| 用途 | 已有资产 |
| --- | --- |
| 悬泉置坞院 / 室内 | `bg_station_courtyard`、`bg_station_interior` |
| 烽燧 / 戈壁 / 绿洲 / 祁连山 | `bg_beacon_tower`、`bg_gobi_postroad`、`bg_oasis`、`bg_qilian_mountain` |
| 关城 | `bg_yumenguan_dusk`、`bg_yumenguan_gate` |
| 主角与常驻角色 | `char_fengan`、`char_suhe`、`char_hanbo`、`char_guowang`、`char_duyan`、`char_baiji`、`char_zhougan`、`char_zhaoqin`、`char_aqi` |
| 第三章新增 | `char_chenggong`（关令）、`char_weichang`（仓曹史） |
| 通用群像 | `char_yizu`、`char_shuzu`、`char_yiren`、`char_shizhe`、`char_shanglv` |
| 简牍 / 印信 / 文书 | `item_bamboo_slip`、`item_seal`、`item_seal_clay`、`item_qixin`、`item_gate_token`、`item_brush_knife`、`item_hemp_paper` |
| 战斗与器物 | `item_crossbow`、`item_horse_harness`、`item_oil_lamp`、`item_convoy_tally` |
| 材质 | `tex_rammed_earth`、`tex_wood_bamboo`、`tex_silk_pattern` |

> 复用原则：能靠已有背景 + 光效/天气变体表达的，不新建；只有"场景语义不同"才新增背景。

## 3. 新增需求总表

| 编号 | id | 类别 | 名称 | 用途 | 尺寸规格 | 优先级 |
| --- | --- | --- | --- | --- | --- | --- |
| C4-BG-01 | `bg_commandery_hall` | 背景 | 郡府正堂 | 章末对质、宣罪 | landscape_16_9 | P0 |
| C4-BG-02 | `bg_station_snow_night` | 背景 | 悬泉置雪夜 | 开局、夜巡、结局 | landscape_16_9 | P0 |
| C4-BG-03 | `bg_register_archive` | 背景 | 名籍架阁 | 查名籍、抄录三十七人 | landscape_16_9 | P1 |
| C4-BG-04 | `bg_beacon_snow_night` | 背景 | 烽燧雪夜 | 防守战、烽火传警 | landscape_16_9 | P1 |
| C4-BG-05 | `bg_prison_yard` | 背景 | 亭狱小院 | 狱吏线、周敢/卫敞收押 | landscape_16_9 | P2 |
| C4-CH-01 | `char_taishou` | 立绘 | 郡守严崇 | 郡府对质的主审 | portrait_4_3 | P0 |
| C4-CH-02 | `char_gongcao` | 立绘 | 郡府功曹 | 宣读文书、传令 | portrait_4_3 | P1 |
| C4-CH-03 | `char_yuli` | 立绘 | 亭狱狱吏 | 收押与递话 | portrait_4_3 | P1 |
| C4-IT-01 | `item_register_slips` | 图标 | 名籍简册 | 三十七人名册 | square | P0 |
| C4-IT-02 | `item_indictment` | 图标 | 劾状 | 弹劾卫敞/赵钦 | square | P0 |
| C4-IT-03 | `item_sealed_document` | 图标 | 印信封检 | 郡府回文、封检 | square | P1 |
| C4-IT-04 | `item_beacon_torch` | 图标 | 烽火苣 | 烽燧防守、传警 | square | P1 |
| C4-CG-01 | `cg_ending_names` | 结局图 | 名籍落笔 | `stand` 结局 | landscape_16_9 | P1 |
| C4-CG-02 | `cg_ending_exile` | 结局图 | 押解东行 | `handover` 结局 | landscape_16_9 | P1 |
| C4-CG-03 | `cg_ending_road` | 结局图 | 关外长路 | `cargo` 结局 | landscape_16_9 | P1 |
| C4-UI-01 | `ui_register_scroll` | UI | 名籍长卷底 | 三十七人名单展示 | 竖向长图 | P2 |
| C4-UI-02 | `ui_ending_seal` | UI | 结局评语印 | 结局评价盖章 | 透明 PNG | P2 |
| C4-UI-03 | `title_ch04` | UI | 章节标题图 | 章节过场标题 | landscape_16_9 | P2 |

统计：背景 5、立绘 3、图标 4、结局图 3、UI 3；其中 P0 共 5 项。

## 4. 背景需求细则

### C4-BG-01 郡府正堂 `bg_commandery_hall`（P0·已交付）
> 已入库：`game-assets/backgrounds/bg_commandery_hall.jpg`，2560×1440，JPG；缩略图 360×203。
- **画面**：汉代郡府正堂内景，高梁粗柱，正中设主位案席，两侧列书佐案几与简架；地铺席，壁挂旧地图；窗外是灰白冬日光。
- **构图**：一点透视，主位略偏中，留出前景站位给对峙双方；不要出现具体人物。
- **色彩**：深褐木构 + 灰青地席，冷调为主，案上灯烛提供唯一暖光。
- **用途**：第四章章末对质、宣罪、三结局的共同收束场景。
- **回退**：缺图时可用 `bg_station_interior`，但压迫感明显不足，建议优先交付。

### C4-BG-02 悬泉置雪夜 `bg_station_snow_night`（P0·已交付）
> 已入库：`game-assets/backgrounds/bg_station_snow_night.jpg`，2560×1440，JPG；缩略图 360×203。
- **画面**：与 `bg_station_courtyard` 同一视角的雪夜版：夯土墙与坞院覆薄雪，门楼挂一盏风灯，远处戈壁全白，天压得低。
- **差异要求**：必须能和使用 `bg_station_courtyard` 的日间场景形成"同一地点、不同时间"的对照（墙体结构、门洞位置一致）。
- **用途**：第四章开局、夜巡、结局回望。

### C4-BG-03 名籍架阁 `bg_register_archive`（P1·已交付）
> 已入库：`game-assets/backgrounds/bg_register_archive.jpg`，2560×1440，JPG；缩略图 360×203。
- **画面**：悬泉置/郡府的名籍库房：成排木架，束束简册与封检按年份码放，矮案上一盏油灯、一支笔、一堆待抄的空简。
- **构图**：视线沿架间过道向深处延伸，前景案面留空，便于叠放 UI 名册。
- **回退**：可先用 `bg_station_interior`。

### C4-BG-04 烽燧雪夜 `bg_beacon_snow_night`（P1·已交付）
> 已入库：`game-assets/backgrounds/bg_beacon_snow_night.jpg`，2560×1440，JPG；缩略图 360×203。
- **画面**：雪夜烽燧，燧顶火苣初燃，火光映雪，梯道与女墙结霜；远处地平线有第二座燧的微弱火光。
- **用途**：防守战与"烽火传警"节点。
- **回退**：可先用 `bg_beacon_tower`。

### C4-BG-05 亭狱小院 `bg_prison_yard`（P2·已交付）
> 已入库：`game-assets/backgrounds/bg_prison_yard.jpg`，2560×1440，JPG；缩略图 360×203。
- **画面**：低矮土墙围出的小院，一间带木栅的囚室，院中一口井与一只水瓮；雪被踩出杂乱脚印。
- **用途**：狱吏线、收押段落。

## 5. 角色立绘需求细则

通用要求同现有立绘：正面全身、头到脚完整、暖米色纸纹背景、水墨平涂低饱和、无文字；1680×2240。

### C4-CH-01 郡守严崇 `char_taishou`（P0·已交付）
> 已入库：`game-assets/characters/char_taishou.jpg`，1680×2240，JPG；缩略图 195×260。
- **身份**：两千石郡守，第四章主审，决定"名字能不能写回去"。
- **外观**：五十岁上下，清瘦，深青/墨绿官袍配赤缘，梁冠，腰悬印绶；须发半白，眼神疲惫但不怒自威。
- **姿态**：一手按案（或持简），一手垂袖；站姿端正，不摆架子但不可亲近。
- **注意**：与 `char_zhaoqin`（黑色官服）区分靠色相与冠式，避免看起来像同一人换色。

### C4-CH-02 郡府功曹 `char_gongcao`（P1·已交付）
> 已入库：`game-assets/characters/char_gongcao.jpg`，1680×2240，JPG；缩略图 195×260。
- **身份**：郡府功曹，负责宣读文书、传令、唱名。
- **外观**：三十五岁左右，赭褐色官袍，进贤冠，手持一卷展开的简册，腰间挂笔囊。
- **姿态**：微侧身，像正在宣读；表情公事公办。

### C4-CH-03 亭狱狱吏 `char_yuli`（P1·已交付）
> 已入库：`game-assets/characters/char_yuli.jpg`，1680×2240，JPG；缩略图 195×260。
- **身份**：亭狱狱吏，负责收押与递话，可被收买也可被吓住。
- **外观**：四十岁上下，短褐外罩旧皂衣，束发无冠，腰挂钥匙串与短刀；面色蜡黄，手指粗糙。
- **姿态**：一手提钥匙，一手扶腰刀；站姿松垮，眼神打量。

> 缺失兜底：现有 `DialogueScreen` 在立绘缺图时不显示人物、不崩溃；对话数据中 `portrait` 可先置 `null`，图到位后再回填 id。

## 6. 物品图标需求细则

通用要求：器物居中、纯色低饱和底、平涂笔触、无文字；1920×1920。

| id | 器物描述 | 建议底色 | 用途 |
| --- | --- | --- | --- |
| `item_register_slips` | 一束用麻绳编连的名籍简册，部分简条缺字、边缘磨损，旁压一枚小封泥 | 深墨绿 | 三十七人名册主图标 |
| `item_indictment` | 一卷展开的劾状木牍，朱笔勾画的示意笔画，一角有官印压痕 | 暗赭红 | 弹劾卫敞/赵钦 |
| `item_sealed_document` | 一封带封检与绳结的公文，封泥上有印文压痕 | 暖灰褐 | 郡府回文、调令 |
| `item_beacon_torch` | 一束麻布缠头的火苣，顶端焦黑，柄部缠绳 | 冷灰蓝 | 烽燧传警、防守道具 |

> P0 已交付：`item_register_slips`、`item_indictment` 均已入库（`game-assets/items/`，1920×1920，缩略图 260×260）。
> P1 已交付：`item_sealed_document`、`item_beacon_torch` 均已入库（规格同上）。

## 7. 结局插图（已确认：做 3 张，走背景通道）

三张对应 `ch3_ending` 延伸出的第四章结局，统一构图语言：**一个背影 + 一个远景**，不画正脸，不写字。

| id | 结局 | 画面 |
| --- | --- | --- |
| `cg_ending_names` | 自守/翻案 | 雪后的名籍架阁，长案上摊开的名册被晨光照亮，案边一只搁下的笔 |
| `cg_ending_exile` | 交人/押解 | 冬日官道上向东的押解队伍剪影，路旁枯苇，远处郡城轮廓 |
| `cg_ending_road` | 交货/出关 | 关外雪原上的一行车辙与两匹马，回头望向关楼的方向 |

> 承载方式（已确认）：作为背景资产存入 `game-assets/backgrounds/`，`category = backgrounds`，缩略图 360×203；第四章对话数据在结局节点把场景 `background` 切到对应的 `cg_ending_*`，无需 UI 代码改动。
> 交付状态：3 张均已入库（2560×1440，JPG）。

## 8. UI / 氛围图（P2）

| id | 内容 | 说明 |
| --- | --- | --- |
| `ui_register_scroll` | 竖向名籍长卷底图 | 供"三十七人名单"逐条显现；需 UI 支持纵向滚动/逐条高亮 |
| `ui_ending_seal` | 朱文结局印 | 透明底，用于结局评语盖章动效 |
| `title_ch04` | 章节标题过场 | 雪原 + 残燧剪影，留出标题文字安全区 |

> 三项素材已生成入库（`game-assets/ui/`）：`ui_register_scroll` 1440×3840 JPG、`ui_ending_seal` 1024×1024 透明 PNG、`title_ch04` 2560×1440 JPG；缩略图见 `_thumbnails/ui/`。
>
> **代码接入状态**：`ui_register_scroll` 与 `ui_ending_seal` 已接入 `DialogueScreen`——`register` 节点展示可滚动长卷并逐条显现姓名（滚轮/↑↓/PgUp/PgDn 滚动，空格/回车/左键继续），`seal` 节点展示评语卡并播放盖章动效。`title_ch04` 章节标题过场尚未接入。
>
> 数据写法：
> ```json
> { "id": "r1", "type": "register", "title": "名籍", "subtitle": "悬泉置 · 三十七人",
>   "names": ["张季", "周敢", "…"], "next": "s1" }
> { "id": "s1", "type": "seal", "title": "结局 · 自守",
>   "text": "三十七个名字，重新写回名籍。", "seal": "ui_ending_seal", "next": "e1" }
> ```

## 9. 规格与入库流程

1. **尺寸与格式**
   - 背景 / 结局图：`landscape_16_9` → 2560×1440，JPG。
   - 立绘：`portrait_4_3` → 1680×2240，JPG。
   - 物品图标：`square` → 1920×1920，JPG。
   - UI 透明素材：PNG（需要透明通道时）。
2. **缩略图**：背景 360×203、立绘 195×260、物品 260×260、UI 长卷 195×520、UI 方图 260×260，存 `game-assets/_thumbnails/<category>/`。
3. **assets.json 条目字段**：`id`、`category`、`name`、`local_path`、`thumbnail`、`width`、`height`、`bytes`、`sha256`、`references`、`image_size`。
4. **命名**：背景 `bg_*`、立绘 `char_*`、物品 `item_*`、结局图 `cg_*`、UI `ui_*` / `title_*`。
5. **回填**：图入库后，把对话 `speakers.*.portrait`、场景 `background`、state `items.*.icon` 从 `null`/占位改为正式 id，并跑一遍引用完整性检查。

## 10. 生成 prompt 模板（英文，供出图直接使用）

**立绘**
```
Character portrait for a Han dynasty narrative adventure game: full-body standing portrait of [角色身份/年龄/体型], wearing [袍服颜色与缘边/冠式], [手持物/姿态], [表情]. Hanfu, front view, full body visible head to feet. Chinese ink-wash lineart with flat low-saturation earth tones, clean linework, warm beige paper texture background, no text, no watermark
```

**背景（冬景）**
```
Game background art for a Han dynasty narrative adventure, deep winter: [地点与陈设], a thin layer of snow, cold gray-blue palette with a single warm torch or lamp as the light source, wide cinematic composition. Chinese ink-wash and watercolor illustration, flat low-saturation earth tones, warm beige paper texture, clean linework, no text, no watermark, no prominent people
```

**物品图标**
```
Game item icon for a Han dynasty narrative adventure: [器物形制与磨损细节]. Painterly illustration, centered object, clean linework with flat low-saturation earth tones, on a solid muted [颜色] background, no text, no watermark
```

## 11. 分批交付

| 批次 | 内容 | 说明 |
| --- | --- | --- |
| 第一批（P0） | `bg_commandery_hall`、`bg_station_snow_night`、`char_taishou`、`item_register_slips`、`item_indictment` | 已全部交付入库 |
| 第二批（P1） | `bg_register_archive`、`bg_beacon_snow_night`、`char_gongcao`、`char_yuli`、`item_sealed_document`、`item_beacon_torch`、3 张结局图 | 已全部交付入库 |
| 第三批（P2） | `bg_prison_yard`、`ui_register_scroll`、`ui_ending_seal`、`title_ch04` | 已交付入库；长卷与盖章已接入代码，章节标题过场待接入 |

## 12. 验收清单

- [x] 所有 P0 资产有文件、有缩略图、已进 `assets.json`。
- [ ] 资源引用完整性检查通过：对话立绘、场景背景、state 图标全部能对应到文件。
- [x] 每张 P0 图的尺寸与 `image_size` 字段一致（1680×2240 / 2560×1440 / 1920×1920）。
- [x] P1 资产（含 3 张结局图）有文件、有缩略图、已进 `assets.json`，尺寸与 `image_size` 一致。
- [x] P2 素材（`bg_prison_yard`、3 项 UI）有文件、有缩略图、已进 `assets.json`；`ui_ending_seal` 为透明 PNG。
- [x] `register` / `seal` 节点已接入渲染层，长卷滚动与盖章动效冒烟通过。
- [ ] 风格一致性抽检：与 `char_fengan`、`bg_station_courtyard`、`item_qixin` 并排比对，无画风跳脱。
- [ ] `ctest` 全绿；`han_client --chapter ch04 --frames 90` 冒烟通过（第四章数据未落地，暂用 ch03 冒烟）。
- [ ] 章节内不出现可读文字与水印。

## 13. 依赖与风险

1. **剧情定名**：郡守"严崇"已定名；功曹、狱吏仍为暂定角色，其名字/职务待第四章设计稿确认（外观可先按本清单做，名字只影响 `name` 字段）。
2. **结局图承载**：已确认走背景通道，不依赖 UI 改动；第四章数据在结局节点切换 `background` 即可。
3. **雪景一致性**：`bg_station_snow_night` 必须与 `bg_station_courtyard` 同构，建议同一次生成会话连续出图，降低结构漂移。
4. **立绘色相冲突**：新官服要与赵钦（黑）、卫敞（靛灰）、成公朔（褐）错开，否则玩家难以在对话中区分。郡守已定墨绿，满足错开要求。

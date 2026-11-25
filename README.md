# 悬泉渡（Xuanquan Du）

一款以西汉边塞为背景的中文叙事冒险游戏。玩家扮演敦煌郡悬泉置的书佐冯安，从一封没有封泥、没有印文的信开始，追查一桩用死者名字运转了十年的军资旧案。

游戏共 **11 章、106 个场景、1115 个剧情节点、81 个任务**，全部剧情、任务、分支与结局均由 JSON 数据驱动，可在游戏内从第一章完整通关到终章。

![CI](https://github.com/Lwj1311/SFML-HAN/actions/workflows/ci.yml/badge.svg)

---

## 背景故事

西汉宣帝地节年间，敦煌郡悬泉置。这里是中原通往西域的驿传节点，邮书、粮草、马匹与过往吏卒都在此交接。

冯安是置里的一名书佐，平日替韩伯抄简、核对廪食簿。故事从一个大风沙的深夜开始：驿门被人急促敲响，来人张季丢下一封信便走，只留下一句“广至驿让交给书佐的，别问是谁写的”。

信上没有封泥，也没有印文，只有两行字：

> 冬至前，三十七人粮，照旧。广至驿收。

名籍上，那三十七个人早已阵亡。死人不会领粮，活人才会。顺着这封信，冯安查到一条用死者名字维持的私运链：改名的名籍、冒领的粮账、未除籍的马簿、私铸的钱、被拆封的官文书——每一桩都能单独结案，合在一起却是同一只手在写。

从悬泉置到广至驿，从玉门关到敦煌郡府，冯安必须在官身、人情与真相之间反复取舍。有人要他压下证据，有人愿意用总账换一条生路，也有人只认文书不认人。

故事在终章汇合：郡府下令十年旧案总核，名籍、粮账、马簿、钱账、封检并案。五本账合上的那一刻，三十七户的名字与十年的旧账，都要有一个明确的了结。

> 游戏为虚构作品，人物与案情均为创作；时代背景、驿置制度与简牍形制参考了悬泉置汉简及汉代边塞史料。

---

## 章节一览

| 章 | 标题 | 主题 | 场景 | 节点 |
| --- | --- | --- | --- | --- |
| 第一章 | 无印之信 | 一封没有印的信，与三十七个死人的名字 | 18 | 157 |
| 第二章 | 白霜之约 | 用死人名字运转的军资私运链 | 9 | 119 |
| 第三章 | 过关 | 印与关：谁有资格开门、放人、销名 | 9 | 101 |
| 第四章 | 三十七个名字 | 把死人写回名籍 | 11 | 139 |
| 第五章 | 名字之后 | 后日谈：三种结局之后的人与事 | 4 | 40 |
| 第六章 | 对账 | 廪食簿与私简：十年旧账第一次对不上 | 10 | 97 |
| 第七章 | 附籍 | 追缴令下的户版与人身 | 9 | 88 |
| 第八章 | 马簿 | 驿马、马料与被重烙的印 | 9 | 92 |
| 第九章 | 钱法 | 轻钱、私铸与关下的钱账 | 9 | 92 |
| 第十章 | 封检 | 被拆开又照原样封回的官文书 | 9 | 94 |
| 第十一章 | 总账 | 终章：十年五案并案，旧案总清算 | 9 | 96 |

前四章为连续主线，第五章为多结局后日谈，第六至十章为独立新案件，第十一章为单一终局的完结章。每章结束时通过 `next_chapter` 自动衔接，跨章只携带双方都声明过的变量与旗标。

---

## 玩法特色

- **分支对话**：每个选择都可能改变旗标、数值、线索与后续场景的触发条件。
- **数据驱动任务**：每章 5 条主线 + 3 条支线（第五章为单任务后日谈），目标、奖励与完成条件全部写在 JSON 中。
- **线索与证物**：封泥、简册、货单、印痕等线索进入案卷，物品可跨场景使用。
- **叙事战斗**：战斗以遭遇配置呈现（敌人、数量、地形），胜败都会推进主线，不会锁死流程。
- **多结局与终局**：前中期章节多为三结局，终章将不同路线收束为同一个明确结局，并附尾声。
- **跨章携带**：`carryover` 声明上一章 id 与需要继承的变量/旗标，战役模式自动携带。
- **主菜单与存档**：新游戏、继续游戏、章节选择、章节标题卡、自动存档（`saves/slot1.json`）。
- **中文字体与排版**：支持通过 `--font` 指定 TTF/TTC 字体文件。

---

## 技术栈

| 类别 | 选型 |
| --- | --- |
| 语言 | C++20（`/W4 /utf-8 /permissive-`） |
| 图形与窗口 | SFML 2.6（graphics / window / system） |
| 数据解析 | nlohmann/json |
| 构建系统 | CMake ≥ 3.21，MSVC（Visual Studio 2022） |
| 依赖管理 | vcpkg manifest（`vcpkg.json`） |
| 测试 | 自研轻量测试框架 + CTest |
| CI/CD | GitHub Actions（Windows x64：构建、测试、打包、包冒烟、产物上传） |
| 平台 | Windows x64（当前唯一验证平台） |

叙事层（`han_narrative`）是无窗口静态库，客户端（`han_client`）只负责渲染与输入，因此剧情可以在无图形环境下完整跑测试。

---

## 项目结构

```text
SFML-HAN/
├── CMakeLists.txt              # 构建入口，定义 han_narrative / han_client / tests
├── vcpkg.json                  # nlohmann-json 与 sfml 依赖
├── assets/data/narrative/      # 剧情数据（每章三份 JSON）
│   ├── ch01.state.json         # 章节状态：旗标、变量、线索、物品、携带规则
│   ├── ch01.quests.json        # 任务：目标、前置、完成条件、奖励
│   └── ch01.dialogue.json      # 对话：角色、遭遇、场景、节点图
├── src/
│   ├── narrative/              # 无窗口叙事引擎
│   │   ├── narrative_state.*   # 状态、变量钳制、线索与物品
│   │   ├── condition.*         # 条件表达式求值
│   │   ├── effects.*           # 效果执行
│   │   ├── quest_system.*      # 任务状态机与奖励
│   │   ├── dialogue.*          # 场景与节点加载、运行
│   │   ├── session.*           # 章节会话、跨章携带、存档快照
│   │   └── validator.*         # 章节数据静态校验
│   ├── ui/                     # SFML 界面：标题页、对话页
│   └── app/main.cpp            # 入口、命令行、主循环、存档
├── tests/                      # 叙事引擎测试（53 个用例）
├── game-assets/                # 背景、立绘、物品图标、UI 与资源清单
├── docs/narrative/             # 章节设计文档
└── .github/workflows/ci.yml    # Windows 构建 / 测试 / 打包流水线
```

---

## 构建与运行

### 前置条件

- Windows 10/11 x64
- Visual Studio 2022（含 C++ 桌面开发工作负载）
- CMake ≥ 3.21
- vcpkg（设置 `VCPKG_ROOT` 或使用随 VS 安装的 vcpkg）

### 构建

```powershell
git clone https://github.com/Lwj1311/SFML-HAN.git
cd SFML-HAN

cmake -S . -B build -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

cmake --build build --config Release --parallel
```

如果使用官方 SFML 2.6 SDK 压缩包而不是 vcpkg 的 SFML，可额外传入：

```powershell
-DHAN_SFML_ROOT="D:/SDK/SFML-2.6.1"
```

构建产物：

- `build/Release/han_client.exe`：游戏客户端
- `build/Release/han_narrative_tests.exe`：叙事引擎测试

### 运行

```powershell
# 主菜单（新游戏 / 继续 / 章节选择）
build\Release\han_client.exe

# 直接进入指定章节
build\Release\han_client.exe --chapter ch07 --data assets/data/narrative --assets game-assets

# 战役模式：章节结束后按 next_chapter 自动进入下一章并携带状态
build\Release\han_client.exe --campaign

# 指定中文字体
build\Release\han_client.exe --font "C:/Windows/Fonts/simhei.ttf"
```

### 命令行参数

| 参数 | 说明 |
| --- | --- |
| `--data <目录>` | 章节数据目录，默认编译期写入的 `assets/data/narrative` |
| `--chapter <id>` | 直接进入指定章节，跳过主菜单 |
| `--assets <目录>` | 美术资源根目录，默认 `game-assets` |
| `--font <文件>` | 中文 TTF/TTC 字体 |
| `--campaign` | 按 `next_chapter` 自动续章并携带状态 |
| `--frames <数量>` | 渲染指定帧数后退出（冒烟测试） |
| `--check` | 仅加载章节数据后退出（打包冒烟） |
| `--help` | 显示帮助 |

存档写入可执行文件同级的 `saves/slot1.json`，在场景边界自动保存。

---

## 测试与持续集成

```powershell
ctest --test-dir build -C Release --output-on-failure
```

当前共 **53 个测试用例**，覆盖：

- 各章数据加载与静态校验（节点 id 唯一、跳转目标存在、分支兜底、引用声明齐全）
- 条件语言与效果执行（旗标、变量钳制、线索、物品、任务状态）
- 单章独立通关与多结局
- 跨章携带（`carryover` 正确复制变量、章节不匹配时拒绝加载）
- 全战役从第一章连续通关到终章，校验八条任务全部完成与终章结局

CI 流水线在每次 push / PR 时执行：配置 → 构建 → 测试 → 打包 zip（客户端 + DLL + 剧情数据 + 美术资源 + VC 运行库）→ 包内 `--check` 冒烟 → 上传产物；打 `v*` 标签时自动发布 Release。

---

## 剧情数据格式

每章由三份 JSON 组成，章节之间用 `state.json` 的 `next_chapter` 串联。

### `chXX.state.json`

声明章节 id、标题、携带规则、初始旗标与变量，以及全部变量范围、线索与物品定义。

```json
{
  "chapter_id": "ch07",
  "title": "第七章 附籍",
  "next_chapter": "ch08",
  "carryover": { "from": "ch06", "variables": ["ch6_ending"] }
}
```

### `chXX.quests.json`

声明任务类型、前置条件、目标、完成条件与奖励。奖励类型支持 `flag`、`clue`、`item`、`variable`（变量使用 `delta`）。

### `chXX.dialogue.json`

声明角色（含立绘）、遭遇配置与场景。场景按 `trigger` 顺序取首个匹配项，节点类型包括：

| 类型 | 作用 |
| --- | --- |
| `narration` | 旁白 |
| `line` | 角色台词（支持条件变体） |
| `choice` | 选项与效果 |
| `branch` | 条件分支，必须有兜底分支 |
| `combat` | 遭遇战，配置 `on_win` / `on_lose` |
| `seal` | 章节印章卡 |
| `register` | 名籍/册页展示 |
| `end` | 场景结束 |

效果命令包括 `set_flag`、`add_var`、`set_var`、`add_clue`、`give_item`、`start_quest`、`complete_objective`、`set_quest_state` 等。所有引用都会在加载与测试阶段被校验器检查。

---

## 美术资源

`game-assets/` 现有 **18 张背景、19 张角色立绘、22 个物品图标、3 张 UI 素材、3 张纹理**，资源清单见 `game-assets/assets.json` 与 `assets.csv`。

美术流程参考甘肃、武威等地博物馆公开文物资料（如陶俑、铜奔马、汉简），记录参考来源与哈希后，经图像生成与人工筛选入库；每条资源在 `assets.json` 中保留 `references` 字段以便溯源。涉及第三方文物图像时，商用前请自行核对原始授权。

---

## 已知限制

- 仅在 Windows x64 + MSVC 上验证，未提供 Linux/macOS 构建脚本。
- 音频系统尚未接入（音乐、环境声、音效）。
- 战斗为叙事化遭遇结算，没有独立战棋或动作玩法。
- 单存档槽位；暂无设置菜单、按键重绑定与多语言。

---

## 致谢

- 时代与制度参考：悬泉置汉简、居延汉简及汉代边塞驿传研究。
- 美术参考：甘肃省博物馆、武威市博物馆等公开文物资料。
- 技术依赖：[SFML](https://www.sfml-dev.org/)、[nlohmann/json](https://github.com/nlohmann/json)、[vcpkg](https://vcpkg.io/)。

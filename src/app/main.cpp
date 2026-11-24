#include "narrative/session.h"
#include "ui/dialogue_screen.h"
#include "ui/title_screen.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

#ifndef HAN_DATA_DIR
#define HAN_DATA_DIR "assets/data/narrative"
#endif
#ifndef HAN_ASSET_DIR
#define HAN_ASSET_DIR "game-assets"
#endif

namespace {

void print_usage() {
    std::cout << "用法: han_client [选项]\n"
                 "  --data <目录>    章节数据目录 (默认 " HAN_DATA_DIR ")\n"
                 "  --chapter <id>   直接进入指定章节，跳过主菜单 (默认 ch01)\n"
                 "  --assets <目录>  美术资源根目录 (默认 " HAN_ASSET_DIR ")\n"
                 "  --font <文件>    中文字体 TTF/TTC\n"
                 "  --campaign       章节结束后按 next_chapter 自动进入下一章并携带状态\n"
                 "  --frames <数量>  渲染指定帧数后退出（冒烟测试）\n"
                 "  --check          仅加载章节数据后退出（打包冒烟验证）\n"
                 "  --help           显示本帮助\n";
}

std::filesystem::path executable_dir() {
#ifdef _WIN32
    wchar_t buffer[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length > 0) {
        return std::filesystem::path(buffer, buffer + length).parent_path();
    }
#endif
    return std::filesystem::current_path();
}

/// Packaged builds compile in paths relative to the executable; keep them
/// anchored there so the extracted zip runs from any working directory.
std::filesystem::path resolve_default_dir(const std::filesystem::path& dir) {
    return dir.is_absolute() ? dir : executable_dir() / dir;
}

std::vector<han::ui::ChapterEntry> discover_chapters(
    const std::filesystem::path& data_dir) {
    std::vector<han::ui::ChapterEntry> chapters;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(data_dir, ec)) {
        if (ec || !entry.is_regular_file()) {
            continue;
        }
        const std::string name = entry.path().filename().string();
        const std::string suffix = ".state.json";
        if (name.size() <= suffix.size() ||
            name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0) {
            continue;
        }

        han::ui::ChapterEntry chapter;
        chapter.id = name.substr(0, name.size() - suffix.size());
        chapter.title = chapter.id;

        std::ifstream stream(entry.path(), std::ios::binary);
        if (stream) {
            try {
                const han::narrative::Json doc = han::narrative::Json::parse(stream);
                chapter.title = doc.value("title", chapter.id);
            } catch (const std::exception&) {
                // A malformed chapter still shows up by id; loading reports it.
            }
        }
        chapters.push_back(std::move(chapter));
    }

    std::sort(chapters.begin(), chapters.end(),
              [](const han::ui::ChapterEntry& left, const han::ui::ChapterEntry& right) {
                  return left.id < right.id;
              });
    return chapters;
}

bool read_save(const std::filesystem::path& path, han::narrative::Json& out) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return false;
    }
    try {
        out = han::narrative::Json::parse(stream);
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

bool write_save(const std::filesystem::path& path,
                const han::narrative::NarrativeSession& session) {
    std::error_code ec;
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path(), ec);
    }
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        return false;
    }
    stream << session.snapshot().dump(2);
    return static_cast<bool>(stream);
}

} // namespace

int main(int argc, char** argv) {
    std::filesystem::path data_dir = resolve_default_dir(HAN_DATA_DIR);
    std::filesystem::path asset_dir = resolve_default_dir(HAN_ASSET_DIR);
    std::filesystem::path font_path;
    std::string chapter_id = "ch01";
    int frame_limit = 0;
    bool campaign = false;
    bool check_only = false;
    bool chapter_given = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        }
        if (arg == "--data" && i + 1 < argc) {
            data_dir = argv[++i];
        } else if (arg == "--chapter" && i + 1 < argc) {
            chapter_id = argv[++i];
            chapter_given = true;
        } else if (arg == "--assets" && i + 1 < argc) {
            asset_dir = argv[++i];
        } else if (arg == "--font" && i + 1 < argc) {
            font_path = argv[++i];
        } else if (arg == "--frames" && i + 1 < argc) {
            frame_limit = std::stoi(argv[++i]);
        } else if (arg == "--campaign") {
            campaign = true;
        } else if (arg == "--check") {
            check_only = true;
        } else {
            std::cerr << "未知参数: " << arg << "\n";
            print_usage();
            return 2;
        }
    }

    if (check_only) {
        han::narrative::NarrativeSession session;
        std::string error;
        if (!session.load(data_dir, chapter_id, &error)) {
            std::cerr << "加载剧情失败: " << error << "\n";
            return 1;
        }
        std::cout << "数据检查通过: " << session.chapter_title() << " (" << chapter_id
                  << ")\n";
        return 0;
    }

    han::ui::DialogueScreen screen;
    std::string error;
    if (!screen.initialize(han::ui::UiConfig{asset_dir, font_path}, &error)) {
        std::cerr << "初始化界面失败: " << error << "\n";
        return 1;
    }

    const std::vector<han::ui::ChapterEntry> chapters = discover_chapters(data_dir);
    const std::string default_chapter =
        chapters.empty() ? std::string("ch01") : chapters.front().id;
    const std::filesystem::path save_path = executable_dir() / "saves" / "slot1.json";

    han::ui::TitleScreen title_screen;
    title_screen.set_chapters(chapters);
    title_screen.set_has_save(std::filesystem::exists(save_path));
    title_screen.open();

    const auto base_title = []() { return sf::String(L"悬泉渡"); };
    const auto make_title = [](const han::narrative::NarrativeSession& current) {
        const std::string text = "悬泉渡 · " + current.chapter_title();
        return sf::String::fromUtf8(text.begin(), text.end());
    };

    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode(1280, 720), base_title(),
                            sf::Style::Default, settings);
    if (!window.isOpen()) {
        std::cerr << "无法创建窗口（当前环境没有可用的桌面会话）\n";
        return 1;
    }
    window.setVerticalSyncEnabled(true);

    han::narrative::NarrativeSession session;
    bool in_game = false;

    std::function<void()> advance_scenes;
    std::function<bool()> enter_next_chapter;

    const auto autosave = [&]() {
        if (!in_game || session.scene_active()) {
            return;
        }
        if (write_save(save_path, session)) {
            title_screen.set_has_save(true);
        }
    };

    const auto enter_chapter = [&](const std::string& requested,
                                   const han::narrative::Json* save) -> bool {
        std::string load_error;
        han::narrative::NarrativeSession next;
        const std::string target =
            save != nullptr ? save->value("chapter_id", requested) : requested;
        if (!next.load(data_dir, target, &load_error)) {
            std::cerr << "加载章节失败: " << load_error << "\n";
            return false;
        }
        if (save != nullptr) {
            if (!next.restore(*save, &load_error)) {
                std::cerr << "读取存档失败: " << load_error << "\n";
                return false;
            }
        } else {
            next.start();
        }

        session = std::move(next);
        in_game = true;
        screen.reset_chapter();
        window.setTitle(make_title(session));
        screen.show_chapter_card(session.chapter_title());
        std::cout << "进入 " << session.chapter_title() << std::endl;

        // Chapter start is a safe save point: no scene is active yet.
        autosave();
        advance_scenes();
        return true;
    };

    advance_scenes = [&]() {
        while (!session.scene_active()) {
            std::string scene_error;
            if (!session.play_next_scene(&scene_error)) {
                if (!scene_error.empty()) {
                    std::cerr << "推进场景失败: " << scene_error << "\n";
                }
                if (campaign && enter_next_chapter()) {
                    return;
                }
                screen.set_chapter_finished(true);
                return;
            }
        }
    };

    enter_next_chapter = [&]() -> bool {
        const std::string next = session.next_chapter();
        if (next.empty() || !std::filesystem::exists(data_dir / (next + ".state.json"))) {
            return false;
        }

        std::string next_error;
        han::narrative::NarrativeSession next_session;
        if (!next_session.load(data_dir, next, session, &next_error)) {
            std::cerr << "加载下一章失败: " << next_error << "\n";
            return false;
        }
        next_session.start();

        session = std::move(next_session);
        screen.reset_chapter();
        window.setTitle(make_title(session));
        screen.show_chapter_card(session.chapter_title());
        std::cout << "进入 " << session.chapter_title() << std::endl;

        autosave();
        advance_scenes();
        return true;
    };

    const auto leave_to_title = [&]() {
        autosave();
        in_game = false;
        title_screen.set_has_save(std::filesystem::exists(save_path));
        title_screen.open();
        window.setTitle(base_title());
    };

    const auto handle_title_action = [&](han::ui::TitleScreen::Action action) {
        switch (action) {
        case han::ui::TitleScreen::Action::NewGame:
            enter_chapter(default_chapter, nullptr);
            break;
        case han::ui::TitleScreen::Action::Continue: {
            han::narrative::Json save;
            if (!read_save(save_path, save)) {
                std::cerr << "存档读取失败\n";
                title_screen.set_has_save(false);
                break;
            }
            enter_chapter(save.value("chapter_id", default_chapter), &save);
            break;
        }
        case han::ui::TitleScreen::Action::ChapterSelected:
            enter_chapter(title_screen.selected_chapter(), nullptr);
            break;
        case han::ui::TitleScreen::Action::Quit:
            window.close();
            break;
        case han::ui::TitleScreen::Action::None:
        default:
            break;
        }
    };

    // Command line chapter selection and smoke runs go straight into the game.
    if (chapter_given || frame_limit > 0) {
        if (!enter_chapter(chapter_id, nullptr)) {
            return 1;
        }
    }

    int frames = 0;
    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                continue;
            }

            if (!in_game) {
                if (event.type == sf::Event::KeyPressed) {
                    handle_title_action(title_screen.handle_key(event.key.code));
                } else if (event.type == sf::Event::MouseButtonPressed &&
                           event.mouseButton.button == sf::Mouse::Left) {
                    const sf::Vector2f point = window.mapPixelToCoords(
                        sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                    handle_title_action(title_screen.handle_click(point));
                } else if (event.type == sf::Event::MouseWheelScrolled &&
                           event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                    title_screen.handle_wheel(event.mouseWheelScroll.delta);
                }
                continue;
            }

            if (event.type == sf::Event::KeyPressed) {
                if (screen.chapter_finished()) {
                    if (event.key.code == sf::Keyboard::Escape) {
                        leave_to_title();
                    } else if (event.key.code == sf::Keyboard::Enter ||
                               event.key.code == sf::Keyboard::Return ||
                               event.key.code == sf::Keyboard::Space) {
                        if (!enter_next_chapter()) {
                            leave_to_title();
                        }
                    }
                } else if (screen.handle_key(event.key.code, session)) {
                    leave_to_title();
                } else {
                    // Save while the session may still be at a scene boundary.
                    autosave();
                    advance_scenes();
                }
            } else if (event.type == sf::Event::MouseButtonPressed &&
                       event.mouseButton.button == sf::Mouse::Left) {
                const sf::Vector2f point = window.mapPixelToCoords(
                    sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                if (screen.chapter_finished()) {
                    if (!enter_next_chapter()) {
                        leave_to_title();
                    }
                } else {
                    screen.handle_click(point, session);
                    autosave();
                    advance_scenes();
                }
            } else if (event.type == sf::Event::MouseWheelScrolled &&
                       event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                screen.handle_wheel(event.mouseWheelScroll.delta, session);
            }
        }

        window.clear(sf::Color(12, 10, 8));
        if (in_game) {
            screen.render(window, session);
        } else {
            title_screen.render(window, screen.font());
        }
        window.display();

        if (frame_limit > 0 && ++frames >= frame_limit) {
            std::cout << "冒烟渲染 " << frames << " 帧完成";
            if (in_game) {
                std::cout << "，场景 " << session.played_scenes().size() << " 个";
            }
            std::cout << std::endl;
            window.close();
        }
    }

    return 0;
}

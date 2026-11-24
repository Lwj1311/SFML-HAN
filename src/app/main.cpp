#include "narrative/session.h"
#include "ui/dialogue_screen.h"

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

#include <filesystem>
#include <functional>
#include <iostream>
#include <string>

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
                 "  --chapter <id>   章节 id (默认 ch01)\n"
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

} // namespace

int main(int argc, char** argv) {
    std::filesystem::path data_dir = resolve_default_dir(HAN_DATA_DIR);
    std::filesystem::path asset_dir = resolve_default_dir(HAN_ASSET_DIR);
    std::filesystem::path font_path;
    std::string chapter_id = "ch01";
    int frame_limit = 0;
    bool campaign = false;
    bool check_only = false;

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

    han::narrative::NarrativeSession session;
    std::string error;
    if (!session.load(data_dir, chapter_id, &error)) {
        std::cerr << "加载剧情失败: " << error << "\n";
        return 1;
    }
    if (check_only) {
        std::cout << "数据检查通过: " << session.chapter_title() << " (" << chapter_id << ")\n";
        return 0;
    }
    session.start();

    han::ui::DialogueScreen screen;
    if (!screen.initialize(han::ui::UiConfig{asset_dir, font_path}, &error)) {
        std::cerr << "初始化界面失败: " << error << "\n";
        return 1;
    }

    const auto make_title = [](const han::narrative::NarrativeSession& current) {
        const std::string text = "悬泉渡 · " + current.chapter_title();
        return sf::String::fromUtf8(text.begin(), text.end());
    };

    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode(1280, 720), make_title(session),
                            sf::Style::Default, settings);
    if (!window.isOpen()) {
        std::cerr << "无法创建窗口（当前环境没有可用的桌面会话）\n";
        return 1;
    }
    window.setVerticalSyncEnabled(true);

    std::function<void()> advance_scenes;
    auto enter_next_chapter = [&]() -> bool {
        const std::string next = session.next_chapter();
        if (next.empty()) {
            return false;
        }
        if (!std::filesystem::exists(data_dir / (next + ".state.json"))) {
            return false;
        }

        han::narrative::NarrativeSession next_session;
        std::string next_error;
        if (!next_session.load(data_dir, next, session, &next_error)) {
            std::cerr << "加载下一章失败: " << next_error << "\n";
            return false;
        }
        next_session.start();
        session = std::move(next_session);

        screen.reset_chapter();
        window.setTitle(make_title(session));
        std::cout << "进入 " << session.chapter_title() << std::endl;

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
    advance_scenes();

    int frames = 0;
    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::KeyPressed) {
                if (screen.handle_key(event.key.code, session)) {
                    window.close();
                } else {
                    advance_scenes();
                }
            } else if (event.type == sf::Event::MouseButtonPressed &&
                       event.mouseButton.button == sf::Mouse::Left) {
                const sf::Vector2f point = window.mapPixelToCoords(
                    sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                if (screen.handle_click(point, session)) {
                    advance_scenes();
                }
            } else if (event.type == sf::Event::MouseWheelScrolled) {
                if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                    screen.handle_wheel(event.mouseWheelScroll.delta, session);
                }
            }
        }

        window.clear(sf::Color(12, 10, 8));
        screen.render(window, session);
        window.display();

        if (frame_limit > 0 && ++frames >= frame_limit) {
            std::cout << "冒烟渲染 " << frames << " 帧完成，场景 "
                      << session.played_scenes().size() << " 个" << std::endl;
            window.close();
        }
    }

    return 0;
}

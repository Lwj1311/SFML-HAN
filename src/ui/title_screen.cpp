#include "title_screen.h"

#include <algorithm>

namespace han::ui {
namespace {

const sf::Color kInk(10, 8, 6);
const sf::Color kGold(214, 178, 106);
const sf::Color kText(236, 228, 210);
const sf::Color kDim(150, 138, 118);

constexpr std::size_t kMainItemCount = 4;

sf::String from_utf8(const std::string& text) {
    return sf::String::fromUtf8(text.begin(), text.end());
}

void draw_text(sf::RenderTarget& target, const sf::Font& font, unsigned size,
               const sf::String& text, float x, float y, sf::Color color,
               float letter_spacing = 1.f) {
    sf::Text drawable(text, font, size);
    drawable.setFillColor(color);
    drawable.setLetterSpacing(letter_spacing);
    drawable.setPosition(x, y);
    target.draw(drawable);
}

float text_width(const sf::Font& font, unsigned size, const sf::String& text,
                 float letter_spacing = 1.f) {
    sf::Text drawable(text, font, size);
    drawable.setLetterSpacing(letter_spacing);
    return drawable.getLocalBounds().width;
}

} // namespace

void TitleScreen::set_chapters(std::vector<ChapterEntry> chapters) {
    chapters_ = std::move(chapters);
    if (chapter_selection_ >= chapters_.size()) {
        chapter_selection_ = 0;
    }
}

void TitleScreen::open() {
    mode_ = Mode::Main;
    main_rects_.clear();
    chapter_rects_.clear();
}

TitleScreen::Action TitleScreen::activate_main() {
    switch (main_selection_) {
    case 0:
        return Action::NewGame;
    case 1:
        return has_save_ ? Action::Continue : Action::None;
    case 2:
        mode_ = Mode::Chapters;
        chapter_selection_ = 0;
        return Action::None;
    case 3:
    default:
        return Action::Quit;
    }
}

TitleScreen::Action TitleScreen::activate_chapter() {
    if (chapters_.empty()) {
        return Action::None;
    }
    chapter_selection_ = std::min(chapter_selection_, chapters_.size() - 1);
    selected_chapter_ = chapters_[chapter_selection_].id;
    return Action::ChapterSelected;
}

TitleScreen::Action TitleScreen::handle_key(sf::Keyboard::Key key) {
    if (mode_ == Mode::Chapters) {
        if (key == sf::Keyboard::Escape || key == sf::Keyboard::BackSpace) {
            mode_ = Mode::Main;
            return Action::None;
        }
        if (chapters_.empty()) {
            return Action::None;
        }
        if (key == sf::Keyboard::Up || key == sf::Keyboard::W || key == sf::Keyboard::K) {
            const std::size_t count = chapters_.size();
            chapter_selection_ = (chapter_selection_ + count - 1) % count;
        } else if (key == sf::Keyboard::Down || key == sf::Keyboard::S ||
                   key == sf::Keyboard::J) {
            chapter_selection_ = (chapter_selection_ + 1) % chapters_.size();
        } else if (key == sf::Keyboard::Enter || key == sf::Keyboard::Return ||
                   key == sf::Keyboard::Space) {
            return activate_chapter();
        }
        return Action::None;
    }

    if (key == sf::Keyboard::Escape) {
        return Action::Quit;
    }
    if (key == sf::Keyboard::Up || key == sf::Keyboard::W || key == sf::Keyboard::K) {
        main_selection_ = (main_selection_ + kMainItemCount - 1) % kMainItemCount;
    } else if (key == sf::Keyboard::Down || key == sf::Keyboard::S ||
               key == sf::Keyboard::J) {
        main_selection_ = (main_selection_ + 1) % kMainItemCount;
    } else if (key == sf::Keyboard::Enter || key == sf::Keyboard::Return ||
               key == sf::Keyboard::Space) {
        return activate_main();
    }
    return Action::None;
}

TitleScreen::Action TitleScreen::handle_click(sf::Vector2f position) {
    if (mode_ == Mode::Chapters) {
        for (std::size_t i = 0; i < chapter_rects_.size(); ++i) {
            if (chapter_rects_[i].contains(position.x, position.y)) {
                chapter_selection_ = i;
                return activate_chapter();
            }
        }
        return Action::None;
    }

    for (std::size_t i = 0; i < main_rects_.size(); ++i) {
        if (main_rects_[i].contains(position.x, position.y)) {
            main_selection_ = i;
            return activate_main();
        }
    }
    return Action::None;
}

TitleScreen::Action TitleScreen::handle_wheel(float delta) {
    if (mode_ == Mode::Chapters && !chapters_.empty()) {
        const std::size_t count = chapters_.size();
        if (delta < 0.f) {
            chapter_selection_ = (chapter_selection_ + 1) % count;
        } else if (delta > 0.f) {
            chapter_selection_ = (chapter_selection_ + count - 1) % count;
        }
    }
    return Action::None;
}

void TitleScreen::render(sf::RenderWindow& window, const sf::Font& font) {
    const sf::Vector2u window_size = window.getSize();
    const float width = static_cast<float>(window_size.x);
    const float height = static_cast<float>(window_size.y);

    window.clear(kInk);

    // Vertical ink wash: a subtle dusk gradient rather than a flat colour.
    constexpr int kBands = 48;
    for (int i = 0; i < kBands; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kBands - 1);
        const auto mix = [t](int top, int bottom) {
            return static_cast<sf::Uint8>(static_cast<float>(top) +
                                          (static_cast<float>(bottom) - top) * t);
        };
        sf::RectangleShape band(sf::Vector2f(width, height / kBands + 1.f));
        band.setPosition(0.f, height * i / kBands);
        band.setFillColor(sf::Color(mix(34, 8), mix(30, 7), mix(24, 5)));
        window.draw(band);
    }

    // Pale moon and halo.
    sf::CircleShape halo(150.f);
    halo.setOrigin(150.f, 150.f);
    halo.setPosition(width * 0.78f, height * 0.24f);
    halo.setFillColor(sf::Color(214, 190, 140, 14));
    window.draw(halo);

    sf::CircleShape moon(108.f);
    moon.setOrigin(108.f, 108.f);
    moon.setPosition(width * 0.78f, height * 0.24f);
    moon.setFillColor(sf::Color(206, 186, 146, 26));
    window.draw(moon);

    sf::RectangleShape frame(sf::Vector2f(width - 56.f, height - 56.f));
    frame.setPosition(28.f, 28.f);
    frame.setFillColor(sf::Color::Transparent);
    frame.setOutlineThickness(1.f);
    frame.setOutlineColor(sf::Color(kGold.r, kGold.g, kGold.b, 46));
    window.draw(frame);

    if (mode_ == Mode::Chapters) {
        const sf::String heading = from_utf8("选择章节");
        draw_text(window, font, 40, heading,
                  (width - text_width(font, 40, heading, 1.2f)) * 0.5f, 112.f,
                  kGold, 1.2f);

        const float item_w = std::min(560.f, width - 200.f);
        const float item_h = 58.f;
        const float gap = 12.f;
        const float start_y = 196.f;
        chapter_rects_.clear();

        if (chapters_.empty()) {
            const sf::String empty = from_utf8("未找到章节数据");
            draw_text(window, font, 20, empty,
                      (width - text_width(font, 20, empty)) * 0.5f, start_y + 20.f, kDim);
        }

        for (std::size_t i = 0; i < chapters_.size(); ++i) {
            const float y = start_y + static_cast<float>(i) * (item_h + gap);
            sf::FloatRect rect((width - item_w) * 0.5f, y, item_w, item_h);
            chapter_rects_.push_back(rect);

            const bool active = i == chapter_selection_;
            sf::RectangleShape row(sf::Vector2f(item_w, item_h));
            row.setPosition(rect.left, rect.top);
            row.setFillColor(active ? sf::Color(kGold.r, kGold.g, kGold.b, 34)
                                    : sf::Color(18, 16, 13, 190));
            row.setOutlineThickness(1.f);
            row.setOutlineColor(active ? sf::Color(kGold.r, kGold.g, kGold.b, 170)
                                       : sf::Color(kDim.r, kDim.g, kDim.b, 45));
            window.draw(row);

            const sf::String label = from_utf8(chapters_[i].title);
            draw_text(window, font, 22, label,
                      rect.left + (item_w - text_width(font, 22, label)) * 0.5f,
                      rect.top + 15.f, active ? kGold : kText);
        }

        const sf::String hint = from_utf8("↑↓ 选择 · 回车进入 · Esc 返回");
        draw_text(window, font, 16, hint,
                  (width - text_width(font, 16, hint)) * 0.5f, height - 76.f, kDim);
        return;
    }

    const sf::String title = from_utf8("悬泉渡");
    draw_text(window, font, 88, title,
              (width - text_width(font, 88, title, 1.35f)) * 0.5f, height * 0.17f,
              kGold, 1.35f);

    const sf::String subtitle = from_utf8("汉 · 敦煌 · 悬泉驿");
    draw_text(window, font, 20, subtitle,
              (width - text_width(font, 20, subtitle, 1.2f)) * 0.5f, height * 0.35f,
              kDim, 1.2f);

    const float rule_w = 240.f;
    sf::RectangleShape rule(sf::Vector2f(rule_w, 1.f));
    rule.setPosition((width - rule_w) * 0.5f, height * 0.35f + 42.f);
    rule.setFillColor(sf::Color(kGold.r, kGold.g, kGold.b, 120));
    window.draw(rule);

    const char* labels[] = {"新游戏", "继续", "章节选择", "退出"};
    const bool enabled[] = {true, has_save_, true, true};

    const float item_w = std::min(320.f, width - 160.f);
    const float item_h = 54.f;
    const float gap = 10.f;
    const float start_y = height * 0.48f;
    main_rects_.clear();

    for (std::size_t i = 0; i < kMainItemCount; ++i) {
        const float y = start_y + static_cast<float>(i) * (item_h + gap);
        sf::FloatRect rect((width - item_w) * 0.5f, y, item_w, item_h);
        main_rects_.push_back(rect);

        const bool active = i == main_selection_;
        sf::RectangleShape row(sf::Vector2f(item_w, item_h));
        row.setPosition(rect.left, rect.top);
        row.setFillColor(active ? sf::Color(kGold.r, kGold.g, kGold.b, 30)
                                : sf::Color(16, 14, 11, 170));
        row.setOutlineThickness(1.f);
        row.setOutlineColor(active ? sf::Color(kGold.r, kGold.g, kGold.b, 160)
                                   : sf::Color(kDim.r, kDim.g, kDim.b, 40));
        window.draw(row);

        if (active) {
            sf::RectangleShape marker(sf::Vector2f(4.f, item_h));
            marker.setPosition(rect.left, rect.top);
            marker.setFillColor(kGold);
            window.draw(marker);
        }

        sf::Color color = kText;
        if (!enabled[i]) {
            color = sf::Color(kDim.r, kDim.g, kDim.b, 110);
        } else if (active) {
            color = kGold;
        }

        const sf::String label = from_utf8(labels[i]);
        draw_text(window, font, 22, label,
                  rect.left + (item_w - text_width(font, 22, label)) * 0.5f,
                  rect.top + 13.f, color);
    }

    const sf::String hint = from_utf8("↑↓ 选择 · 回车确认 · Esc 退出");
    draw_text(window, font, 16, hint,
              (width - text_width(font, 16, hint)) * 0.5f, height - 76.f, kDim);

    const sf::String version = from_utf8("开发版 · 全七章");
    draw_text(window, font, 14, version, width - 64.f - text_width(font, 14, version),
              height - 62.f, sf::Color(kDim.r, kDim.g, kDim.b, 130));
}

} // namespace han::ui
